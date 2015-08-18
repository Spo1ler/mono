#include "corgc-descriptor.h"

#define MAX_USER_DESCRIPTORS 16

#define MAKE_ROOT_DESC(type,val) ((type) | ((val) << ROOT_DESC_TYPE_SHIFT))
#define ALIGN_TO(val,align) ((((guint64)val) + ((align) - 1)) & ~((align) - 1))

// this code comes from sgen-descriptor.c

static gsize* complex_descriptors = NULL;
static int complex_descriptors_size = 0;
static int complex_descriptors_next = 0;
static MonoGCRootMarkFunc user_descriptors [MAX_USER_DESCRIPTORS];
static int user_descriptors_next = 0;
static void *all_ref_root_descrs [32];

int
alloc_complex_descriptor (gsize *bitmap, int numbits)
{
  int nwords, res, i;

  numbits = ALIGN_TO (numbits, 8);
  nwords = numbits / 8 + 1;

  corgc_lock ();
  res = complex_descriptors_next;
  /* linear search, so we don't have duplicates with domain load/unload
   * this should not be performance critical or we'd have bigger issues
   * (the number and size of complex descriptors should be small).
   */
  for (i = 0; i < complex_descriptors_next; ) {
    if (complex_descriptors [i] == nwords) {
      int j, found = TRUE;
      for (j = 0; j < nwords - 1; ++j) {
	if (complex_descriptors [i + 1 + j] != bitmap [j]) {
	  found = FALSE;
	  break;
	}
      }
      if (found) {
	corgc_unlock ();
	return i;
      }
    }
    i += (int)complex_descriptors [i];
  }
  if (complex_descriptors_next + nwords > complex_descriptors_size) {
    int new_size = complex_descriptors_size * 2 + nwords;
    complex_descriptors = (gsize*)g_realloc (complex_descriptors, new_size * sizeof (gsize));
    complex_descriptors_size = new_size;
  }
  complex_descriptors_next += nwords;
  complex_descriptors [res] = nwords;
  for (i = 0; i < nwords - 1; ++i) {
    complex_descriptors [res + 1 + i] = bitmap [i];
  }
  corgc_unlock ();
  return res;
}

gsize*
corgc_get_complex_descriptor (mword desc)
{
  return complex_descriptors + (desc >> LOW_TYPE_BITS);
}

extern "C" void*
mono_gc_make_descr_for_string (gsize *bitmap, int numbits)
{
  	return (void*)CORGC_DESC_STRING;
}

extern "C" void*
mono_gc_make_descr_for_object (gsize *bitmap, int numbits, size_t obj_size)
{
  int first_set = -1, num_set = 0, last_set = -1, i;
  mword desc = 0;
  size_t stored_size = ALIGN_TO(obj_size,ALLOC_ALIGN);

  for (i = 0; i < numbits; ++i) {
    if (bitmap [i / GC_BITS_PER_WORD] & ((gsize)1 << (i % GC_BITS_PER_WORD))) {
      if (first_set < 0)
	first_set = i;
      last_set = i;
      num_set++;
    }
  }

  if (first_set < 0) {
    if (stored_size <= MAX_RUNLEN_OBJECT_SIZE && stored_size <= LARGE_OBJECT_SIZE)
      return (void*)(DESC_TYPE_SMALL_PTRFREE | stored_size);
    return (void*)DESC_TYPE_COMPLEX_PTRFREE;
  }

  g_assert (!(stored_size & 0x7));

  /* we know the 2-word header is ptr-free */
  if (last_set < BITMAP_NUM_BITS + OBJECT_HEADER_WORDS && stored_size <= LARGE_OBJECT_SIZE) {
    desc = DESC_TYPE_BITMAP | ((*bitmap >> OBJECT_HEADER_WORDS) << LOW_TYPE_BITS);
    return (void*) desc;
  }

  if (stored_size <= MAX_RUNLEN_OBJECT_SIZE && stored_size <= LARGE_OBJECT_SIZE) {
    /* check run-length encoding first: one byte offset, one byte number of pointers
     * on 64 bit archs, we can have 3 runs, just one on 32.
     * It may be better to use nibbles.
     */
    if (first_set < 256 && num_set < 256 && (first_set + num_set == last_set + 1)) {
      desc = DESC_TYPE_RUN_LENGTH | stored_size | (first_set << 16) | (num_set << 24);
      return (void*) desc;
    }
  }

  /* it's a complex object ... */
  desc = DESC_TYPE_COMPLEX | (alloc_complex_descriptor (bitmap, last_set + 1) << LOW_TYPE_BITS);
  return (void*) desc;
}

void*
mono_gc_make_descr_for_array (int vector, gsize *elem_bitmap, int numbits, size_t elem_size)
{
  int first_set = -1, num_set = 0, last_set = -1, i;
  mword desc = DESC_TYPE_VECTOR | (vector ? VECTOR_KIND_SZARRAY : VECTOR_KIND_ARRAY);
  for (i = 0; i < numbits; ++i) {
    if (elem_bitmap [i / GC_BITS_PER_WORD] & ((gsize)1 << (i % GC_BITS_PER_WORD))) {
      if (first_set < 0)
	first_set = i;
      last_set = i;
      num_set++;
    }
  }

  if (first_set < 0) {
    if (elem_size <= MAX_ELEMENT_SIZE)
      return (void*)(desc | VECTOR_SUBTYPE_PTRFREE | (elem_size << VECTOR_ELSIZE_SHIFT));
    return (void*)DESC_TYPE_COMPLEX_PTRFREE;
  }

  if (elem_size <= MAX_ELEMENT_SIZE) {
    desc |= elem_size << VECTOR_ELSIZE_SHIFT;
    if (!num_set) {
      return (void*)(desc | VECTOR_SUBTYPE_PTRFREE);
    }
    /* Note: we also handle structs with just ref fields */
    if (num_set * sizeof (gpointer) == elem_size) {
      return (void*)(desc | VECTOR_SUBTYPE_REFS | ((gssize)(-1) << 16));
    }
    /* FIXME: try run-len first */
    /* Note: we can't skip the object header here, because it's not present */
    if (last_set < VECTOR_BITMAP_SIZE) {
      return (void*)(desc | VECTOR_SUBTYPE_BITMAP | (*elem_bitmap << 16));
    }
  }
  /* it's am array of complex structs ... */
  desc = DESC_TYPE_COMPLEX_ARR;
  desc |= alloc_complex_descriptor (elem_bitmap, last_set + 1) << LOW_TYPE_BITS;
  return (void*) desc;
}

/* Return the bitmap encoded by a descriptor */
extern "C" gsize*
mono_gc_get_bitmap_for_descr (void *descr, int *numbits)
{
  mword d = (mword)descr;
  gsize *bitmap;

  switch (d & DESC_TYPE_MASK) {
  case DESC_TYPE_RUN_LENGTH: {		
    int first_set = (d >> 16) & 0xff;
    int num_set = (d >> 24) & 0xff;
    int i;

    bitmap = g_new0 (gsize, (first_set + num_set + 7) / 8);

    for (i = first_set; i < first_set + num_set; ++i)
      bitmap [i / GC_BITS_PER_WORD] |= ((gsize)1 << (i % GC_BITS_PER_WORD));

    *numbits = first_set + num_set;

    return bitmap;
  }

  case DESC_TYPE_BITMAP: {
    gsize bmap = (d >> LOW_TYPE_BITS) << OBJECT_HEADER_WORDS;

    bitmap = g_new0 (gsize, 1);
    bitmap [0] = bmap;
    *numbits = 0;
    while (bmap) {
      (*numbits) ++;
      bmap >>= 1;
    }
    return bitmap;
  }

  case DESC_TYPE_COMPLEX: {
    gsize *bitmap_data = corgc_get_complex_descriptor (d);
    int bwords = (int)(*bitmap_data) - 1;//Max scalar object size is 1Mb, which means up to 32k descriptor words
    int i;

    bitmap = g_new0 (gsize, bwords);
    *numbits = bwords * GC_BITS_PER_WORD;

    for (i = 0; i < bwords; ++i) {
      bitmap [i] = bitmap_data [i + 1];
    }

    return bitmap;
  }

  default:
    g_assert_not_reached ();
  }
}

extern "C" void*
mono_gc_make_descr_from_bitmap (gsize *bitmap, int numbits)
{
  if (numbits == 0) {
    return (void*)MAKE_ROOT_DESC (ROOT_DESC_BITMAP, 0);
  } else if (numbits < ((sizeof (*bitmap) * 8) - ROOT_DESC_TYPE_SHIFT)) {
    return (void*)MAKE_ROOT_DESC (ROOT_DESC_BITMAP, bitmap [0]);
  } else {
    mword complex = alloc_complex_descriptor (bitmap, numbits);
    return (void*)MAKE_ROOT_DESC (ROOT_DESC_COMPLEX, complex);
  }
}

extern "C" void*
mono_gc_make_root_descr_all_refs (int numbits)
{
  gsize *gc_bitmap;
  void *descr;
  int num_bytes = numbits / 8;

  if (numbits < 32 && all_ref_root_descrs [numbits])
    return all_ref_root_descrs [numbits];

  gc_bitmap = (gsize*)g_malloc0 (ALIGN_TO (ALIGN_TO (numbits, 8) + 1, sizeof (gsize)));
  memset (gc_bitmap, 0xff, num_bytes);
  if (numbits < ((sizeof (*gc_bitmap) * 8) - ROOT_DESC_TYPE_SHIFT)) 
    gc_bitmap[0] = GUINT64_TO_LE(gc_bitmap[0]);
  else if (numbits && num_bytes % (sizeof (*gc_bitmap)))
    gc_bitmap[num_bytes / 8] = GUINT64_TO_LE(gc_bitmap [num_bytes / 8]);
  if (numbits % 8)
    gc_bitmap [numbits / 8] = (1 << (numbits % 8)) - 1;
  descr = mono_gc_make_descr_from_bitmap (gc_bitmap, numbits);
  g_free (gc_bitmap);

  if (numbits < 32)
    all_ref_root_descrs [numbits] = descr;

  return descr;
}

extern "C" void*
mono_gc_make_root_descr_user (MonoGCRootMarkFunc marker)
{
  void *descr;

  g_assert (user_descriptors_next < MAX_USER_DESCRIPTORS);
  descr = (void*)MAKE_ROOT_DESC (ROOT_DESC_USER, (mword)user_descriptors_next);
  user_descriptors [user_descriptors_next ++] = marker;

  return descr;
}

extern "C" void*
corgc_get_complex_descriptor_bitmap (mword desc)
{
  return complex_descriptors + (desc >> ROOT_DESC_TYPE_SHIFT);
}

MonoGCRootMarkFunc
corgc_get_user_descriptor_func (mword desc)
{
  return user_descriptors [desc >> ROOT_DESC_TYPE_SHIFT];
}

void
corgc_init_descriptors (void)
{
  
}
