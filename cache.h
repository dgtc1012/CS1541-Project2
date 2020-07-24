#include <stdlib.h>
#include <stdio.h>

struct cache_blk_t {
  unsigned long tag;
  unsigned char valid;
  unsigned char dirty;
  unsigned long long ts;	//a timestamp that may be used to implement LRU replacement
  // To guarantee that L2 is inclusive of L1, you may need an additional flag 
  // in L2 to indicate that the block is cached in L1
};

struct cache_t {
	// The cache is represented by a 2-D array of blocks. 
	// The first dimension of the 2D array is "nsets" which is the number of sets (entries)
	// The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;				// # sets
  int blocksize;			// block size
  int assoc;				// associativity
  int hit_latency;			// latency in case of a hit
  struct cache_blk_t **blocks;    // the array of cache blocks
};

struct cache_t * cache_create(int, int, int, int);
int cache_access(struct cache_t *, unsigned long, char, unsigned long long, struct cache_t *);
struct cache_blk_t * find_blk(struct cache_t *, unsigned long);
int cache_free(struct cache_t *, struct cache_t *);
