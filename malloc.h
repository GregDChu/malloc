#include <stdio.h>
#include <stdint.h>

#ifndef MYMALLOC_HEADER
#define MYMALLOC_HEADER

/* Allocates memory of a given size
 *  size - size of memory to allocate
 * Returns a pointer to the start of
 * the allocated memory or NULL if none
 * was allocated.
 */
extern void *malloc(size_t size);

/* Frees the chunk of memory pointed
 * to by a pointer.
 *  ptr - a pointer to the chunk of 
 *        memory to free
 * Returns nothing
 */
extern void free(void *ptr);

/* Allocates memory of a given size
 * and initializes all bytes to 0
 *  nmemb - the number of elements
 *  size - the size in bytes of each
 *         element
 * Returns a pointer to the start of
 * the allocated memory or NULL if no
 * memory was allocated
 */
extern void *calloc(size_t nmemb, size_t size);

/* Adjusts the size of allocated
 * memory.
 *  ptr - a pointer to the chunk of
 *        memory to reallocate
 *  size - the new size of memory
 *         in bytes
 * Returns a pointer to the adjusted
 * allocated memory or NULL if nothing
 * happened.
 *
 */
extern void *realloc(void *ptr, size_t size);

/* A data structure containing information
 * about each chunk of available memory
 *  mem_start - start address of available memory
 *  mem_siz - the size of available memory
 *  status - the availability status (FREE, INUSE)
 *  prev - the previous Header
 *  next - the next Header
 */
typedef struct Header Header;

/* Extends the amount of working memory 
 * available to the program.
 *  headptr - a pointer to a Header pointer
 *            where the available memory's
 *            Header will be stored
 *  size - the minimum amount of memory being
 *         asked for
 * Returns 0 on success and -1 on failure
 * Sets errno to ENOMEM on failure
 */
static int getmem(Header **headptr, int size);

/* Formats a chunk of raw memory so
 * that it has a header attached to it.
 *  headptr - a pointer to a Header pointer
 *            where the available memory's
 *            Header will be stored
 *  memstart - a pointer to the starting
 *             address of raw memory
 *  size - the size in bytes of the raw memory
 * Returns nothing
 */
static void formatmem(Header **headptr, void *memstart, int size);

/* Divides a chunk of memory,
 * giving each a unique header.
 *  header - a pointer to the Header
 *           of the memory to divide
 *  size - the desired size of one of the two
 *  chunks of the divided memory
 * Returns a pointer to the Header of the
 * chunk of memory of size
*/
static Header *divmem(Header *header, int size);

/* Merges all free chunks of
 * memory before and after a given chunk.
 * header - a pointer to the header of the
 *          chunk to merge with its adjacent
 *          chunks of memory
 * Returns nothing
*/
static void mergemem(Header *header);

/* Expands a block of memory either
 * by expansion in place or relocation.
 *  header - a pointer to the header of
 *           the chunk of memory to expand
 * Returns the header of the expanded memory
*/
static Header *expandmem(Header *header, int newsize);

/* Shrinks a block of memory either
 * by shrinking in place or relocation.
 *  header - a pointer to the header of
 *           the chunk of memory to shrink
 * Returns the header of the shrunken memory
*/
static Header *shrinkmem(Header *header, int newsize);

/* Gets the header attached to
 * the chunk of memory pointed to
 * by the pointer.
 * ptr - a pointer to the memory
 *       whose header is desired
 * Returns a pointer to a Header
*/
static Header *getheader(void *ptr);

// A small calculation of the size-aligned
// memory needed for a Header
static size_t headersize();

/* When DEBUG_MALLOC is set to 1
 * it prints the given message.
 *  msg - the desired string to print
*/
static void throwmsg(const char *msg);

#endif