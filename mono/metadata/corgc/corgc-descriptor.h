/*
 * corgc-descriptor.h: GC descriptors describe object layout.

 * Copyright 2001-2003 Ximian, Inc
 * Copyright 2003-2010 Novell, Inc.
 * Copyright 2011 Xamarin Inc (http://www.xamarin.com)
 *
 * Copyright (C) 2012 Xamarin Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License 2.0 as published by the Free Software Foundation;
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License 2.0 along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __CORGC_DESCRIPTOR_H__
#define __CORGC_DESCRIPTOR_H__

// Using sgen descriptors for now

#include "corgc-descriptor-macro.h"

typedef void (*MonoGCMarkFunc)     (void **addr, void *gc_data);
typedef void (*MonoGCRootMarkFunc) (void *addr, MonoGCMarkFunc mark_func, void *gc_data);


/*
 * ######################################################################
 * ########  GC descriptors
 * ######################################################################
 * Used to quickly get the info the GC needs about an object: size and
 * where the references are held.
 */
#define OBJECT_HEADER_WORDS (sizeof(MonoObject)/sizeof(gpointer))
#define LOW_TYPE_BITS 3
#define DESC_TYPE_MASK	((1 << LOW_TYPE_BITS) - 1)
#define MAX_RUNLEN_OBJECT_SIZE 0xFFFF
#define VECTOR_INFO_SHIFT 14
#define VECTOR_KIND_SHIFT 13
#define VECTOR_ELSIZE_SHIFT 3
#define VECTOR_BITMAP_SHIFT 16
#define VECTOR_BITMAP_SIZE (GC_BITS_PER_WORD - VECTOR_BITMAP_SHIFT)
#define BITMAP_NUM_BITS (GC_BITS_PER_WORD - LOW_TYPE_BITS)
#define MAX_ELEMENT_SIZE 0x3ff
#define VECTOR_SUBTYPE_PTRFREE (DESC_TYPE_V_PTRFREE << VECTOR_INFO_SHIFT)
#define VECTOR_SUBTYPE_REFS    (DESC_TYPE_V_REFS << VECTOR_INFO_SHIFT)
#define VECTOR_SUBTYPE_BITMAP  (DESC_TYPE_V_BITMAP << VECTOR_INFO_SHIFT)

#define VECTOR_KIND_SZARRAY  (DESC_TYPE_V_SZARRAY << VECTOR_KIND_SHIFT)
#define VECTOR_KIND_ARRAY  (DESC_TYPE_V_ARRAY << VECTOR_KIND_SHIFT)

/*
 * Objects are aligned to 8 bytes boundaries.
 *
 * A descriptor is a pointer in MonoVTable, so 32 or 64 bits of size.
 * The low 3 bits define the type of the descriptor. The other bits
 * depend on the type.
 *
 * It's important to be able to quickly identify two properties of classes from their
 * descriptors: whether they are small enough to live in the regular major heap (size <=
 * SGEN_MAX_SMALL_OBJ_SIZE), and whether they contain references.
 *
 * To that end we have three descriptor types that only apply to small classes: RUN_LENGTH,
 * BITMAP, and SMALL_PTRFREE.  We also have the type COMPLEX_PTRFREE, which applies to
 * classes that are either not small or of unknown size (those being strings and arrays).
 * The lowest two bits of the SMALL_PTRFREE and COMPLEX_PTRFREE tags are the same, so we can
 * quickly check for references.
 *
 * As a general rule the 13 remaining low bits define the size, either
 * of the whole object or of the elements in the arrays. While for objects
 * the size is already in bytes, for arrays we need to shift, because
 * array elements might be smaller than 8 bytes. In case of arrays, we
 * use two bits to describe what the additional high bits represents,
 * so the default behaviour can handle element sizes less than 2048 bytes.
 * The high 16 bits, if 0 it means the object is pointer-free.
 * This design should make it easy and fast to skip over ptr-free data.
 * The first 4 types should cover >95% of the objects.
 * Note that since the size of objects is limited to 64K, larger objects
 * will be allocated in the large object heap.
 * If we want 4-bytes alignment, we need to put vector and small bitmap
 * inside complex.
 *
 * We don't use 0 so that 0 isn't a valid GC descriptor.  No deep reason for this other than
 * to be able to identify a non-inited descriptor for debugging.
 */
enum {
  /* Keep in sync with `descriptor_types` in sgen-debug.c! */
  DESC_TYPE_RUN_LENGTH = 1,   /* 16 bits aligned byte size | 1-3 (offset, numptr) bytes tuples */
  DESC_TYPE_BITMAP = 2,	    /* | 29-61 bitmap bits */
  DESC_TYPE_SMALL_PTRFREE = 3,
  DESC_TYPE_MAX_SMALL_OBJ = 3,
  DESC_TYPE_COMPLEX = 4,      /* index for bitmap into complex_descriptors */
  DESC_TYPE_VECTOR = 5,       /* 10 bits element size | 1 bit kind | 2 bits desc | element desc */
  DESC_TYPE_COMPLEX_ARR = 6,  /* index for bitmap into complex_descriptors */
  DESC_TYPE_COMPLEX_PTRFREE = 7, /* Nothing, used to encode large ptr objects and strings. */
  DESC_TYPE_MAX = 7,

  DESC_TYPE_PTRFREE_MASK = 3,
  DESC_TYPE_PTRFREE_BITS = 3
};

/* values for array kind */
enum {
  DESC_TYPE_V_SZARRAY = 0, /*vector with no bounds data */
  DESC_TYPE_V_ARRAY = 1, /* array with bounds data */
};

/* subtypes for arrays and vectors */
enum {
  DESC_TYPE_V_PTRFREE = 0,/* there are no refs: keep first so it has a zero value  */
  DESC_TYPE_V_REFS,       /* all the array elements are refs */
  DESC_TYPE_V_RUN_LEN,    /* elements are run-length encoded as DESC_TYPE_RUN_LENGTH */
  DESC_TYPE_V_BITMAP      /* elements are as the bitmap in DESC_TYPE_SMALL_BITMAP */
};

#define CORGC_DESC_STRING	(DESC_TYPE_COMPLEX_PTRFREE | (1 << LOW_TYPE_BITS))

/* Root bitmap descriptors are simpler: the lower three bits describe the type
 * and we either have 30/62 bitmap bits or nibble-based run-length,
 * or a complex descriptor, or a user defined marker function.
 */
enum {
  ROOT_DESC_CONSERVATIVE, /* 0, so matches NULL value */
  ROOT_DESC_BITMAP,
  ROOT_DESC_RUN_LEN, 
  ROOT_DESC_COMPLEX,
  ROOT_DESC_USER,
  ROOT_DESC_TYPE_MASK = 0x7,
  ROOT_DESC_TYPE_SHIFT = 3,
};

extern "C" gsize* corgc_get_complex_descriptor (mword desc);
extern "C" void* corgc_get_complex_descriptor_bitmap (mword desc);
extern "C" MonoGCRootMarkFunc corgc_get_user_descriptor_func (mword desc);

extern "C" void corgc_init_descriptors (void);

#endif
