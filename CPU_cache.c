#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <math.h>
#include "CPU.h"

// to keep cache statistics
unsigned int accesses = 0;
unsigned int read_accesses = 0;
unsigned int write_accesses = 0;
unsigned int L1hits = 0;
unsigned int L1misses = 0;
unsigned int L2hits = 0;
unsigned int L2misses = 0;

unsigned int mem_latency = 100;

#include "cache.h"

unsigned int L1size = 16;
unsigned int bsize = 32;
unsigned int L1assoc = 4;
unsigned int L2size = 0;
unsigned int L2assoc = 0;
unsigned int L2_hit_latency = 6;

int main(int argc, char **argv)
{
    struct trace_item *tr_entry;
    size_t size;
    char *trace_file_name;
    char *config_file_name;
    int trace_view_on = 0;

    unsigned char t_type = 0;
    unsigned char t_sReg_a= 0;
    unsigned char t_sReg_b= 0;
    unsigned char t_dReg= 0;
    unsigned int t_PC = 0;
    unsigned int t_Addr = 0;

    unsigned int cycle_number = 0;

    if (argc < 3) {
        fprintf(stdout, "\nUSAGE: tv <trace_file> <config file> <trace_view_on (1 or 0)>\n");
        fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
        exit(0);
    }

    trace_file_name = argv[1];
    config_file_name = argv[2];
    if (argc == 4) trace_view_on = atoi(argv[3]) ;
    // here you should extract the cache parameters from the command line (cache size, associativity, latency)

    fprintf(stdout, "\n ** opening %s\n", config_file_name);
    FILE * cache_fd = fopen(config_file_name, "r");

    if(!cache_fd)
    {
        fprintf(stdout, "\n ** cache_config.txt not found\n");
        exit(0);
    }

    fscanf(cache_fd, "%ud", &L1size);
    fscanf(cache_fd, "%ud", &bsize);
    fscanf(cache_fd, "%ud", &L1assoc);
    fscanf(cache_fd, "%ud", &L2size);
    fscanf(cache_fd, "%ud", &L2assoc);
    fscanf(cache_fd, "%ud", &L2_hit_latency);
    fscanf(cache_fd, "%ud", &mem_latency);

    fclose(cache_fd);

    fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

    //trace_fd = fopen(trace_file_name, "rb");
    trace_fd = fopen(trace_file_name, "r");

    if (!trace_fd)
    {
        fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
        exit(0);
    }

    trace_init();

    struct cache_t *L1;
    struct cache_t *L2;
    struct cache_t *nextL;

    L1 = cache_create(L1size, bsize, L1assoc, 0);

    nextL = NULL;   // the next level in the hierarchy -- NULL indicates main memory

    if (L2size != 0)
    {
        L2 = cache_create(L2size, bsize, L2assoc, L2_hit_latency);
        nextL = L2;
    }

    while(1)
    {
        //printf("enter while loop");
        size = trace_get_item(&tr_entry);

        if (!size) {       /* no more instructions (trace_items) to simulate */
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            /*printf("+ Cache statistics \n");*/
            /*printf("+ L1 hits:\t\t %d\n", L1hits);*/
            /*printf("+ L2 hits:\t\t %d\n", L2hits);*/
            /*printf("+ L1 misses:\t\t %d\n", L1misses);*/
            /*printf("+ L2 misses:\t\t %d\n", L2misses);*/
            /*printf("+ read accesses:\t %d\n", read_accesses);*/
            /*printf("+ write accesses:\t %d\n", write_accesses);*/
            /*printf("+ total accesses:\t %d\n", accesses);*/
            /*printf("+ L1 hit rate: \t\t% lf\n", ((double) L1hits)/((double) accesses));*/
            /*printf("+ L2 hit rate: \t\t% lf\n", ((double) L2hits)/((double) accesses));*/
            printf("+ L1 miss rate: \t% lf\n", ((double) L1misses)/((double) accesses));
            /*printf("+ L2 miss rate: \t% lf\n", ((double) L2misses)/((double) accesses));*/
            /*cache_free(L1, nextL);*/
            break;
        }
        else{              /* parse the next instruction to simulate */
            cycle_number++;
            t_type = tr_entry->type;
            t_sReg_a = tr_entry->sReg_a;
            t_sReg_b = tr_entry->sReg_b;
            t_dReg = tr_entry->dReg;
            t_PC = tr_entry->PC;
            t_Addr = tr_entry->Addr;
        }

        // SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
        // IN ONE CYCLE, EXCEPT IF THERE IS A DATA CACHE MISS.

        switch(tr_entry->type) {
            case ti_NOP:
                if (trace_view_on) printf("[cycle %d] NOP:\n", cycle_number);
                break;
            case ti_RTYPE:
                if (trace_view_on) {
                    printf("[cycle %d] RTYPE:", cycle_number);
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
                };
                break;
            case ti_ITYPE:
                if (trace_view_on){
                    printf("[cycle %d] ITYPE:", cycle_number);
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
                };
                break;
            case ti_LOAD:
                if (trace_view_on){
                    printf("[cycle %d] LOAD:", cycle_number);
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
                };
                accesses++;
                read_accesses++;
                cycle_number = cycle_number + cache_access(L1, tr_entry->Addr, 'r', cycle_number, nextL);
                break;
            case ti_STORE:
                if (trace_view_on){
                    printf("[cycle %d] STORE:", cycle_number);
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
                };
                accesses++;
                write_accesses++;
                cycle_number = cycle_number + cache_access(L1, tr_entry->Addr, 'w', cycle_number, nextL);
                break;
            case ti_BRANCH:
                if (trace_view_on) {
                    printf("[cycle %d] BRANCH:", cycle_number);
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
                };
                break;
            case ti_JTYPE:
                if (trace_view_on) {
                    printf("[cycle %d] JTYPE:", cycle_number);
                    printf(" (PC: %x)(addr: %x)\n", tr_entry->PC, tr_entry->Addr);
                };
                break;
            case ti_SPECIAL:
                if (trace_view_on) printf("[cycle %d] SPECIAL:", cycle_number);
                break;
            case ti_JRTYPE:
                if (trace_view_on) {
                    printf("[cycle %d] JRTYPE:", cycle_number);
                    printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
                };
                break;
        }

    }

    trace_uninit();

    exit(0);
}

struct cache_t * cache_create(int size, int blocksize, int assoc, int latency)
{
    int i;
    int nblocks = 1;                      // number of blocks in the cache
    int nsets = 1;                        // number of sets (entries) in the cache

    // YOUR JOB: calculate the number of sets and blocks in the cache
    //
    // nblocks = X;
    // nsets = Y;

    //printf("size: %d, blocksize: %d, assoc: %d, latency: %d\n", size, blocksize, assoc, latency);
    // our code
    nblocks = (size*pow(2,10))/(blocksize);
    nsets = nblocks/assoc;
    
    //printf("nblocks: %d, nsets: %d\n", nblocks, nsets);

    struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));

    C->nsets = nsets;
    C->assoc = assoc;
    C->hit_latency = latency;
    C->blocksize = blocksize;

    //hey we changed this. We should be allocating a pointer to an array of structs, not the actual struct
    C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

    for(i = 0; i < nsets; i++) {
        C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
    }
    /*printf("created\n");*/
    //printf("Cache created %d\n", C->nsets);
    return C;
}

int cache_free(struct cache_t* L1, struct cache_t* L2)
{
    int i;
    for(i = 0; i < L1->nsets; i++)
        free(L1->blocks[i]);

    free(L1->blocks);

    free(L1);
    if(L2 != NULL)
    {
        for(i = 0; i < L2->nsets; i++)
            free(L2->blocks[i]);

        free(L2->blocks);
        free(L2);
    }

    return 0;
}


int cache_access(struct cache_t *cp, unsigned long address, char access_type, unsigned long long now, struct cache_t *next_cp)
{
    /*
       if(next_cp == NULL){
       printf("********************IN L2 CACHE***********\n");
       }
       else{
       printf("********************IN L1 CACHE***********\n");
       }
       */
    //
    // Based on address, determine the set to access in cp and examine the blocks
    // in the set to check hit/miss and update the golbal hit/miss statistics
    // If a miss, determine the victim in the set to replace (LRU). Replacement for
    // L2 blocks should observe the inclusion property.
    //
    // The function should return the hit_latency in case of a hit. In case
    // of a miss, you need to figure out a way to return the time it takes to service
    // the request, which includes writing back the replaced block, if dirty, and bringing
    // the requested block from the lower level (from L2 in case of L1, and from memory in case of L2).
    // This will require one or two calls to cache_access(L2, ...) for misses in L1.
    // It is recommended to start implementing and testing a system with only L1 (no L2). Then add the
    // complexity of an L2.
    // return(cp->hit_latency);
    //fflush(stdout);
    //printf("enter cache access\n");
    unsigned int index_bits = log2(cp->nsets);
    //printf("index_bits %d\n", index_bits);
    //printf("blocksize %d\n", cp->blocksize);
    unsigned int tag_bits = 32 - (index_bits + log2(cp->blocksize)); //removed + 2
    //printf("tag_bits %d\n", tag_bits);
    //printf("address %x\n", address);
    unsigned int index;
    if(index_bits == 0){
        index = 0;
    }
    else{
        index = address << tag_bits;
    //    printf("index1 %x\n", index);
        index = index >> (32 - index_bits);
    }
    //printf("index %x\n", index);
    unsigned int tag = address >> (index_bits + ((int)log2(cp->blocksize))); //removed + 2
    //printf("tag %x\n", tag);
    struct cache_blk_t * set = cp->blocks[index];
    struct cache_blk_t blk = set[0];
    int set_index;
    int hflag = 0;
    //fflush(stdout);
    //printf("Index: %x, Tag: %x\n", index, tag);
    int return_temp;
    /*
       iterate over the set if the first block is valid and check if the tags match
       if the tags match hit flag is set to 1 and we exit the loop
       'set_index' will be set to the set number that contains the correct address
       before we check that the tag matches we check if the index is valid
       if it is not valid then it is empty and the entry is missing
       we can put the missing entry there
       */
    for(set_index = 0; set_index < cp->assoc; set_index++)
    {
        if(set[set_index].valid == 0)
            break;

        if(set[set_index].tag == tag)
        {
            /*
            if(next_cp == NULL && L2size == 0)
                L1hits++;
            else if(next_cp != NULL)
                L1hits++;
            else if(next_cp == NULL && L2size > 0)
                L2hits++;
            */
            hflag = 1;
            break;
        }

    }

    /*if(set_index == cp->assoc)
    {
        if(next_cp == NULL && L2size == 0)
            L1misses++;
        else if(next_cp != NULL)
            L1misses++;
        else if(next_cp == NULL && L2size > 0)
            L2misses++;

    }*/

    //execute this code if we are trying to read from the specified address
    if(access_type == 'r')
    {
        //if there was a hit when searching the sets, set the time stamp to the current cycle and return the latency for this level of cache
        if(hflag)
        {
            if(L2size == 0 || (next_cp != NULL)){
                L1hits++;
            }
            else{
                L2hits++;
            }
            
            set[set_index].ts = now;
            //printf("return: %d\n", cp->hit_latency);
            return cp->hit_latency;
        }
        else
        {
            //if it is not a hit, check if every set was valid
            //if not, set_index = the set number that is empty, simply place the missing address into that set in the index
            if(L2size == 0 || (next_cp != NULL)){
                L1misses++;
            }
            else{
                L2misses++;
            }
            
            if(set_index < cp->assoc)
            {
                set[set_index].valid = 1;
                set[set_index].dirty = 0;
                set[set_index].tag   = tag;
                set[set_index].ts    = now;
                // we need to also do this in L2 cache, call cache_access for L2

                if(next_cp == NULL)
                {
                    //printf("return: %d\n", cp->hit_latency + mem_latency);
                    return cp->hit_latency + mem_latency;
                }
                else
                {
                    return_temp = cache_access(next_cp, address, access_type, now, NULL);
                    //printf("return: %d\n", cp->hit_latency + return_temp);
                    return cp->hit_latency + return_temp;
                }
            }
            else //if every index is valid
            {
                // need to find index with smallest timestamp and evict it
                unsigned long long t = now;
                int i = 0, j = 0;
                for(i = 0; i < cp->assoc; i++)
                {
                    blk = set[i];
                    if(t > blk.ts)
                    {
                        t = blk.ts;
                        j = i;
                    }
                }
                if(set[j].dirty = 1)
                {
                    if(next_cp != NULL)
                    {
                        unsigned long next_address = set[j].tag << (32-tag_bits);
                        unsigned long temp = index << (int)log2(cp->blocksize);
                        next_address = next_address | temp;

                        /*
                           unsigned int next_index_bits = log2(next_cp->nsets);
                           unsigned int next_tag_bits = next_index_bits + log2(next_cp->blocksize) + next_cp->assoc;
                           unsigned int next_index = next_address << next_tag_bits;
                           next_index = next_index >> (32 - next_index_bits);
                           unsigned int next_tag = next_address >> (next_index_bits + ((int)log2(next_cp->blocksize)));
                           struct cache_blk_t * next_set = next_cp->blocks[next_index];
                           int next_set_index;
                           for(next_set_index = 0; next_set_index < next_cp->assoc; next_set_index++)
                           {
                           if(next_set[next_set_index].tag == tag)
                           {
                           next_set[next_set_index].dirty = 1;
                           break;
                           }
                           }
                           */

                        struct cache_blk_t * L2_blk = find_blk(next_cp, next_address);

                        L2_blk->dirty = 1;
                        L2_blk->ts = now;


                        set[j].valid = 1;
                        set[j].dirty = 0;
                        set[j].tag   = tag;
                        set[j].ts    = now;
                        // we need to also do this in L2 cache, call cache_access for L2
                        //my be wrong
                        return_temp = cache_access(next_cp, address, access_type, now, NULL);
                        //printf("return: %d\n", cp->hit_latency + return_temp);
                        return cp->hit_latency + return_temp;
                    }
                    else
                    {
                        //replace address
                        set[j].valid = 1;
                        set[j].dirty = 0;
                        set[j].tag   = tag;
                        set[j].ts    = now;

                        //printf("return: %d\n", cp->hit_latency + mem_latency);
                        return cp->hit_latency + mem_latency;
                    }

                }
                else
                {
                    //replace address
                    set[j].valid = 1;
                    set[j].dirty = 0;
                    set[j].tag   = tag;
                    set[j].ts    = now;

                    if(next_cp != NULL)
                    {
                        //might not be right
                        return_temp = cache_access(next_cp, address, access_type, now, NULL);
                        //printf("return: %d\n", cp->hit_latency + return_temp);
                        return cp->hit_latency + return_temp;
                    }
                    else
                    {
                        //printf("return: %d\n", cp->hit_latency + mem_latency);
                        return cp->hit_latency + mem_latency;
                    }
                }

                // check timestamps and kick something out
            }
        }
        // in 2 level cache scheme, we will need to check if the next cache level variable is null
        // if it is null (we are in L2 cache) we return L2 latency and possibly memory latency
    }
    else
    {
        if(hflag)
        {
            set[set_index].ts = now;
            set[set_index].dirty = 1;
            if(L2size == 0 || (next_cp != NULL)){
                L1hits++;
            }
            else{
                L2hits++;
            }
            //printf("return: %d\n", cp->hit_latency);
            return cp->hit_latency;
        }
        else
        {
            if(L2size == 0 || (next_cp != NULL)){
                L1misses++;
            }
            else{
                L2misses++;
            }
            
            if(set_index < cp->assoc)
            {
                set[set_index].valid = 1;
                set[set_index].dirty = 0;
                set[set_index].tag   = tag;
                set[set_index].ts    = now;
                // we need to also do this in L2 cache, call cache_access for L2
                if(next_cp == NULL)
                {
                    //printf("return: %d\n", cp->hit_latency + mem_latency);
                    return mem_latency + cp->hit_latency;
                }
                else
                {
                    return_temp = cache_access(next_cp, address, access_type, now, NULL);
                    //printf("return: %d\n", cp->hit_latency + return_temp);
                    return cp->hit_latency + return_temp;
                }


            }
            else //if we have to evict something while writing
            {
                // need to find index with smallest timestamp and evict it
                unsigned long long t = now;
                int i = 0, j = 0;
                for(i = 0; i < cp->assoc; i++)
                {
                    blk = set[i];
                    if(t > blk.ts)
                    {
                        t = blk.ts;
                        j = i;
                    }
                }

                //if oldest timestamp is dirty gotta write back to next level
                if(set[j].dirty = 1)
                {
                    if(next_cp != NULL)
                    {
                        unsigned long next_address = set[j].tag << (32-tag_bits);
                        unsigned long temp = index << (int)log2(cp->blocksize);
                        next_address = next_address | temp;

                        /*
                           unsigned int next_index_bits = log2(next_cp->nsets);
                           unsigned int next_tag_bits = next_index_bits + log2(next_cp->blocksize) + next_cp->assoc;
                           unsigned int next_index = next_address << next_tag_bits;
                           next_index = next_index >> (32 - next_index_bits);
                           unsigned int next_tag = next_address >> (next_index_bits + ((int)log2(next_cp->blocksize)));
                           struct cache_blk_t * next_set = next_cp->blocks[next_index];
                           int next_set_index;
                           for(next_set_index = 0; next_set_index < next_cp->assoc; next_set_index++)
                           {
                           if(next_set[next_set_index].tag == tag)
                           {
                           next_set[next_set_index].dirty = 1;
                           break;
                           }
                           }
                           */

                        struct cache_blk_t * L2_blk = find_blk(next_cp, next_address);

                        L2_blk->dirty = 1;
                        L2_blk->ts = now;

                        set[j].valid = 1;
                        set[j].dirty = 0;
                        set[j].tag   = tag;
                        set[j].ts    = now;
                        // we need to also do this in L2 cache, call cache_access for L2
                        //my be wrong
                        return_temp = cache_access(next_cp, address, access_type, now, NULL);
                        //printf("return: %d\n", cp->hit_latency + return_temp);
                        return cp->hit_latency + return_temp;
                    }
                    else
                    {
                        //replace address
                        set[j].valid = 1;
                        set[j].dirty = 0;
                        set[j].tag   = tag;
                        set[j].ts    = now;
                        //printf("return: %d\n", cp->hit_latency + mem_latency);
                        return cp->hit_latency + mem_latency;
                    }

                }
                else
                {
                    //replace address
                    set[j].valid = 1;
                    set[j].dirty = 0;
                    set[j].tag   = tag;
                    set[j].ts    = now;

                    if(next_cp != NULL)
                    {
                        //might not be right
                        return_temp = cache_access(next_cp, address, access_type, now, NULL);
                        //printf("return: %d\n", cp->hit_latency + return_temp);
                        return cp->hit_latency + return_temp;
                    }
                    else
                    {
                        //printf("return: %d\n", cp->hit_latency + mem_latency);
                        return cp->hit_latency + mem_latency;
                    }
                }
            }
        }

    }

    return(cp->hit_latency);
}

struct cache_blk_t * find_blk(struct cache_t *cp, unsigned long address)
{
    unsigned int index_bits = log2(cp->nsets);
    //printf("index_bits %d\n", index_bits);
    //printf("blocksize %d\n", cp->blocksize);
    unsigned int tag_bits = 32 - (index_bits + log2(cp->blocksize));//removed + 2
    //printf("tag_bits %d\n", tag_bits);
    unsigned int index = address << tag_bits;
    //printf("address %d\n", address);
    //printf("index1 %d\n", index);
    index = index >> (32 - index_bits);
    //printf("index %d\n", index);
    unsigned int tag = address >> (index_bits + ((int)log2(cp->blocksize))); // removed + 2
    //printf("tag %d\n", tag);
    struct cache_blk_t * set = cp->blocks[index];
    int set_index;
    //fflush(stdout);
    //printf("Index: %d, Tag: %d\n", index, tag);

    for(set_index = 0; set_index < cp->assoc; set_index++)
    {
        if(set[set_index].tag == tag)
        {
            break;
        }
    }
    return &set[set_index];
}

int trace_get_item(struct trace_item **item)
{
    int n_items;

    if (trace_buf_ptr == trace_buf_end) {   /* if no more unprocessed items in the trace buffer, get new data  */
        n_items = fread(trace_buf, sizeof(struct trace_item), TRACE_BUFSIZE, trace_fd);
        if (!n_items) return 0;                         /* if no more items in the file, we are done */

        trace_buf_ptr = 0;
        trace_buf_end = n_items;                        /* n_items were read and placed in trace buffer */
    }

    *item = &trace_buf[trace_buf_ptr];      /* read a new trace item for processing */
    trace_buf_ptr++;

    if (is_big_endian()) {
        (*item)->PC = my_ntohl((*item)->PC);
        (*item)->Addr = my_ntohl((*item)->Addr);
    }

    return 1;
}

uint32_t my_ntohl(uint32_t x)
{
    u_char *s = (u_char *)&x;
    return (uint32_t)(s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]);
}


int is_big_endian(void)
{
    union {
        uint32_t i;
        char c[4];
    } bint = { 0x01020304 };

    return bint.c[0] == 1;
}

void trace_init()
{
    trace_buf = malloc(sizeof(struct trace_item) * TRACE_BUFSIZE);

    if (!trace_buf) {
        fprintf(stdout, "** trace_buf not allocated\n");
        exit(-1);
    }

    trace_buf_ptr = 0;
    trace_buf_end = 0;
}

void trace_uninit()
{
    free(trace_buf);
    fclose(trace_fd);
}

int my_trace_get_item(struct trace_item **item)
{
    unsigned int type;
    char PC[9];
    unsigned int dreg;
    unsigned int areg;
    unsigned int breg;
    char addr[9];

    *item = &trace_buf[trace_buf_ptr];
    //fflush(stdout);
    //printf("dont got shit\n");

    fscanf(trace_fd, "%u %s %u %u %u %s", &type, PC, &dreg, &areg, &breg, addr);

    //fflush(stdout);
    //printf("got shit\n");

    (*item)->type = type;
    (*item)->PC   = strtol(PC, NULL, 16);
    (*item)->dReg = dreg;
    (*item)->sReg_a = areg;
    (*item)->sReg_b = breg;
    (*item)->Addr   = strtol(addr, NULL, 16);

    return 0;
}
