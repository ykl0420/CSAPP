#include <stdio.h>
#include <string.h>
char s[100000],t[100000];
int main(){
	while(fgets(s,sizeof(s),stdin) != NULL){
		int n = strlen(s);
		int m = 0;
		t[m] = '\0';
		for(int i = 0; i < n; i ++){
			if(m && t[m - 1] == '(' && s[i] != ')') continue;
			t[m ++] = s[i];
		}
		t[m] = '\0';
		printf("%s",t);
	}
	return 0;
}
