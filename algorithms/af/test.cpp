#include <algorithm>
#include <fstream>
#include <sys/time.h>
#include <time.h>

#include "utils/interface.h"

using namespace std;

int main() {
    printf("\n");
    void* ind = NULL;

    int N = (int)2000;
    int *frec = new int[300];
    int a = 255;
    unsigned char* T = new unsigned char[N+1];
    unsigned char* Tc = new unsigned char[N+1];
    for (int i = 0; i < N; i++) {
        T[i] = Tc[i] = (unsigned char)(rand() % a);
    }
    T[N] = '\0';
    
    for (int i = 0; i < 300; i++) frec[i]=0;
    for (int i = 0; i < N; i++) frec[T[i]]++;
    for (int i = 0; i < 260; i=i+2) printf("[%d,%d] [%d,%d]\n", i,frec[i],i+1,frec[i+1]);
    delete [] frec;

    printf("building\n");
    int error = build_index(T, N, "free_text", &ind);
    if (error) {
        fprintf(stderr, "error during construction: %d\n", error);
        exit(13);
    }

    printf("searching\n");
    for (int q = 0; q < 1000; q++) {
        int M = 10;
        int p = rand() % (N - M);
        unsigned char* P = (unsigned char*)malloc(sizeof(unsigned char)*(M+1));
        for (int i = 0; i < M; i++) {
            P[i] = Tc[p+i];
        }
        P[M] = '\0';
        unsigned long* occ = NULL;
        unsigned long numocc;
        locate(ind, P, M, &occ, &numocc);
        if (numocc == 0) {
            printf("Could not find!\n");
        }
        if (occ) free(occ);
    }
    printf("done\n");

    //delete[] T;
}
