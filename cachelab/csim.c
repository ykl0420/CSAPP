#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "cachelab.h"
#define ull unsigned long long
static unsigned Tm, S, s, E, b, hit_cnt, miss_cnt, evict_cnt;
typedef struct{
	ull state; // state = (valid_bit << 63) | tag 
	unsigned time;
}node;
void access(node **cache,ull x){
	unsigned ind = x >> b & (S - 1), mn = 1e9, pos = -1;
	x >>= s + b;
	for(int i = 0; i < E; i ++){
		if(!(cache[ind][i].state ^ x)){
			cache[ind][i].time = Tm, hit_cnt ++;
			return;
		}
		if(cache[ind][i].time <= mn) pos = i, mn = cache[ind][i].time;
	}
	miss_cnt ++;
	if(!(cache[ind][pos].state >> 63)) evict_cnt ++;
	cache[ind][pos].state = x, cache[ind][pos].time = Tm;
}
node** init(){
	S = 1u << s;
	node** cache = malloc(S * sizeof(node*));
	for(int i = 0; i < S; i ++){
		cache[i] = malloc(E * sizeof(node));
		for(int j = 0; j < E; j ++) cache[i][j].state = 1ull << 63,cache[i][j].time = 0;
	}
	return cache;
}
void destroy(FILE* data_file, node **cache){
    for(int i = 0; i < S; i ++) free(cache[i]);
    free(cache);
    fclose(data_file);
}
FILE* parse(int argc,char *argv[]){
	FILE* data_file = NULL;
	int opt;
	while((opt = getopt(argc,argv,"hvs:E:b:t:")) != -1){
		switch(opt){
			case 'h':
			case 'v':
				break; // both not implement
			case 's':
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				data_file = fopen(optarg,"r");
				break;
			case '?':
				// don't know how to handle
				break;
		}
	}
	if(data_file == NULL){
		// ?
	}
	return data_file;
}
void process(FILE* data_file, node **cache){
	char op; ull x;
	while(fscanf(data_file, " %c %llx,%*d", &op, &x) != -1){
		// printf("%c %llx\n", op, x);
		++Tm;
		if(op == 'I') continue;
		if(op != 'S') access(cache,x); // "Load"
		if(op != 'L') access(cache,x); // "Store"
	}
}
int main(int argc,char *argv[]){
	FILE* data_file = parse(argc,argv);
	node **cache = init();
	process(data_file, cache);
    printSummary(hit_cnt, miss_cnt, evict_cnt);
    destroy(data_file, cache);
    return 0;
}
