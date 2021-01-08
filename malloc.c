#include <stdio.h>
#include <stdint.h>
#include <errno.h>

typedef struct Header {
    void *mem_start;
    int mem_siz;
    uint8_t status;
    struct Header *prev;
    struct Header *next;
} Header;

#define FREE 0
#define INUSE 1

#define DEBUG_MALLOC 0
#define BUGBUF_SIZ 500

#define MIN_UNIT 16 // [1,INT_MAX]
#define HEAP_CHUNK_SIZ (MIN_UNIT * 4000)
#define MIN_FREE_CHUNK_SIZ (MIN_UNIT * 10)

char bugbuf[BUGBUF_SIZ];
struct Header *heapstart = NULL;

static void throwmsg(const char *msg)
{
    #if DEBUG_MALLOC
    snprintf(&bugbuf, BUGBUF_SIZ, msg);
    fputs(&bugbuf, stderr);
    #endif
}

static size_t headersize()
{
    // Round the size of the header to the closest
    // multiple of the minimum alignment unit
    return sizeof(Header) + 
    ((MIN_UNIT - (sizeof(Header) % MIN_UNIT)) % MIN_UNIT);
}

static void formatmem(Header **headptr, void *memstart, int size)
{
    // Assign head ptr to start address of returned memory chunk
    *headptr = (Header*)memstart;
    // Initialize memory header data
    (*headptr)->mem_start = memstart + headersize();
    (*headptr)->mem_siz = size - headersize();
    (*headptr)->status = FREE;
    (*headptr)->prev = NULL;
    (*headptr)->next = NULL;
}

static Header *divmem(Header *header, int size)
{
    // CAUTION: Doesn't verify that dividing the chunk will result in
    // overlapping headers or memory chunks of size 0

    // If the memory is not already divided appropriately
    if(header->mem_siz != size)
    {
        // Section off and format the remaining memory
        Header *remaining_header = NULL;
        void *remaining_data = header->mem_start + size;
        int remaining_size = header->mem_siz - size;
        formatmem(&remaining_header, remaining_data, remaining_size);
        // Update the original header
        header->mem_siz = (void*)remaining_header - header->mem_start;
        // Link the new header to the original header
        remaining_header->next = header->next;
        if(header->next != NULL)
        {
            header->next->prev = remaining_header;
        }
        // Link original header to the new header
        header->next = remaining_header;
        remaining_header->prev = header;
    }
    return header;
}

static Header *getheader(void *ptr)
{
    Header *chunk = heapstart;
    // Check if the pointer falls within the bounds
    // of the current memory chunk
    while(chunk != NULL &&
    ptr >= chunk->mem_start + chunk->mem_siz)
    {
        chunk = chunk->next;
    }
    #if DEBUG_MALLOC
    // If none found, throw a message
    if(chunk == NULL)
    {
        snprintf(&bugbuf, BUGBUF_SIZ, 
        "MALLOC: Unable to find allocated memory at %p\n",
        ptr);
        fputs(&bugbuf, stderr);
    }
    #endif
    return chunk;
}

static Header *expandmem(Header *header, int newsize)
{
    // Check if next chunk exists and is free
    Header *next = header->next;
    if(next != NULL && (next->status == FREE))
    {
        // Check if there is enough memory for:
        // * User desired space
        // * A shifted header
        // * Remaining space attached to shifted header

        // In the case of an exact match if next's header is dissolved
        if((header->mem_siz + headersize() + next->mem_siz)
        == newsize)
        {
            // 'Dissolve' header
            header->next = next->next;
            if(next->next != NULL)
            {
                next->next->prev = header;
            }
            // Update header info about its memory size
            header->mem_siz = header->mem_siz + headersize() + next->mem_siz;
            return header;
        }
        // In the case of a feasible fit
        else if(header->mem_siz + next->mem_siz - MIN_FREE_CHUNK_SIZ >=
        newsize)
        {
            // Shift header to expand preceeding block
            void *dest = (void*)(header->mem_start + newsize);
            Header *sheader = (Header*)memmove(dest, next, headersize());
            // Update header info
            header->mem_siz = dest - header->mem_start;
            header->next = sheader;
            // Update shifted header info
            int offset = ((int)dest - (int)next);
            sheader->mem_siz = sheader->mem_siz - offset;
            sheader->mem_start = sheader->mem_start + offset;
            if(sheader->next != NULL){
                sheader->next->prev = sheader;
            }
            return header;
        }
        // Otherwise, no possible fit with next block
    }
    // Check if previous chunk exists and is free
    Header *prev = header->prev;
    if(prev != NULL && (prev->status == FREE))
    {
        // In the case of an exact fit if the current header is dissolved
        if(prev->mem_siz + headersize() + header->mem_siz ==
        newsize)
        {
            // Dissolve current header
            // Recalculate previous header's memory size and start
            prev->mem_siz = prev->mem_siz + headersize() + header->mem_siz;
            // Relink previous header to next header
            prev->next = header->next;
            if(header->next != NULL)
            {
                header->next->prev = prev;
            }
            // Copy data into start of previous header
            memmove(prev->mem_start, header->mem_start, header->mem_siz);
            prev->status = INUSE;
            return prev;
        }
        // In the case of a feasible fit into the previous chunk
        else if(prev->mem_siz + header->mem_siz - MIN_FREE_CHUNK_SIZ >=
        newsize)
        {
            // Store current header info
            Header tmpHeader;
            memcpy(&tmpHeader, header, headersize());
            // Recalculate both headers' info
            int offset = newsize - prev->mem_siz;
            prev->mem_siz = newsize;
            tmpHeader.mem_siz = tmpHeader.mem_siz - (offset);
            tmpHeader.mem_start = tmpHeader.mem_start + offset;
            // Copy data into previous memory chunk
            memmove(prev->mem_start, header->mem_start, header->mem_siz);
            // Update current header appropriately
            void *sheader_loc = prev->mem_start + prev->mem_siz;
            Header *sheader = (Header*)memcpy(sheader_loc, 
                                &tmpHeader, headersize());
            sheader->status = FREE;
            // Update previous header linking
            prev->next = sheader;
            if(sheader->next != NULL)
            {
                sheader->next->prev = sheader;
            }
            prev->status = INUSE;
            return prev;
        }
    }
    // Essentially malloc for the data and copy over old data
    void* new_space = malloc(newsize);
    if(new_space == NULL)
    {
        return NULL;
    }
    memmove(new_space, header->mem_start, header->mem_siz);
    free(header->mem_start);
    return getheader(new_space);
}

static Header *shrinkmem(Header *header, int newsize)
{
    Header *next = header->next;
    // If next header is free, then it can be shifted
    if(next != NULL && next->status == FREE)
    {
        // Shift the next header
        void *mem_end = header->mem_start + newsize;
        Header *sheader = (Header*)memmove(mem_end, next, headersize());
        // Relink header to the new next header
        header->next = sheader;
        if(sheader->next != NULL)
        {
            sheader->next->prev = sheader;
        }
        // Recalculate headers' sizes and start locations
        sheader->mem_start = sheader + headersize();
        int offset = (header->mem_siz - newsize);
        sheader->mem_siz = sheader->mem_siz + offset;
        header->mem_siz = newsize;
        return header;
    }
    // If next header is in use, then try dividing the current chunk
    else if(newsize + headersize() + MIN_FREE_CHUNK_SIZ <= header->mem_siz)
    {
        return divmem(header, newsize);
    }
    // Or find the chunk's contents a new home
    else
    {
        void* new_space = malloc(newsize);
        if(new_space == NULL)
        {
            return NULL;
        }
        memmove(new_space, header->mem_start, newsize);
        free(header->mem_start);
        return getheader(new_space);
    }
}

static int getmem(Header **headptr, int size)
{
    // If the first time getting memory,
    // align the start of data
    void * prog_start;
    if(heapstart == NULL)
    { 
        int offset = (MIN_UNIT - ((uintptr_t)sbrk(0) % MIN_UNIT)) % MIN_UNIT;
        prog_start = sbrk(offset);
    }
    int req_siz = HEAP_CHUNK_SIZ + headersize();
    // Check if requested size is more than typical request
    if(size + headersize() > req_siz)
    {
        // Update request size
        req_siz = size + headersize();
    }
    // If requested size is smaller than standard HEAP_CHUNK_SIZ, ensure that
    // during division, it can handle another header with minimum data
    else if(req_siz - (size + headersize()) < 
    (headersize() + MIN_FREE_CHUNK_SIZ))
    {
        // Update request size to be large enough to handle
        // another header and the minimum amount of memory
        req_siz = (2 * headersize() ) + size + MIN_FREE_CHUNK_SIZ;
    }
    // Ask for memory
    void *memstart = sbrk(req_siz);
    // Check for errors
    if(memstart < 0 )
    {
        throwmsg("MALLOC: Cannot sbrk");
        errno = ENOMEM;
        return -1;
    }
    // Format memory appropriately
    formatmem(headptr, memstart, req_siz);
    // Assign it to heapstart if first time grabbing data
    if(heapstart == NULL)
    {
        heapstart = *headptr;
    }
    // Return as successful
    return 0;
}

static void mergemem(Header *header)
{
    // Check for free adjacent memory in previous chunk
    Header *prev = header->prev;
    if(prev != NULL && (prev->status == FREE))
    {
        // Previous chunk is free, so merge them
        // Link previous chunk header to next chunk header
        prev->next = header->next;
        if(header->next != NULL)
        {
            header->next->prev = prev;
        }
        // Calculate size of merged data
        // (from start of previous to end of current)
        prev->mem_siz = (header->mem_start + header->mem_siz) - prev->mem_start;
        // Reassign the current chunk to work on
        header = prev;
    }
    // Check for free adjacent memory in next chunk
    Header *next = header->next;
    if(next != NULL && (next->status == FREE))
    {
        // Next chunk is free, so merge them
        // Link header to following header (next, next header)
        header->next = next->next;
        if(next->next != NULL)
        {
            next->next->prev = header;
        }
        // Calculate size of merged data
        header->mem_siz = (next->mem_start + next->mem_siz) - 
        header->mem_start;
    }
}

extern void *malloc(size_t size)
{
    // Check if size is 0
    if(size == 0)
    {
        return NULL;
    }
    // Adjust size for minimum size unit
    int adjusted_size = size + ((MIN_UNIT - (size % MIN_UNIT)) % MIN_UNIT);
    // Check for available memory
    Header *prev_chunk = NULL;
    Header *curr_chunk = heapstart;
    int found = 0;
    while(curr_chunk != NULL && !found)
    {
        // Check for a chunk that is free and large enough for:
        // * The requested size
        // * A header for the remaining data
        // * More remaining data than the minimum allowed (MIN_FREE_CHUNK_SIZ)
        if(curr_chunk->status == FREE &&
        (adjusted_size == curr_chunk->mem_siz ||
        adjusted_size + headersize() + MIN_FREE_CHUNK_SIZ <= 
        curr_chunk->mem_siz))
        {
            // Signal a large enough chunk was found
            found = 1;
        }
        else
        {
            // Continue to the next chunk of data
            prev_chunk = curr_chunk;
            curr_chunk = curr_chunk->next;
        }
    }
    // If cannot find memory of large enough size, request some
    if(!found)
    {
        struct Header *new_mem = NULL;
        // If successfully recieved memory
        if(getmem(&new_mem, adjusted_size) == 0)
        {
            //Link it to previous (doesn't hurt if it's null)
            new_mem->prev = prev_chunk;
            // Link previous to it if it exists
            if(prev_chunk != NULL)
            {
                prev_chunk->next = new_mem;
            }
            // Hand off new memory for rest of function use
            curr_chunk = new_mem;
        }
        // If failed to get memory, abort mission
        else
        {
            throwmsg("MALLOC: Unable to get enough space for malloc");
            return NULL;
        }
    }
    // If the function reaches here, it has located a large enough memory chunk
    // Carve out the appropriate size of memory from that chunk
    divmem(curr_chunk, adjusted_size);
    // Mark the chunk as 'in use'
    curr_chunk->status = INUSE;
    // Quick debug message
    #if DEBUG_MALLOC
        snprintf(&bugbuf, BUGBUF_SIZ, 
        "MALLOC: malloc(%d)    =>    (ptr=%p, size=%d)\n",
        size, curr_chunk->mem_start, adjusted_size);
        fputs(&bugbuf, stderr);
    #endif
    // Return the chunk with the appropriate size
    return curr_chunk->mem_start;
}

extern void free(void *ptr)
{
    // Guard against loop when snprtinf-ing
    if(ptr != NULL)
    {
        // Locate the block of memory the ptr belongs to
        Header *chunk = getheader(ptr);
        // If the ptr passed in wasn't found, throw warning
        if(chunk == NULL || (chunk->status == FREE))
        {
            snprintf(&bugbuf, BUGBUF_SIZ, 
            "MALLOC: No data to free at %p\n", ptr);
            fputs(&bugbuf, stderr);
        }
        else
        {
            // Free the block of memory
            chunk->status = FREE;
            // Merge any free adjacent memory
            mergemem(chunk);
        }
        // Quick debug message
    #if DEBUG_MALLOC
        snprintf(&bugbuf, BUGBUF_SIZ, "MALLOC: free(%p)\n", ptr);
        fputs(&bugbuf, stderr);
    #endif
    }
}

extern void *calloc(size_t nmemb, size_t size)
{
    // Check if either param is zero
    if(nmemb == 0 || size == 0)
    {
        return NULL;
    }
    // Calculate bytes of memory needed
    size_t block_size = nmemb * size;
    // Adjust size for minimum size unit
    int adjusted_size = block_size + 
    ((MIN_UNIT - (block_size % MIN_UNIT)) % MIN_UNIT);
    // Allocate memory
    void *mem = malloc(adjusted_size);
    // Clear all memory
    memset(mem, 0, adjusted_size);
    // Quick debug message
    #if DEBUG_MALLOC
        snprintf(&bugbuf, BUGBUF_SIZ, 
        "MALLOC: calloc(%d, %d)    =>    (ptr=%p, size=%d)\n",
        nmemb, size, mem, adjusted_size);
        fputs(&bugbuf, stderr);
    #endif
    // Return pointer to start of memory
    return mem;
}

extern void *realloc(void *ptr, size_t size)
{
    // Check for zero values in params
    if(ptr == NULL)
    {
        return malloc(size);
    }
    else if(size == 0)
    {
        free(ptr);
        return NULL;
    }
    // Get the header
    Header *header = getheader(ptr);
    if(header == NULL)
    {
        return NULL;
    }
    // Adjust size to fit alignment
    int adjusted_size = size + ((MIN_UNIT - (size % MIN_UNIT)) % MIN_UNIT);
    // Check if expanding
    if(adjusted_size > header->mem_siz)
    {
        header = expandmem(header, adjusted_size);
    }
    // Check if shrinking
    else if(adjusted_size < header->mem_siz)
    {
        header = shrinkmem(header, adjusted_size);
    }
    else
    {
    #if DEBUG_MALLOC
        throwmsg("MALLOC: No reallocation because size wasn't different.");
    #endif
        return ptr;
    }
    // Quick debug message
    #if DEBUG_MALLOC
        snprintf(&bugbuf, BUGBUF_SIZ, 
        "MALLOC: realloc(%p, %d)    =>    (ptr=%p, size=%d)\n",
        ptr, size, header->mem_start, header->mem_siz);
        fputs(&bugbuf, stderr);
    #endif
    if(header == NULL)
    {
        return NULL;
    }
    return header->mem_start;
}
