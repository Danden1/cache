#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt()

#define BYTES_PER_WORD 4
// #define DEBUG

/*
 * Cache structures
 */

struct cline{
	int age;
	int valid;
	short* modified;
	uint32_t tag;
};

struct cset{
	struct cline *lines;
};

struct cache{
	int s; //set
	int E; //associativity
	int b; //block
	struct cset *sets;
};


int time = 0;
//cache
struct cache simCache;
// counts
int reads=0, writes=0, writeback=0;
int readhit=0, writehit=0;
int readmiss=0, writemiss = 0;

static int index_bit(int n) {
	int cnt = 0;
	while(n){
		cnt++;
		n = n >> 1;
	}
	return cnt-1;
}



void build_cache(int set, int way,int block) {
	int i = 0;
	int j = 0;
	simCache.s = index_bit(set);
	simCache.E = index_bit(way);
	simCache.b = index_bit(block);

	simCache.sets = (struct cset*)malloc(sizeof(struct cset)*set);

	for(i = 0; i < set; i++){
		simCache.sets[i].lines = (struct cline*)malloc(sizeof(struct cline)*way);
		memset(simCache.sets[i].lines, 0, way*sizeof(struct cline));
		for(j = 0;j < block; j++){
			simCache.sets[i].lines[j].modified = (short*)malloc(sizeof(short)*block);
			memset(simCache.sets[i].lines[j].modified, 0, block*sizeof(short));
		}
	}

}



void access_cache(char* op, uint32_t addr) {
	uint32_t tag, set;
	int i = 0, index = 0, min_val = 987654321;
	short  way_flag = 1, writeback_cnt = 0;
	uint32_t tmp_addr, blocksize,waysize;
	blocksize = 1 << simCache.b;
	waysize = 1<<simCache.E;
	tag = addr >> (simCache.s+simCache.b);
	set = addr << (32-simCache.s-simCache.b) >> (32-simCache.s);

	time+=1;

	if(strncmp(op,"R",1) == 0){
		reads+=1;
		for(i = 0; i < waysize; i++){
				if(simCache.sets[set].lines[i].tag == tag){
					way_flag = 0;
					if(simCache.sets[set].lines[i].valid == 1){
						readhit+=1;
						simCache.sets[set].lines[i].age = time;
						break;
					}
					else{
						readmiss+=1;
						simCache.sets[set].lines[i].tag = tag;
						simCache.sets[set].lines[i].modified[addr%blocksize] = 0;
						simCache.sets[set].lines[i].valid = 1;
						simCache.sets[set].lines[i].age = time;
						break;
					}
				}
		}
		if(way_flag){
			for(i = 0; i < waysize; i++){
				if(min_val > simCache.sets[set].lines[i].age){
					min_val = simCache.sets[set].lines[i].age;
					index = i;
				}
			}


		for( i = 0; i < blocksize; i++){
			if(simCache.sets[set].lines[index].modified[i] == 1){
				writeback_cnt=1;
			}
		}
		writeback+=writeback_cnt;


		simCache.sets[set].lines[index].tag = tag;
		memset(simCache.sets[set].lines[index].modified,0,sizeof(short)*blocksize);
		simCache.sets[set].lines[index].valid = 1;
		simCache.sets[set].lines[index].age = time;
		readmiss+=1;
		}
	}
	else if(strncmp(op,"W",1) == 0){
		writes+=1;
		for(i = 0; i < waysize; i++){
				if(simCache.sets[set].lines[i].tag == tag){
					way_flag = 0;
					if(simCache.sets[set].lines[i].valid == 1){
						writehit+=1;
						simCache.sets[set].lines[i].age = time;
						simCache.sets[set].lines[i].modified[addr%blocksize] = 1;
						break;
					}
					else{
						writemiss+=1;
						simCache.sets[set].lines[i].tag = tag;
						simCache.sets[set].lines[i].modified[addr%blocksize] = 1;
						simCache.sets[set].lines[i].valid = 1;
						simCache.sets[set].lines[i].age = time;
						break;
					}
				}
		}
		if(way_flag){
			for(i = 0; i < waysize; i++){
				if(min_val > simCache.sets[set].lines[i].age){
					min_val = simCache.sets[set].lines[i].age;
					index = i;
				}
			}

		for( i = 0; i < blocksize; i++){
			if(simCache.sets[set].lines[index].modified[i] == 1){
				writeback_cnt=1;
			}
		}
		writeback +=writeback_cnt;
		memset(simCache.sets[set].lines[index].modified,0,sizeof(short)*blocksize);
		simCache.sets[set].lines[index].tag = tag;
		simCache.sets[set].lines[index].modified[addr%blocksize] = 1;
		simCache.sets[set].lines[index].valid = 1;
		simCache.sets[set].lines[index].age = time;
		writemiss+=1;
		}
	}

}

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/// ...
        // access_cache()
        // ...
void cdump(int capacity, int assoc, int blocksize){

	printf("Cache Configuration:\n");
	printf("-------------------------------------\n");
	printf("Capacity: %dB\n", capacity);
	printf("Associativity: %dway\n", assoc);
	printf("Block Size: %dB\n", blocksize);
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat		                           */
/*                                                             */
/***************************************************************/
void sdump() {
	printf("Cache Stat:\n");
    	printf("-------------------------------------\n");
	printf("Total reads: %d\n", reads);
	printf("Total writes: %d\n", writes);
	printf("Write-backs: %d\n", writeback);
	printf("Read hits: %d\n", readhit);
	printf("Write hits: %d\n", writehit);
	printf("Read misses: %d\n", readmiss);
	printf("Write misses: %d\n", writemiss);
	printf("\n");
}


/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/* 					                            		       */
/* Cache Design						                           */
/*  							                               */
/* 	    cache[set][assoc][word per block]		               */
/*                                						       */
/*      				                        		       */
/*       ----------------------------------------	           */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*                              						       */
/*                                                             */
/***************************************************************/
void xdump(struct cache* L)
{
	int i,j,k = 0;
	int b = L->b, s = L->s;
	int way = 1<<L->E, set = 1 << s;
	int E = index_bit(way);

	uint32_t line;

	printf("Cache Content:\n");
    	printf("-------------------------------------\n");
	for(i = 0; i < way;i++)
	{
		if(i == 0)
		{
			printf("    ");
		}
		printf("      WAY[%d]",i);
	}
	printf("\n");

	for(i = 0 ; i < set;i++)
	{
		printf("SET[%d]:   ",i);
		for(j = 0; j < way;j++)
		{
			if(k != 0 && j == 0)
			{
				printf("          ");
			}
			if(L->sets[i].lines[j].valid){
				line = L->sets[i].lines[j].tag << (s+b);
				line = line|(i << b);
			}
			else{
				line = 0;
			}
			printf("0x%08x  ", line);
		}
		printf("\n");
	}
	printf("\n");
}




int main(int argc, char *argv[]) {
	int i, j, k;
	int capacity=1024;
	int way=8;
	int blocksize=8;
	int set;

	// Input option
	int opt = 0;
	char* token;
	int xflag = 0;

	// parse file
	char *trace_name = (char*)malloc(32);
	FILE *fp;
    char line[16];
    char *op;
    uint32_t addr;

    /* You can define any variables that you want */

	trace_name = argv[argc-1];
	if (argc < 3) {
		printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n",argv[0]);
		exit(1);
	}
	while((opt = getopt(argc, argv, "c:x")) != -1){
		switch(opt){
			case 'c':
                // extern char *optarg;
				token = strtok(optarg, ":");
				capacity = atoi(token);
				token = strtok(NULL, ":");
				way = atoi(token);
				token = strtok(NULL, ":");
				blocksize  = atoi(token);
				break;
			case 'x':
				xflag = 1;
				break;
			default:
			printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n",argv[0]);
			exit(1);

		}
	}

	// allocate
	set = capacity/way/blocksize;

    /* TODO: Define a cache based on the struct declaration */
    build_cache(set,way,blocksize);

	// simulate
	fp = fopen(trace_name, "r"); // read trace file
	if(fp == NULL){
		printf("\nInvalid trace file: %s\n", trace_name);
		return 1;
	}
	cdump(capacity, way, blocksize);

    /* TODO: Build an access function to load and store data from the file */
    while (fgets(line, sizeof(line), fp) != NULL) {
        op = strtok(line, " ");
        addr = strtoull(strtok(NULL, ","), NULL, 16);

#ifdef DEBUG
        // You can use #define DEBUG above for seeing traces of the file.
        fprintf(stderr, "op: %s\n", op);
        fprintf(stderr, "addr: %x\n", addr);
#endif
        access_cache(op, addr);
    }

    // test example
	sdump();
	if (xflag){
	    	xdump(&simCache);
	}

    	return 0;
}
