//--------------------------------------------------------------------------------------------------
// System Programming                       Memory Lab                                   Fall 2020
//
/// @file
/// @brief dynamic memory manager
/// @author <Jinsol Park>
/// @studid <2018-14000>
//--------------------------------------------------------------------------------------------------


// Dynamic memory manager
// ======================
// This module implements a custom dynamic memory manager.
//
// Heap organization:
// ------------------
// The data segment for the heap is provided by the dataseg module. A 'word' in the heap is
// eight bytes.
//
// Implicit free list:
// -------------------
// - minimal block size: 32 bytes (header +footer + 2 data words)
// - h,f: header/footer of free block
// - H,F: header/footer of allocated block
//
// - state after initialization
//
//         initial sentinel half-block                  end sentinel half-block
//                   |                                             |
//   ds_heap_start   |   heap_start                         heap_end       ds_heap_brk
//               |   |   |                                         |       |
//               v   v   v                                         v       v
//               +---+---+-----------------------------------------+---+---+
//               |???| F | h :                                 : f | H |???|
//               +---+---+-----------------------------------------+---+---+
//                       ^                                         ^
//                       |                                         |
//               32-byte aligned                           32-byte aligned
//
// place sentinel half block to make algorithm easier when coalescing
// - allocation policies: first, next, best fit
// - block splitting: always at 32-byte boundaries
// - immediate coalescing upon free
//


#include <assert.h>
#include <error.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dataseg.h"
#include "memmgr.h"

void mm_check(void);


static void *ds_heap_start = NULL;                     ///< physical start of data segment
static void *ds_heap_brk   = NULL;                     ///< physical end of data segment
static void *heap_start    = NULL;                     ///< logical start of heap
static void *heap_end      = NULL;                     ///< logical end of heap
static void *next_block    = NULL;                     ///< next block used by next-fit policy
static int  PAGESIZE       = 0;                        ///< memory system page size
static int  mm_initialized = 0;                        ///< initialized flag (yes: 1, otherwise 0)
static int  mm_loglevel    = 0;                        ///< log level (0: off; 1: info; 2: verbose)

static void* (*get_block)(size_t) = NULL; //< function pointer
                    // it can point to any function that returns void*


#define MAX(a, b)          ((a) > (b) ? (a) : (b))     ///< MAX function

#define TYPE               unsigned long               ///< word type of heap - 8 bytes
#define TYPE_SIZE          sizeof(TYPE)                ///< size of word type

#define ALLOC              1                           ///< block allocated flag
#define FREE               0                           ///< block free flag
#define STATUS_MASK        ((TYPE)(0x7))               ///< mask to retrieve flagsfrom header/footer
#define SIZE_MASK          (~STATUS_MASK)              ///< mask to retrieve size from header/footer

#define CHUNKSIZE          (1*(1 << 12))               ///< size by which heap is extended

#define BS                 32                          ///< minimal block size. Must be a power of 2
#define BS_MASK            (~(BS-1))                   ///< alignment mask

#define WORD(p)            ((TYPE)(p))                 ///< convert pointer to TYPE
#define PTR(w)             ((void*)(w))                ///< convert TYPE to void*

#define PREV_PTR(p)        ((p)-TYPE_SIZE)             ///< get pointer to word preceeding p

#define PACK(size,status)  ((size) | (status))         ///< pack size & status into boundary tag
#define SIZE(v)            (v & SIZE_MASK)             ///< extract size from boundary tag
#define STATUS(v)          (v & STATUS_MASK)           ///< extract status from boundary tag

#define GET(p)             (*(TYPE*)(p))               ///< read word at *p
#define GET_SIZE(p)        (SIZE(GET(p)))              ///< extract size from header/footer
#define GET_STATUS(p)      (STATUS(GET(p)))            ///< extract status from header/footer

#define PUT(p, v)           (*((TYPE*)(p)) = (v))       ///< write value v to dereference pointer *p

#define ROUND_UP(w)       (((w)+BS-1)/BS*BS)
#define ROUND_DOWN(w)     ((w)/BS*BS)

#define HDR2FTR(p)        ((p) + GET_SIZE(p)-TYPE_SIZE) // get location of footer tag given a header tag
#define NEXT_BLOCK(p)     ((p) + GET_SIZE(p))
#define PREV_BLOCK(p)     ((p) - GET_SIZE(((p)-TYPE_SIZE)))

// add more macros as needed

/// @brief print a log message if level <= mm_loglevel. The variadic argument is a printf format
///        string followed by its parametrs
#define LOG(level, ...) mm_log(level, __VA_ARGS__)

/// @brief print a log message. Do not call directly; use LOG() instead
/// @param level log level of message.
/// @param ... variadic parameters for vprintf function (format string with optional parameters)
static void mm_log(int level, ...)
{
  if (level > mm_loglevel) return;

  va_list va;
  va_start(va, level);
  const char *fmt = va_arg(va, const char*);

  if (fmt != NULL) vfprintf(stdout, fmt, va);

  va_end(va);

  fprintf(stdout, "\n");
}


/// @brief print error message and terminate process. The variadic argument is a printf format
///        string followed by its parameters
#define PANIC(...) mm_panic(__func__, __VA_ARGS__)

/// @brief print error message and terminate process. Do not call directly, Use PANIC() instead.
/// @param func function name
/// @param ... variadic parameters for vprintf function (format string with optional parameters)
static void mm_panic(const char *func, ...)
{
  va_list va;
  va_start(va, func);
  const char *fmt = va_arg(va, const char*);

  fprintf(stderr, "PANIC in %s%s", func, fmt ? ": " : ".");
  if (fmt != NULL) vfprintf(stderr, fmt, va);

  va_end(va);

  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}

static void* ff_get_free_block(size_t);
static void* nf_get_free_block(size_t);
static void* bf_get_free_block(size_t);

/// @brief initialize the heap
/// @param ap allocation policy

void mm_init(AllocationPolicy ap)
{
  LOG(1, "mm_init(%d)", ap);
  //set allocation policy
  char *apstr;
  switch(ap){
    case ap_FirstFit: get_block = ff_get_free_block; apstr = "first fit"; break;
    case ap_NextFit: get_block = nf_get_free_block; apstr = "next fit"; break;
    case ap_BestFit: get_block = bf_get_free_block; apstr = "best fit"; break;
    default: PANIC("invalid allocation policy.");
  }
  LOG(2, "    allocation policy       %s\n", apstr);

  ds_heap_stat(&ds_heap_start, &ds_heap_brk, NULL);
  PAGESIZE = ds_getpagesize();

  LOG(1, "  ds_heap_start    %p\n"
         "  ds_heap_brk      %p\n"
         "  PAGESIZE         %d\n",
         ds_heap_start, ds_heap_brk, PAGESIZE);

  if (ds_heap_start == NULL) PANIC("Data segment not initialized.");
  if (ds_heap_start != ds_heap_brk) PANIC("Heap not clean.");
  if (PAGESIZE == 0) PANIC("Reported pagesize == 0.");

  LOG(2, "Get first block of memory for heap"); 
  if(ds_sbrk(CHUNKSIZE) == (void*)-1) PANIC("Cannot increase heap break.");     // increase heap by pre defined size 
  ds_heap_brk = ds_sbrk(0);                                                     //get curr pointer of brk as ds_heap_brk
  LOG(2, "Break is now at %p", ds_heap_brk);
  
  // compute location of heap_start/end (32 bytes from start/end of ds_heap_start.brk)
  heap_start = PTR((WORD(ds_heap_start) + TYPE_SIZE + BS - 1) / BS * BS);       // 32-byte round up aligned
  heap_end = PTR((WORD(ds_heap_brk) - TYPE_SIZE) / BS * BS);                    // 32-byte round down
  LOG(2, "  heap_start at    %p\n"
         "  heap_end_at      %p\n",
        heap_start, heap_end);

  // write sentinel half blocks
  TYPE F = PACK(0,ALLOC);
  PUT(heap_start - TYPE_SIZE, F);
  
  TYPE H = PACK(0,ALLOC);
  PUT(heap_end, H);

  //write free block
  TYPE size = heap_end - heap_start;
  TYPE bdrytag = PACK(size, FREE);
  PUT(heap_start, bdrytag);
  PUT(heap_end - TYPE_SIZE, bdrytag);

  next_block = heap_start;                                                      // initialize the global var for next fit
  LOG(1, "next block is initialized to: %p", next_block);

  mm_initialized = 1;
}

/// @brief getting free block with first fit policy
/// @param size requiring size of allocating block
static void* ff_get_free_block(size_t size){
  LOG(1, "ff_get_free_block(0x%lx (%lu))", size, size);
  assert(mm_initialized);
  
  // implementing first fit
  void *block = heap_start;
  size_t bsize, bstatus;

  LOG(2, "  starting search at %p", block);
  do{
    bstatus = GET_STATUS(block);
    bsize = GET_SIZE(block);
    LOG(2, "    %p: size: %lx (%lu), status: %s",
        block, bsize, bsize, bstatus == ALLOC ? "allocated" : "free");

    if((bstatus == FREE) && (bsize >= size)){                               // when FREE and fits size
      // found block
      LOG(2, "    --> match");
      return block;                                                         // returning appropriate free block
    }
    block += bsize;                                                         // brings us to beginning of next block
  } while (GET_SIZE(block) > 0);                                            // run through the heap until reach sentinel (size 0)
  LOG(2, "    no suitable block found");                                    // when not block is found
  return NULL;
}

/// @brief getting free block with next fit policy
/// @param size requiring size of allocating block
static void* nf_get_free_block(size_t size){
  LOG(1, "nf_get_free_block((0x%lx (%lu))",size, size);
  assert(mm_initialized);
  size_t bsize, bstatus;
  void* block = next_block;                                 // initialize 'block' to the global car next_block
  int cnt = 0;
  while(1){
    bstatus = GET_STATUS(block);  
    bsize = GET_SIZE(block);
    if(GET_SIZE(NEXT_BLOCK(block)) == 0) cnt++;             // cnt is used to check if block position loops around and comes back

    if((bstatus == FREE) && (bsize >= size)){               // when FREE and fits size
      if(block + bsize < heap_end){                         // if the next block address is valid
        next_block = block + bsize;                         // set that to next_block
      } else{
        next_block = heap_start;                            // if next block is over heap, set the beginning of heap to next_block
      }
      return block;                                         // when this is an appropriate block
    } 

    LOG(1,"current cnt is: %d", cnt);
    if(block + bsize < heap_end){                           // change the 'block' pointer depending on different cases
      block += bsize;                               
    }else {
      block = heap_start;
    } 
    if(cnt == 2) break;                                     // when end of heap is reached twice, it assures that 
                                                            // the block pointer looped around all possible blocks and came back
  }
  LOG(1, "  no suitable block is found");
  return NULL;                                              // when finding block fails
}

/// @brief getting free block with best fit policy
/// @param size requiring size of allocating block
static void* bf_get_free_block(size_t size){
  LOG(1, "bf_get_free_block((0x%lx (%lu))",size, size);
  assert(mm_initialized);
  size_t bsize, bstatus;
  void *block = heap_start;
  size_t minsize = 0;                                               // keeps the minimum size larger than 'size'
  void *minblock = NULL;                                            // keeps the pointer to such a block
  
  while(1){
    if((GET_STATUS(block) ==FREE) && (GET_SIZE(block)>= size)) {
      minsize = GET_SIZE(block);
      minblock = block;
      break;
    }                                                               // initialized minsize and minblock to first FREE block
    block += GET_SIZE(block);
    if(GET_SIZE(block) == 0) break;                                 // when it reaches end of heap
  } // find the first free block

  do{
    bstatus = GET_STATUS(block);
    bsize = GET_SIZE(block);

    if((bstatus == FREE) && (bsize >= size)){                       // if block is FREE, and fits size
      if(bsize < minsize){                                          // if that block's size is smaller than previous minsize
        minsize = bsize;                                            // update min block
        minblock = block;
      }
    }
    block += bsize;                                                 // brings us to beginning of next block
  } while (GET_SIZE(block) != 0);                                   // run through the heap until reach sentinel (size 0)

                                                                    // run through entire implicit free list, 
                                                                    // take block that matches the smallest possible size
  return minblock;
}


/// @brief coalescing blocks upon free()
/// @param block the pointer to the block that need to be coalesced
static void* coalesce(void *block){
  // input is given by pointer to header
  LOG(1, "coalesce(%p)", block);

  assert(mm_initialized);
  assert(GET_STATUS(block) == FREE);

  TYPE size = GET_SIZE(block);
  void *hdr = block;
  void *ftr = block + size - TYPE_SIZE; 

  // can we coalesce with the following block
  if(GET_STATUS(NEXT_BLOCK(block)) == FREE){
    LOG(2, "    coalescing with suceeding block");

    //size of coalesced block : size of block +  size of following block
    size += GET_SIZE(NEXT_BLOCK(block));  // gives new size of block coalesced
    ftr = hdr + size - TYPE_SIZE;
  }

  if(size > GET_SIZE(block)){
    PUT(hdr, PACK(size, FREE));
    PUT(ftr, PACK(size, FREE));
  }
  // can we coalesce with the previous block
  if(GET_STATUS(block-TYPE_SIZE) == FREE){
    LOG(1, "    coalescing with previous block");

    //size of coalesced block : size of block +  size of previous block
    size += GET_SIZE(PREV_BLOCK(block));  // gives new size of block coalesced
    hdr = PREV_BLOCK(block);              // header is just the previous block itself

  }

  if(size > GET_SIZE(block)){
    PUT(hdr, PACK(size, FREE));
    PUT(ftr, PACK(size, FREE));
  }
  return hdr;
}

/// @brief expanding heap
/// @param blocksize blocksize of block that needs to be allocated
void* expand_heap(size_t blocksize){ //doxygen
  LOG(1, "expand_heap()");
  TYPE size;
  if(GET_STATUS(heap_end - TYPE_SIZE) == FREE){
    LOG(1, "prev size: %lx", GET_SIZE(heap_end - TYPE_SIZE));
    size = blocksize - GET_SIZE(heap_end - TYPE_SIZE);
  } else { size = blocksize; }
  // old_heap_end      new_heap_end
  // ,                  ,
  //-----------------------------------------
  // |s|?|              |s|?|
  // -----------------------------------------
  //     ^                  ^
  //   ds_old_brk         ds_new_brk
  // they above is a grid for the situation below
  int ps = ds_getpagesize();
  size = (size + ps - 1) / ps * ps;
  void *ds_old_brk = ds_sbrk(size); 
  void *ds_new_brk = ds_sbrk(0);   
  void *old_heap_end = heap_end;
  void *new_heap_end = PTR((WORD(ds_new_brk) - TYPE_SIZE) / BS * BS);
  
  // make footer and header for new large free block
  PUT(old_heap_end, PACK(size, FREE));
  PUT(new_heap_end - TYPE_SIZE, PACK(size, FREE));
  
  // make new sentinel
  PUT(new_heap_end, PACK(0, ALLOC));

  heap_end = new_heap_end;
  ds_heap_brk = ds_new_brk;

  void *free_hdr = coalesce(old_heap_end);    // call coalesce for the new large free block
  
  return free_hdr;                            // return the header of new large free block
}

void* mm_malloc(size_t size)
{
  LOG(1, "mm_malloc(0x%lx) (%lu in decimal)", size, size);

  assert(mm_initialized);
  
  //figure out how big the needed block is
  //internally, 32 block size, align, put header and footer at end
  
  // compute the block size
  size_t blocksize = ROUND_UP(TYPE_SIZE + size + TYPE_SIZE);  // round up the size that needs to be allocated
  LOG(1, "  blocksize:      %lx (%lu)", blocksize, blocksize);

  // find free block
  void *block = get_block(blocksize); // declare a new function
  LOG(2, "    got free block: %p", block);
  

  if(block == NULL){  // NULL is returned if block could not be found
    // no free block is big enough --> need to expand heap
    //IDEA FOR EXPANDING THE HEAP:
    //we have sentinel at end. just expand the heap and put the new free block there
    //in the place of unknown leftover part
    //call coalesce on that block, merge it with two small free block there
    //when we expand heap, need to write new footer
    //when expand heap by increasing ds_heap_brk, make the end sentinel and stuff again
    //do the thing done at the beginning in mm_init()
    block = expand_heap(blocksize); // well implemented!!
    
  }

  //split block when the return value is not NULL
  size_t bsize = GET_SIZE(block);
  if(blocksize < bsize){
    // ---------------------------------------------------------
    // |h|                                                   |f|
    // ---------------------------------------------------------
    //                    ^
    //                  blocksize
    // after splitting;
    // ---------------------------------------------------------
    // |h|     alloc    |f|h|       split free block        |f|
    // ---------------------------------------------------------

    void *next_block = block + blocksize;
    size_t next_size = bsize - blocksize;
    
    PUT(next_block, PACK(next_size, FREE));                           // header of next block
    PUT(next_block + next_size - TYPE_SIZE, PACK(next_size, FREE));   // footer of next block
  }
  PUT(block, PACK(blocksize, ALLOC));
  PUT(block + blocksize - TYPE_SIZE, PACK(blocksize, ALLOC));
  
  return block + TYPE_SIZE;                                           // returning the block to payload
}

void* mm_calloc(size_t nmemb, size_t size)
{
  LOG(1, "mm_calloc(0x%lx, 0x%lx)", nmemb, size);

  assert(mm_initialized);

  //
  // calloc is simply malloc() followed by memset()
  //
  void *payload = mm_malloc(nmemb * size);

  if (payload != NULL) memset(payload, 0, nmemb * size);

  return payload;
}

void* mm_realloc(void *ptr, size_t size)
{
  LOG(1, "mm_realloc(%p, 0x%lx)", ptr, size);

  assert(mm_initialized);

  //
  // TODO (optional)
  //

  return NULL;
}

void mm_free(void *ptr)
{
  LOG(1, "mm_free(%p)", ptr);

  assert(mm_initialized);

  void *block = ptr - TYPE_SIZE;                // header of given block
  if(GET_STATUS(block) != ALLOC) {              // if the block is already free
    LOG(1, "    WARNING: double-free detected");
    return;
  }

  //mark as free
  TYPE size = GET_SIZE(block);
  PUT(block, PACK(size, FREE));
  PUT(block+size - TYPE_SIZE, PACK(size, FREE));

  //need to implement coalsescin
  void *free_hdr = coalesce(block);
}


void mm_setloglevel(int level)
{
  mm_loglevel = level;
}


void mm_check(void)
{
  assert(mm_initialized);

  void *p;

  printf("\n----------------------------------------- mm_check ----------------------------------------------\n");
  printf("  ds_heap_start:          %p\n", ds_heap_start);
  printf("  ds_heap_brk:            %p\n", ds_heap_brk);
  printf("  heap_start:             %p\n", heap_start);
  printf("  heap_end:               %p\n", heap_end);
  printf("\n");
  p = PREV_PTR(heap_start);
  printf("  initial sentinel:       %p: size: %6lx, status: %lx\n", p, GET_SIZE(p), GET_STATUS(p));
  p = heap_end;
  printf("  end sentinel:           %p: size: %6lx, status: %lx\n", p, GET_SIZE(p), GET_STATUS(p));
  printf("\n");
  printf("  blocks:\n");

  long errors = 0;
  p = heap_start;
  while (p < heap_end) {
    TYPE hdr = GET(p);
    TYPE size = SIZE(hdr);
    TYPE status = STATUS(hdr);
    printf("    %p: size: %6lx, status: %lx\n", p, size, status);

    void *fp = p + size - TYPE_SIZE;
    TYPE ftr = GET(fp);
    TYPE fsize = SIZE(ftr);
    TYPE fstatus = STATUS(ftr);

    if ((size != fsize) || (status != fstatus)) {
      errors++;
      printf("    --> ERROR: footer at %p with different properties: size: %lx, status: %lx\n", 
             fp, fsize, fstatus);
    }

    p = p + size;
    if (size == 0) {
      printf("    WARNING: size 0 detected, aborting traversal.\n");
      break;
    }
  }

  printf("\n");
  if ((p == heap_end) && (errors == 0)) printf("  Block structure coherent.\n");
  printf("-------------------------------------------------------------------------------------------------\n");
}
