#include <stdio.h>
#include <stdlib.h>
#include "cachelab.h"
#define ull unsigned long long
unsigned Tm, S, s, E, b, hit_cnt, miss_cnt, evict_cnt;
unsigned **tm;
ull **st;
void access(ull x){
	unsigned ind = x >> b & (S - 1), mn = 1e9, pos = -1;
	x >>= s + b;
	for(int i = 0; i < E; i ++){
		if(!(st[ind][i] ^ x)){
			tm[ind][i] = Tm, hit_cnt ++;
			return;
		}
		if(tm[ind][i] <= mn) pos = i, mn = tm[ind][i];
	}
	miss_cnt ++;
	if(!(st[ind][pos] >> 31)) evict_cnt ++;
	st[ind][pos] = x, tm[ind][pos] = Tm;
}
int main(int argc,char *argv[]){
	// printf("%d\n",argc);
	// printf("%s %s %s %s\n",argv[2],argv[4],argv[6],argv[8]);
	sscanf(argv[2],"%u",&s);
	sscanf(argv[4],"%u",&E);
	sscanf(argv[6],"%u",&b);
	freopen(argv[8],"r",stdin);
	S = 1u << s;
	st = malloc(S * sizeof(ull*));
	tm = malloc(S * sizeof(unsigned*));
	for(int i = 0; i < S; i ++){
		st[i] = malloc(E * sizeof(ull));
		for(int j = 0; j < E; j ++) st[i][j] = 1u << 31;
	}
	for(int i = 0; i < S; i ++){
		tm[i] = malloc(E * sizeof(unsigned));
		for(int j = 0; j < E; j ++) tm[i][j] = 0;
	}
	char op;
	ull x;
	int foo;
	while(scanf(" %c %llx,%d", &op, &x, &foo) != -1){
		printf("%c %llx\n", op, x);
		++Tm;
		if(op == 'I') continue;
		if(op != 'S') access(x);
		if(op != 'L') access(x);
	}
    printSummary(hit_cnt, miss_cnt, evict_cnt);
    for(int i = 0; i < S; i ++) free(st[i]);
    for(int i = 0; i < S; i ++) free(tm[i]);
    free(st),free(tm);
    return 0;
}
