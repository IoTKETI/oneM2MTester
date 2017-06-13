/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "memory.h"

#undef Malloc
#undef Realloc
#undef Free
#undef mprintf
#undef mprintf_va_list
#undef mputprintf
#undef mputprintf_va_list
#undef memptystr
#undef mcopystr
#undef mcopystrn
#undef mputstr
#undef mputc


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

/** @def va_copy
 * work-around for missing va_copy() in GCC
 */
#if defined(__GNUC__) && !defined(va_copy)
# ifdef __va_copy
#  define va_copy(dest, src) __va_copy(dest, src)
# else
#  define va_copy(dest, src) (dest) = (src)
# endif
#endif

/** Initial buffer size for mprintf */
#define BUFSIZE 1024

/** @def MEMORY_DEBUG_ADDRESS
 *
 * If you want to catch memory management functions (malloc, realloc,
 * free) which operate on a given address, define this macro and set
 * memory_debug_address to the address you want to monitor.
 *
 * @note This macro has no effect if \c MEMORY_DEBUG is not defined
 */
#ifdef DOXYGEN_SPECIAL
/* Make the macro visible to Doxygen */
#define MEMORY_DEBUG_ADDRESS
#endif

#ifdef MEMORY_DEBUG_ADDRESS
/** @brief Memory checkpoint address. */
void *memory_debug_address = NULL;

/** @brief Memory checkpoint ordinal. */
size_t memory_debug_ordinal = (size_t)-1;
#endif

/**
 *  @name Number of memory allocations and frees performed.
 *  @{
 *  @note These are tracked even without MEMORY_DEBUG.
 */
static size_t malloc_count = 0, free_count = 0;
/** @} */

/**
 * If preprocessor symbol \c MEMORY_DEBUG is defined the memory
 * management routines do some self-checks against improper use. This
 * implies extra memory usage and significant performance penalty.
 */
#ifdef MEMORY_DEBUG

#define FILENAME_FORMAT "%s:%d: "

/* Currently allocated memory amount, peak, and number of allocations */
static size_t allocated_size = 0, peak_size = 0;
static unsigned int alloc_count = 0;
/* Sum of all the allocated amounts */
static unsigned long long total_allocated = 0;

typedef struct memory_block_struct {
    size_t size;
    struct memory_block_struct *prev, *next;
    const char *filename; /* pointer to string literal */
    int lineno;
    unsigned int ordinal; /* The ordinal which uniquely identifies this allocation */
    void *magic; /* only to reserve some place for the magic guard */
    double begin; /* beginning of useful memory with proper alignment */
} memory_block;

static memory_block *list_head = NULL, *list_tail = NULL;

static void add_to_list(memory_block *block_ptr)
{
    block_ptr->prev = list_tail;
    block_ptr->next = NULL;
    if (list_tail != NULL) list_tail->next = block_ptr;
    else list_head = block_ptr;
    list_tail = block_ptr;
}

static void remove_from_list(memory_block *block_ptr)
{
    if (block_ptr->prev != NULL)
	block_ptr->prev->next = block_ptr->next;
    else list_head = block_ptr->next;
    if (block_ptr->next != NULL)
	block_ptr->next->prev = block_ptr->prev;
    else list_tail = block_ptr->prev;
}

/* Layout of memory (allocated in one chunk)
 * +----------+ <--- block_ptr
 * | size     |
 * +----------+
 * | prev     |
 * +----------+
 * | next     |
 * +----------+
 * | filename |
 * +----------+
 * | ordinal  |
 * +----------+
 * | magic    |
 * +----------+  +-----------+
 * | begin    |  | user data |
 * +----------+  |           |
 *                ...........
 *               |           |
 * +-----------+ +-----------+
 * | end magic |
 * +-----------+
 *
 *
 * The magic values initially contain the value of block_ptr.
 * When the block is freed, a bit-flipped value is written instead.
 * During realloc and free, the magic values are checked to match block_ptr.
 * If they don't, the a memory overrun or underrun has occurred
 * (except if the magic value happens to match the bit-flipped block_ptr,
 * in which case it's likely a double-free).
 */

static void set_magic_values(memory_block *block_ptr)
{
    unsigned char *ptr = (unsigned char*)&block_ptr->begin;
    memcpy(&block_ptr->magic, &block_ptr, sizeof(block_ptr));
    memcpy(ptr + block_ptr->size, &block_ptr, sizeof(block_ptr));
}

static void check_magic_values(const char *filename, int line,
  memory_block *block_ptr, int is_realloc)
{
    void *inv_ptr = (void*)(~(size_t)block_ptr);
    unsigned char *ptr = (unsigned char*)&block_ptr->begin;
    /* compare the magic */
    if (memcmp(&block_ptr->magic, &block_ptr, sizeof(block_ptr))) {
        /* mismatch! */
	const char *err_str;
	if (memcmp(&block_ptr->magic, &inv_ptr, sizeof(inv_ptr)))
	    err_str = "memory corruption";
	else err_str = "duplicate free/realloc";
	if (filename) {
	    fprintf(stderr, FILENAME_FORMAT, filename, line);
	}
	fprintf(stderr, "Fatal error: %s detected at block begin when %s "
	    "pointer %p.\n", err_str, is_realloc ? "reallocating" : "freeing",
	    ptr);
	if (block_ptr->filename) fprintf(stderr,
	  FILENAME_FORMAT "Last freed here.\n", block_ptr->filename, block_ptr->lineno);
	abort();
    }
    memcpy(&block_ptr->magic, &inv_ptr, sizeof(inv_ptr));/*invert magic*/
    if (memcmp(ptr + block_ptr->size, &block_ptr, sizeof(block_ptr))) {
        if (filename) {
            fprintf(stderr, FILENAME_FORMAT, filename, line);
        }
	fprintf(stderr, "Fatal error: memory corruption detected "
	    "at block end when %s pointer %p.\n",
	    is_realloc ? "reallocating" : "freeing", ptr);
        if (block_ptr->filename) fprintf(stderr,
          FILENAME_FORMAT "Last freed here.\n", block_ptr->filename, block_ptr->lineno);
	abort();
    }
    memcpy(ptr + block_ptr->size, &inv_ptr, sizeof(inv_ptr));
    block_ptr->filename = filename;
    block_ptr->lineno   = line;
}

/** @def MEMORY_DEBUG_FREE
 *  @brief Enable checking for uses of unallocated/deallocated memory areas.
 *
 * If preprocessor symbol \c MEMORY_DEBUG_FREE is defined the memory
 * management routines can verify that unused and deallocated memory areas
 * are not written accidentally by the program after calling \a Free().
 * This verification can be done using functions \a check_mem_leak() or
 * \a check_mem_corrupt().
 *
 * Note: This functionality can significantly increase the memory requirements
 * of the program since the deallocated blocks cannot be recycled. They are
 * not returned to the system using \a free() until the end of the program run.
 */
#ifdef DOXYGEN_SPECIAL
/* Make the macro visible to Doxygen */
#define MEMORY_DEBUG_FREE
#endif

#ifdef MEMORY_DEBUG_FREE

static memory_block *free_list_head = NULL, *free_list_tail = NULL;

static void add_to_free_list(memory_block *block_ptr)
{
    block_ptr->prev = free_list_tail;
    block_ptr->next = NULL;
    if (free_list_tail != NULL) free_list_tail->next = block_ptr;
    else free_list_head = block_ptr;
    free_list_tail = block_ptr;
}

static void check_free_list(void)
{
    memory_block *block_ptr = free_list_head;
    size_t counter = 0;
    while (block_ptr != NULL) {
	void *inv_ptr = (void*)(~(size_t)block_ptr);
	unsigned char *ptr = (unsigned char*)&block_ptr->begin;
	size_t i;
	if (memcmp(&block_ptr->magic, &inv_ptr, sizeof(inv_ptr))) {
	    fprintf(stderr, "Fatal error: memory corruption detected in front "
		"of freed pointer %p\n", ptr);
	    abort();
	}
	for (i = 0; i < block_ptr->size; i++) {
	    if (ptr[i] != 0xAA) {
		fprintf(stderr, "Fatal error: memory overwriting detected in "
		    "deallocated area: base pointer: %p, offset: %u, data "
		    "written: %02X\n", ptr, i, ptr[i]);
		abort();
	    }
	}
	if (memcmp(ptr + block_ptr->size, &inv_ptr, sizeof(inv_ptr))) {
	    fprintf(stderr, "Fatal error: memory corruption detected at the "
		"end of freed pointer %p\n", ptr);
	    abort();
	}
	block_ptr = block_ptr->next;
        counter++;
    }
    fprintf(stderr, "%u deallocated memory block%s OK\n", counter,
	counter > 1 ? "s are" : " is");
}

static void release_free_blocks(void)
{
    memory_block *block_ptr = free_list_head;
    while (block_ptr != NULL) {
	memory_block *next_ptr = block_ptr->next;
	free(block_ptr);
	block_ptr = next_ptr;
    }
    free_list_head = NULL;
    free_list_tail = NULL;
}
#endif
/* MEMORY_DEBUG_FREE */

#ifdef MEMORY_DEBUG_ADDRESS
/** Check the address and the ordinal of the allocation against the checkpoints
 *
 * If the address or the ordinal matches, a line is printed to the standard
 * error. Breakpoints can be set on the printf to trigger when the watched
 * allocation happens.
 *
 * @param block_ptr pointer to a memory block structure which is being
 * allocated/reallocated/freed
 * @param oper the actual operation: 0=Malloc, 1=Realloc, 2=Free
 */
static void check_memory_address(memory_block *block_ptr, int oper)
{
  void *ptr = (unsigned char*)&block_ptr->begin;
  if (ptr == memory_debug_address)
  {
    if (block_ptr->filename) {
      fprintf(stderr, FILENAME_FORMAT, block_ptr->filename, block_ptr->lineno);
    }
    fprintf(stderr, "MEMDEBUG: returning pointer %p while %sing memory.\n",
            ptr, oper==0?"allocat":oper==1?"reallocat":"free");
  }
  if (block_ptr->ordinal == memory_debug_ordinal) {
    if (block_ptr->filename) {
      fprintf(stderr, FILENAME_FORMAT, block_ptr->filename, block_ptr->lineno);
    }
    fprintf(stderr, "MEMDEBUG: returning ordinal %lu while %sing memory.\n",
            (unsigned long)block_ptr->ordinal,
            oper==0 ? "allocat" : (oper==1 ? "reallocat" : "free"));
  }
}
#endif
/* MEMORY_DEBUG_ADDRESS */

/*#define FATAL_ERROR_INTERNAL(f,l,s) fatal_error(f,l,s)*/
#define MALLOC_INTERNAL(f,l,s) Malloc_dbg(f,l,s)
#define MEMPTYSTR_INTERNAL(f,l) memptystr_dbg(f,l)
#define MCOPYSTR_INTERNAL(f,l,s) mcopystr_dbg(f,l,s)
#define MCOPYSTRN_INTERNAL(f,l,s,l2) mcopystrn_dbg(f,l,s,l2)
#define REALLOC_INTERNAL(f,l,p,s) Realloc_dbg(f,l,p,s)
#define FREE_INTERNAL(f,l,p) Free_dbg(f,l,p)
#define MPRINTF_VA_LIST_INTERNAL(f,l,s,p) mprintf_va_list_dbg(f,l,s,p)

static const size_t offset = (unsigned char*)&(((memory_block*)NULL)->begin)
  - (unsigned char*)NULL;

static void extract_location(void *p, const char **fn, int *ln)
{
    memory_block *block_ptr = NULL;
    block_ptr = (memory_block*)(void*)((unsigned char*)p - offset);
    *fn = block_ptr->filename;
    *ln = block_ptr->lineno;
}

#else
/* not MEMORY_DEBUG */

#define FATAL_ERROR_INTERNAL(f,l,s) fatal_error(s)
#define MALLOC_INTERNAL(f,l,s) Malloc(s)
#define MEMPTYSTR_INTERNAL(f,l) memptystr()
#define MCOPYSTR_INTERNAL(f,l,s) mcopystr(s)
#define MCOPYSTRN_INTERNAL(f,l,s,l2) mcopystrn(s,l2)
#define REALLOC_INTERNAL(f,l,p,s) Realloc(p,s)
#define FREE_INTERNAL(f,l,p) Free(p)
#define MPRINTF_VA_LIST_INTERNAL(f,l,s,p) mprintf_va_list(s,p)

#define extract_location(p,f,l) ((void)p,(void)f,(void)l)
#endif

/** Report a fatal error.
 *
 * @param size the number of bytes that could not be allocated
 *
 * This function does not return.
 */
__attribute__ ((__noreturn__))
static void fatal_error(
#ifdef MEMORY_DEBUG
  const char *filename, int line,
#endif
  size_t size)
{
    const char *err_msg = strerror(errno);
#ifdef MEMORY_DEBUG
    fprintf(stderr, FILENAME_FORMAT "Fatal error: cannot allocate %lu bytes"
        " of memory after allocating %lu bytes: ", filename, line,
        (unsigned long) size, (unsigned long) allocated_size);
#else
    fprintf(stderr, "Fatal error: cannot allocate %lu bytes of memory: ",
        (unsigned long) size);
#endif
    if (err_msg != NULL) fprintf(stderr, "%s. Exiting.\n", err_msg);
    else fprintf(stderr, "Unknown error (errno: %d). Exiting.\n", errno);
    exit(EXIT_FAILURE);
}

#ifdef MEMORY_DEBUG
void *Malloc(size_t size)
{
  return Malloc_dbg(0,0,size);
}

void *Malloc_dbg(const char *filename, int line, size_t size)
#else
void *Malloc(size_t size)
#endif
{
    if (size > 0) {
	void *ptr;
#ifdef MEMORY_DEBUG
	memory_block *block_ptr;
	block_ptr = (memory_block*)malloc(sizeof(*block_ptr) -
	    sizeof(block_ptr->begin) + size + sizeof(block_ptr));
	if (block_ptr == NULL) fatal_error(filename, line, size);
        block_ptr->filename = filename;
        block_ptr->lineno = line;
	block_ptr->size = size;
	add_to_list(block_ptr);
	set_magic_values(block_ptr);
	ptr = &block_ptr->begin;
	total_allocated += size;
	block_ptr->ordinal = alloc_count++;
	allocated_size += size;
#ifdef MEMORY_DEBUG_ADDRESS
        check_memory_address(block_ptr, 0);
#endif
	if (peak_size < allocated_size) peak_size = allocated_size;
#else
	ptr = malloc(size);
	if (ptr == NULL) fatal_error(size);
#endif
	malloc_count++;
	return ptr;
    } else return NULL;
}

#ifdef MEMORY_DEBUG

void *Realloc(void *ptr, size_t size)
{
  return Realloc_dbg(0,0,ptr,size);
}

void *Realloc_dbg(const char *filename, int line, void *ptr, size_t size)
#else
void *Realloc(void *ptr, size_t size)
#endif
{
    if (ptr == NULL) return MALLOC_INTERNAL(filename, line, size);
    else if (size == 0) {
	FREE_INTERNAL(filename, line, ptr);
	return NULL;
    } else {
	void *new_ptr;
#ifdef MEMORY_DEBUG
	memory_block *block_ptr = NULL;
	block_ptr = (memory_block*)(void*)((unsigned char*)ptr - offset);
	check_magic_values(filename, line, block_ptr, 1);
	remove_from_list(block_ptr);
	allocated_size -= block_ptr->size;
	if (size < block_ptr->size)
	    memset((unsigned char*)ptr + size, 0x55, block_ptr->size - size);
	block_ptr = (memory_block*)realloc(block_ptr, sizeof(*block_ptr) -
	    sizeof(block_ptr->begin) + sizeof(block_ptr) + size);
	if (block_ptr == NULL) fatal_error(filename, line, size);
#ifdef MEMORY_DEBUG_ADDRESS
        check_memory_address(block_ptr, 1);
#endif
        block_ptr->filename = filename;
        block_ptr->lineno = line;
	block_ptr->size = size;
	add_to_list(block_ptr);
	set_magic_values(block_ptr);
	new_ptr = &block_ptr->begin;
	total_allocated += size;
	alloc_count++;
	allocated_size += size;
	if (peak_size < allocated_size) peak_size = allocated_size;
#else
	new_ptr = realloc(ptr, size);
	if (new_ptr == NULL) FATAL_ERROR_INTERNAL(filename, line, size);
#endif
	return new_ptr;
    }
}

#ifdef MEMORY_DEBUG
void Free(void *ptr)
{
  Free_dbg(0,0,ptr);
}

extern void Free_dbg(const char *filename, int line, void *ptr)
#else
void Free(void *ptr)
#endif
{
    if (ptr != NULL) {
#ifdef MEMORY_DEBUG
	memory_block *block_ptr = NULL;
	block_ptr = (memory_block*)(void*)((unsigned char*)ptr - offset);
#ifdef MEMORY_DEBUG_ADDRESS
        check_memory_address(block_ptr, 2);
#endif
	check_magic_values(filename, line, block_ptr, 0);
	remove_from_list(block_ptr);
	allocated_size -= block_ptr->size;
	memset(ptr, 0xAA, block_ptr->size);
#ifdef MEMORY_DEBUG_FREE
	add_to_free_list(block_ptr);
#else
	free(block_ptr);
#endif
#else
	free(ptr);
#endif
	free_count++;
    }
}

#ifdef MEMORY_DEBUG
static const size_t maxprint = 32;
#endif

void check_mem_leak(const char *program_name)
{
#ifdef MEMORY_DEBUG
    fprintf(stderr, "%s(%d): memory usage statistics:\n"
	"total allocations: %lu\n"
	"malloc/new  calls: %lu\n"
	"free/delete calls: %lu\n"
	"peak memory usage: %lu bytes\n"
	"average block size: %g bytes\n",
	program_name, (int)getpid(),
	(unsigned long)alloc_count, (unsigned long)malloc_count,
        (unsigned long) free_count, (unsigned long) peak_size,
	(double)total_allocated / (double)alloc_count);
    if (list_head != NULL) {
	memory_block *block_ptr = list_head;
	size_t counter = 0;
	fprintf(stderr, "unallocated blocks:\n");
	do {
	    if (block_ptr->filename != 0) {
	    	fprintf(stderr, FILENAME_FORMAT,
	    	    block_ptr->filename, block_ptr->lineno);
	    }
	    fprintf(stderr, "\tMemory leak at %p, size %lu, ord %lu: ",
		(void*)&block_ptr->begin, (unsigned long)block_ptr->size,
		(unsigned long)block_ptr->ordinal);
	    {
	      const unsigned char * mm = (const unsigned char*)&block_ptr->begin;
	      size_t x, limit = (block_ptr->size > maxprint) ? maxprint
	        : block_ptr->size;
	      for (x = 0; x < limit; ++x) {
		fputc( isprint(mm[x]) ? mm[x] : '.', stderr );
	      }
	      fputc(10, stderr);
	    }
	    block_ptr = block_ptr->next;
	    counter++;
	} while (block_ptr != NULL);
	fprintf(stderr, "total unallocated: %lu bytes in %lu blocks\n",
	    (unsigned long) allocated_size, (unsigned long) counter);
    }
#ifdef MEMORY_DEBUG_FREE
    check_free_list();
    release_free_blocks();
#endif
#else
    if (malloc_count != free_count) {
	fprintf(stderr, "%s: warning: memory leakage detected.\n"
	    "Total malloc calls: %lu, free calls: %lu\n"
	    "Please submit a bug report including the current input file(s).\n",
	    program_name,
            (unsigned long) malloc_count, (unsigned long) free_count);
    }
#endif
}

#ifdef MEMORY_DEBUG
void check_mem_corrupt(const char *program_name)
{
    size_t counter = 0;
    memory_block *block_ptr;
    fprintf(stderr, "%s: checking memory blocks for corruption\n",
	program_name);
    for (block_ptr = list_head; block_ptr != NULL; block_ptr = block_ptr->next)
    {
	unsigned char *ptr = (unsigned char*)&block_ptr->begin;
	if (memcmp(ptr - sizeof(block_ptr), &block_ptr, sizeof(block_ptr))) {
	    fprintf(stderr, "Fatal error: memory corruption detected in front "
		"of pointer %p\n", ptr);
	    abort();
	}
	if (memcmp(ptr + block_ptr->size, &block_ptr, sizeof(block_ptr))) {
	    fprintf(stderr, "Fatal error: memory corruption detected at the "
		"end of pointer %p\n", ptr);
	    abort();
	}
        counter++;
    }
    fprintf(stderr, "%s: %lu memory block%s OK\n", program_name,
	(unsigned long) counter, counter > 1 ? "s are" : " is");
#ifdef MEMORY_DEBUG_FREE
    check_free_list();
#endif
}
#endif

/** @brief Find the string length of an expstring_t

Finds the length of the C string pointed at by \p str, using some assumptions
satisfied by an expstring_t.

@pre There is an offset from the beginning of the string which is a power
     of two, is within the buffer allocated for \p str and it contains '\\0'
@pre The allocated buffer is at most twice as long as strlen(str)
@pre \p bufsize is a valid pointer

@param [in]  str an expstring_t, must not be null
@param [out] bufsize pointer to a location where the minimum buffer size
             (always a power of two) is deposited.
@return      the length of \p str as a C string
*/
static size_t fast_strlen(const expstring_t str, size_t *bufsize)
{
    size_t size, min, max;
    /* Find the 0 byte at the end of the allocated buffer */
    for (size = 1; str[size - 1] != '\0'; size *= 2) {}
    *bufsize = size;
    if (size == 1) return 0;
    /* Binary search the second half of the buffer */
    min = size / 2 - 1;
    max = size - 1;
    while (max - min > 1) {
	size_t med = (min + max) / 2;
	if (str[med] != '\0') min = med;
	else max = med;
    }
    return max;
}

/** @brief Find the first power of two which is greater than \p len
 *
 *  @param len the desired length
 *  @return a power of 2 greater than \p len
 *
 *  \p len is taken to be the number of characters needed. The returned value
 *  will always be greater than \p len to accommodate the null terminator.
 */
static size_t roundup_size(size_t len)
{
    size_t size;
    for (size = 1; len >= size; size *= 2);
    return size;
}

#ifdef MEMORY_DEBUG
expstring_t mprintf_va_list(const char *fmt, va_list pvar)
{
  return mprintf_va_list_dbg(0,0,fmt,pvar);
}

expstring_t mprintf_va_list_dbg(const char *filename, int line, const char *fmt, va_list pvar)
#else
expstring_t mprintf_va_list(const char *fmt, va_list pvar)
#endif
{
    char buf[BUFSIZE];
    expstring_t ptr;
    int len;
    size_t size, slen;
    va_list pvar2;
    va_copy(pvar2, pvar);
    len = vsnprintf(buf, BUFSIZE, fmt, pvar2);
    va_end(pvar2);
    if (len < 0) {
	/* The result does not fit in buf and we have no idea how many bytes
	 * are needed (only old versions of libc may return -1).
	 * Try to double the buffer size until it is large enough. */
	for (size = 2 * BUFSIZE; ; size *= 2) {
	    ptr = (expstring_t)MALLOC_INTERNAL(filename, line, size);
	    va_copy(pvar2, pvar);
	    len = vsnprintf(ptr, size, fmt, pvar2);
	    va_end(pvar2);
	    if (len >= 0 && (size_t)len < size) break;
	    FREE_INTERNAL(filename, line, ptr);
	}
	slen = (size_t)len;
    } else if (len >= BUFSIZE) {
	/* The result does not fit in buf, but we know how many bytes we need.
	 * Allocate a buffer that is large enough and repeat vsnprintf() */
        slen = (size_t)len;
	size = roundup_size(slen);
	ptr = (expstring_t)MALLOC_INTERNAL(filename, line, size);
	/* use the original pvar since this is the last try */
	if (vsnprintf(ptr, size, fmt, pvar) != len) {
	    perror("Fatal error: unexpected vsnprintf() return value");
	    exit(EXIT_FAILURE);
	}
    } else {
	/* the complete result is in buf */
        slen = (size_t)len;
	size = roundup_size(slen);
	ptr = (expstring_t)MALLOC_INTERNAL(filename, line, size);
	memcpy(ptr, buf, slen);
    }
    memset(ptr + slen, '\0', size - slen);
    return ptr;
}

#ifdef MEMORY_DEBUG
expstring_t mprintf_dbg(const char *filename, int line, const char *fmt, ...)
{
    expstring_t ptr;
    va_list pvar;
    va_start(pvar, fmt);
    ptr = mprintf_va_list_dbg(filename, line, fmt, pvar);
    va_end(pvar);
    return ptr;
}
#endif

expstring_t mprintf(const char *fmt, ...)
{
    expstring_t ptr;
    va_list pvar;
    va_start(pvar, fmt);
    ptr = mprintf_va_list(fmt, pvar);
    va_end(pvar);
    return ptr;
}

/* Tracking the origin: extracts the filename/line from the original allocation
 * of the expstring_t. */
expstring_t mputprintf_va_list(expstring_t str, const char *fmt, va_list pvar)
{
    const char *filename = NULL;
    int line = 0;
    if (str != NULL) {
	size_t size;
	size_t len = fast_strlen(str, &size);
	size_t rest = size - len;
	int len2;
	va_list pvar2;
	/* make a copy of pvar to allow re-use later */
	va_copy(pvar2, pvar);
	len2 = vsnprintf(str + len, rest, fmt, pvar2);
	va_end(pvar2);
	extract_location(str, &filename, &line);
	if (len2 < 0) {
	    /* the result does not fit in buf and we have no idea how many
	     * bytes are needed (only old versions of libc may return -1)
	     * try to double the buffer size until it is large enough */
	    size_t newlen;
	    do {
		size *= 2;
		str = (expstring_t)REALLOC_INTERNAL(filename, line, str, size);
		rest = size - len;
		va_copy(pvar2, pvar);
		len2 = vsnprintf(str + len, rest, fmt, pvar2);
		va_end(pvar2);
	    } while (len2 < 0 || (size_t)len2 >= rest);
	    newlen = len + (size_t)len2;
	    memset(str + newlen, '\0', size - newlen);
	} else if ((size_t)len2 >= rest) {
	    /* there isn't enough space in buffer, but we know how many bytes
	     * we need: reallocate the buffer to be large enough and repeat
	     * vsnprintf() */
	    size_t newlen = len + (size_t)len2;
	    size = roundup_size(newlen);
	    str = (expstring_t)REALLOC_INTERNAL(filename, line, str, size);
	    /* use the original pvar since this is the last try */
	    if (vsnprintf(str + len, size - len, fmt, pvar) != len2) {
		perror("Fatal error: unexpected vsnprintf() return value");
		exit(EXIT_FAILURE);
	    }
	    memset(str + newlen, '\0', size - newlen);
	}
    } else str = MPRINTF_VA_LIST_INTERNAL(filename, line, fmt, pvar);
    return str;
}

expstring_t mputprintf(expstring_t str, const char *fmt, ...)
{
    va_list pvar;
    va_start(pvar, fmt);
    str = mputprintf_va_list(str, fmt, pvar);
    va_end(pvar);
    return str;
}

#ifdef MEMORY_DEBUG
expstring_t memptystr(void)
{
  return memptystr_dbg(0,0);
}

expstring_t memptystr_dbg(const char *filename, int line)
#else
expstring_t memptystr(void)
#endif
{
    expstring_t ptr = (expstring_t)MALLOC_INTERNAL(filename, line, 1);
    ptr[0] = '\0';
    return ptr;
}

#ifdef MEMORY_DEBUG
expstring_t mcopystr(const char *str)
{
  return mcopystr_dbg(0,0,str);
}

expstring_t mcopystr_dbg(const char *filename, int line, const char *str)
#else
expstring_t mcopystr(const char *str)
#endif
{
    if (str != NULL) {
	size_t len = strlen(str);
	size_t size = roundup_size(len);
	expstring_t ptr = (expstring_t)MALLOC_INTERNAL(filename, line, size);
	memcpy(ptr, str, len);
	memset(ptr + len, '\0', size - len);
	return ptr;
    } else return MEMPTYSTR_INTERNAL(filename, line);
}

#ifdef MEMORY_DEBUG
expstring_t mcopystrn(const char *str, size_t len)
{
  return mcopystrn_dbg(0,0,str, len);
}

expstring_t mcopystrn_dbg(const char *filename, int line, const char *str,
  size_t len)
#else
expstring_t mcopystrn(const char *str, size_t len)
#endif
{
    if (len != 0 && str != NULL) {
        size_t size = roundup_size(len);
        expstring_t ptr = (expstring_t)MALLOC_INTERNAL(filename, line, size);
        memcpy(ptr, str, len);
        memset(ptr + len, '\0', size - len);
        return ptr;
    } else return MEMPTYSTR_INTERNAL(filename, line);
}

/* Tracking the origin: extracts the filename/line from the original allocation
 * of the expstring_t. */
expstring_t mputstr(expstring_t str, const char *str2)
{
    const char *filename = NULL;
    int line = 0;
    if (str2 != NULL) {
	if (str != NULL) {
	    size_t size;
	    size_t len = fast_strlen(str, &size);
	    size_t len2 = strlen(str2);
	    size_t newlen = len + len2;
	    if (size <= newlen) {
		size_t newsize = roundup_size(newlen);
		extract_location(str, &filename, &line);
		str = (expstring_t)REALLOC_INTERNAL(filename, line, str, newsize);
		memset(str + newlen, '\0', newsize - newlen);
	    }
	    memcpy(str + len, str2, len2);
	} else str = MCOPYSTR_INTERNAL(filename, line, str2);
    }
    return str;
}

/* Tracking the origin: extracts the filename/line from the original allocation
 * of the expstring_t. */
expstring_t mputstrn(expstring_t str, const char *str2, size_t len2)
{
    const char *filename = NULL;
    int line = 0;
    if (len2 != 0 && str2 != NULL) {
        if (str != NULL) {
            size_t size;
            size_t len = fast_strlen(str, &size);
            size_t newlen = len + len2;
            if (size <= newlen) {
                size_t newsize = roundup_size(newlen);
                extract_location(str, &filename, &line);
                str = (expstring_t)REALLOC_INTERNAL(filename, line, str, newsize);
                memset(str + newlen, '\0', newsize - newlen);
            }
            memcpy(str + len, str2, len2);
        } else str = MCOPYSTRN_INTERNAL(filename, line, str2, len2);
    }
    return str;
}

/* Tracking the origin: extracts the filename/line from the original allocation
 * of the expstring_t. */
expstring_t mputc(expstring_t str, char c)
{
    const char *filename = NULL;
    int line = 0;
    if (str != NULL) {
	if (c != '\0') {
	    size_t size;
	    size_t len = fast_strlen(str, &size);
	    if (size <= len + 1) {
	        extract_location(str, &filename, &line);
                str = (expstring_t)REALLOC_INTERNAL(filename, line, str, 2 * size);
		memset(str + size, '\0', size);
	    }
	    str[len] = c;
	}
    } else {
	if (c != '\0') {
	    str = (expstring_t)MALLOC_INTERNAL(filename, line, 2);
	    str[0] = c;
	    str[1] = '\0';
	} else str = MEMPTYSTR_INTERNAL(filename, line);
    }
    return str;
}

/* Tracking the origin: extracts the filename/line from the original allocation
 * of the expstring_t. */
expstring_t mtruncstr(expstring_t str, size_t newlen)
{
  const char *filename = NULL;
  int line = 0;
  if (str != NULL) {
    size_t size;
    size_t len = fast_strlen(str, &size);
    if (newlen < len) {
      size_t newsize = roundup_size(newlen);
      if (newsize < size) {
        extract_location(str, &filename, &line);
	str = (expstring_t)REALLOC_INTERNAL(filename, line, str, newsize);
      }
      memset(str + newlen, '\0', newsize - newlen);
    }
  }
  return str;
}

size_t mstrlen(const expstring_t str)
{
  if (str != NULL) {
    size_t size;
    return fast_strlen(str, &size);
  } else return 0;
}

char * buildstr(unsigned int b) {
  if (b > 99) return NULL; /* invalid */
  if (b == 99) return memptystr(); /* empty string for full version */
  return mprintf("%02d", b);
}
