/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]){
	if(N == 32){
		for(int i = 0; i < N; i += 8){
			for(int j = 0; j < M; j += 8){
				for(int p = 0; p < 8; p ++){
					int tmp;
					for(int q = 0; q < 8; q ++){
						if(i + p != q + j) B[q + j][p + i] = A[p + i][q + j];
						else tmp = A[p + i][q + j];
					}
					if(i == j) B[p + j][p + i] = tmp;
				}
			}
		}
	}else if(N == 64){
		// freopen("out.txt","w",stdout);
		// printf("NASDA %ld\n",(int*)B - (int*)A);
		int i,j,p,q,u,tmp;
		for(i = 0; i < N; i += 8){
			for(j = 0; j < M; j += 8){
				if(i == j || i == 0 || j == 0) continue;
				u = 0;
				for(p = 0; p < 8; p ++){
					for(q = 0; q < 4; q ++) B[q + j][p + i] = A[p + i][q + j];
					for(q = 4; q < 8; q ++) B[q - 4 + u][p + u] = A[p + i][q + j];
				}
				for(q = 4; q < 8; q ++) for(p = 0; p < 8; p ++)	B[q + j][p + i] = B[q - 4 + u][p + u];
			}
		}
		for(i = 0; i < N; i += 8){
			for(j = 0; j < M; j += 8){
				if(i == j || (i != 0 && j != 0) || i == 48 || j == 48) continue;
				u = 48;
				for(p = 0; p < 8; p ++){
					for(q = 0; q < 4; q ++) B[q + j][p + i] = A[p + i][q + j];
					for(q = 4; q < 8; q ++) B[q - 4 + u][p + u] = A[p + i][q + j];
				}
				for(q = 4; q < 8; q ++) for(p = 0; p < 8; p ++)	B[q + j][p + i] = B[q - 4 + u][p + u];
			}
		}
		for(i = 0; i < N; i += 8){
			for(j = 0; j < M; j += 8){
				if(!(i == 0 && j == 48) && !(i == 48 && j == 0)) continue;
				u = 56;
				for(p = 0; p < 8; p ++){
					for(q = 0; q < 4; q ++) B[q + j][p + i] = A[p + i][q + j];
					for(q = 4; q < 8; q ++) B[q - 4 + u][p + u] = A[p + i][q + j];
				}
				for(q = 4; q < 8; q ++) for(p = 0; p < 8; p ++)	B[q + j][p + i] = B[q - 4 + u][p + u];
			}
		}
		for(i = 0; i < N; i += 8){
			if(i == 56) continue;
			u = 56,j = i;
			for(p = 0; p < 8; p ++){
				for(q = 0; q < 4; q ++){
					if(p == q || p == q + 4) tmp = A[p + i][q + j];
					else B[q + j][p + i] = A[p + i][q + j];
				}
				for(q = 4; q < 8; q ++) B[q - 4 + u][p + u] = A[p + i][q + j];
				if(p < 4) B[p + j][p + i] = tmp;
				else B[p - 4 + j][p + i] = tmp;
			}
			for(q = 4; q < 8; q ++) for(p = 0; p < 8; p ++)	B[q + j][p + i] = B[q - 4 + u][p + u];
		}
		i = 56,j = 56;
		for(p = 0; p < 8; p ++){
			for(q = 0; q < 8; q ++){
				B[q + j][p + i] = A[p + i][q + j];
			}
		}
	}else{
		for(int i = 0; i < N; i += 14){
			for(int j = 0; j < M; j += 3){
				for(int p = 0; p + i < N && p < 14; p ++){
					// int tmp;
					for(int q = 0; q + j < M && q < 3; q ++){
						// if(i + p != q + j) 
						B[q + j][p + i] = A[p + i][q + j];
						// else tmp = A[p + i][q + j];
					}
					// if(i == j) B[p + j][p + i] = tmp;
				}
			}
		}
	}
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

