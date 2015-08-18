#ifndef CORGC_DESCRIPTOR_MACRO_H
#define CORGC_DESCRIPTOR_MACRO_H

#ifndef mword
#if SIZEOF_VOID_P == 4
typedef guint32 mword;
#else
typedef guint64 mword;
#endif //SIZEOF_VOID_P == 4
#endif //mword

#define CORGC_VTABLE_HAS_REFERENCES(vt)	(corgc_descr_has_references ((mword)((MonoVTable*)(vt))->gc_descr))
#define CORGC_CLASS_HAS_REFERENCES(c)	(corgc_descr_has_references ((mword)(c)->gc_descr))
#define SGEN_OBJECT_HAS_REFERENCES(o)	(vtable_has_references (vtable((o))))

/* helper macros to scan and traverse objects, macros because we resue them in many functions */
#ifdef __GNUC__
#define PREFETCH_READ(addr)	__builtin_prefetch ((addr), 0, 1)
#define PREFETCH_WRITE(addr)	__builtin_prefetch ((addr), 1, 1)
#else
#define PREFETCH_READ(addr)
#define PREFETCH_WRITE(addr)
#endif //__GNUC__

#if defined(__GNUC__) && SIZEOF_VOID_P==4
#define GNUC_BUILTIN_CTZ(bmap)	__builtin_ctz(bmap)
#elif defined(__GNUC__) && SIZEOF_VOID_P==8
#define GNUC_BUILTIN_CTZ(bmap)	__builtin_ctzl(bmap)
#endif // __GNUC__

#define OBJ_FOREACH_PTR(obj,parm,start,exp)			\
  {								\
    gsize desc = (gsize)vtable(obj)->gc_descr;			\
    switch(desc & DESC_TYPE_MASK){				\
    case DESC_TYPE_RUN_LENGTH:					\
      OBJ_RUN_LEN_FOREACH_PTR(desc,(obj),parm,(start),exp)	\
    case DESC_TYPE_VECTOR:					\
      OBJ_VECTOR_FOREACH_PTR(desc,(obj),parm,(start),exp)	\
    case DESC_TYPE_BITMAP:					\
      OBJ_BITMAP_FOREACH_PTR(desc,(obj),parm,(start),exp)	\
    case DESC_TYPE_COMPLEX:					\
      OBJ_COMPLEX_FOREACH_PTR(desc,(obj),parm,(start),exp)	\
    case DESC_TYPE_COMPLEX_ARR:					\
      OBJ_COMPLEX_ARR_FOREACH_PTR(desc,(obj),parm,(start),exp)	\
    case DESC_TYPE_SMALL_PTRFREE:				\
    case DESC_TYPE_COMPLEX_PTRFREE:				\
      break;							\
    default:							\
      g_assert_not_reached();					\
    }								\
  }

/* code using these macros must define a HANDLE_PTR(ptr) macro that does the work */
#define OBJ_RUN_LEN_FOREACH_PTR(desc,obj,parm,start,exp)		\
  {									\
    /* there are pointers */						\
    void **_objptr_end;							\
    void **parm = (void**)(start);					\
    parm += ((desc) >> 16) & 0xff;					\
    _objptr_end = (void**)(obj) + (((desc) >> 24) & 0xff);		\
    while (parm < _objptr_end) {					\
      {exp};								\
      parm++;								\
    };									\
  }

/* a bitmap desc means that there are pointer references or we'd have
 * choosen run-length, instead: add an assert to check.
 */
#ifdef __GNUC__
#define OBJ_BITMAP_FOREACH_PTR(desc,obj,parm,start,exp)		\
  {								\
    /* there are pointers */					\
    void **parm = (void**)(obj);				\
    gsize _bmap = (desc) >> LOW_TYPE_BITS;			\
    parm += OBJECT_HEADER_WORDS;				\
    do {							\
      int _index = GNUC_BUILTIN_CTZ (_bmap);			\
      parm += _index;						\
      _bmap >>= (_index + 1);					\
      {exp}							\
      ++parm;							\
    } while (_bmap);						\
  }

#else

#define OBJ_BITMAP_FOREACH_PTR(desc,obj,start,exp)		\
  {								\
    /* there are pointers */					\
    void **parm = (void**)(obj);				\
    gsize _bmap = (desc) >> LOW_TYPE_BITS;			\
    parm += OBJECT_HEADER_WORDS;				\
    do {							\
      if ((_bmap & 1)) {					\
	{exp}							\
      }								\
      _bmap >>= 1;						\
      ++parm;							\
    } while (_bmap);						\
  }
#endif // __GNUC__

#define OBJ_COMPLEX_FOREACH_PTR(vt,obj,parm,start,exp)			\
  {									\
    /* there are pointers */						\
    gsize desc = (gsize)(((MonoVTable*)(vt))->gc_descr);		\
    void **parm = (void**)(obj);					\
    gsize *bitmap_data = corgc_get_complex_descriptor ((desc));		\
    gsize bwords = (*bitmap_data) - 1;					\
    void **start_run = parm;						\
    bitmap_data++;							\
    while (bwords-- > 0) {						\
      gsize _bmap = *bitmap_data++;					\
      parm = start_run;							\
      while (_bmap) {							\
	if ((_bmap & 1)) {						\
	  {exp}								\
	}								\
	_bmap >>= 1;							\
	++parm;								\
      }									\
      start_run += GC_BITS_PER_WORD;					\
    }									\
  }

#define OBJ_COMPLEX_ARR_FOREACH_PTR(desc,obj,parm,start,exp)		\
  {									\
    /* there are pointers */						\
    MonoVTable *vt = vtable(obj);					\
    gsize *mbitmap_data = corgc_get_complex_descriptor ((desc));	\
    gsize mbwords = (*mbitmap_data++) - 1;				\
    gsize el_size = mono_array_element_size (vt->klass);		\
    char *e_start = (char*)(obj) +  G_STRUCT_OFFSET (MonoArray, vector); \
    char *e_end = e_start + el_size * mono_array_length_fast ((MonoArray*)(obj)); \
    while (e_start < e_end) {						\
      void **parm = (void**)e_start;					\
      gsize *bitmap_data = mbitmap_data;				\
      gsize bwords = mbwords;						\
      while (bwords-- > 0) {						\
	gsize _bmap = *bitmap_data++;					\
	void **start_run = parm;						\
	/*g_print ("bitmap: 0x%x\n", _bmap);*/				\
	while (_bmap) {							\
	  if ((_bmap & 1)) {						\
	    {exp}							\
	  }								\
	  _bmap >>= 1;							\
	  ++parm;							\
	}								\
	parm = start_run + GC_BITS_PER_WORD;				\
      }									\
      e_start += el_size;						\
    }									\
  }

#define OBJ_VECTOR_FOREACH_PTR(desc,obj,parm,start,exp)			\
  {									\
    /* note: 0xffffc000 excludes DESC_TYPE_V_PTRFREE */			\
    if ((desc) & 0xffffc000) {						\
      int el_size = ((desc) >> 3) & MAX_ELEMENT_SIZE;			\
      /* there are pointers */						\
      int etype = (desc) & 0xc000;					\
      if (etype == (DESC_TYPE_V_REFS << 14)) {				\
	void **parm = (void**)((char*)(obj) + G_STRUCT_OFFSET (MonoArray, vector)); \
	void **end_refs = (void**)((char*)parm + el_size * mono_array_length_fast ((MonoArray*)(obj))); \
	/* Note: this code can handle also arrays of struct with only references in them */ \
	while (parm < end_refs) {					\
	  {exp}								\
	  ++parm;							\
	}								\
      }									\
      else if (etype == DESC_TYPE_V_RUN_LEN << 14) {			\
	int offset = ((desc) >> 16) & 0xff;				\
	int num_refs = ((desc) >> 24) & 0xff;				\
	char *e_start = (char*)(obj) + G_STRUCT_OFFSET (MonoArray, vector); \
	char *e_end = e_start + el_size * mono_array_length_fast ((MonoArray*)(obj)); \
	while (e_start < e_end) {					\
	  void **parm = (void**)e_start;				\
	  int i;							\
	  parm += offset;						\
	  for (i = 0; i < num_refs; ++i) {				\
	    ++parm;							\
	    {exp}							\
	  }								\
	  parm -= num_refs;						\
	  e_start += el_size;						\
	}								\
      }									\
      else if (etype == DESC_TYPE_V_BITMAP << 14) {			\
	char *e_start = (char*)(obj) +  G_STRUCT_OFFSET (MonoArray, vector); \
	char *e_end = e_start + el_size * mono_array_length_fast ((MonoArray*)(obj)); \
	while (e_start < e_end) {					\
	  void **parm = (void**)e_start;				\
	  gsize _bmap = (desc) >> 16;					\
	  /* Note: there is no object header here to skip */		\
	  while (_bmap) {						\
	    if ((_bmap & 1)) {						\
	      {exp}							\
	    }								\
	    _bmap >>= 1;						\
	    ++parm;							\
	  }								\
	  e_start += el_size;						\
	}								\
      }									\
    }									\
  }

#endif
