#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
const int N = 35;

void input(int p[2], int list[], int length){
    for(int i = 0; i < length; i++)
        if(write(p[1], (list + i), sizeof(list[i])) < sizeof(list[i])){
            fprintf(2, "write error in pipe input.");
            exit(1);
        }
    close(p[1]);
    close(p[1]); 
}

int output(int p[2], int list[]){
    close(p[1]);
    int index = 0, front, len = read(p[0], &front, sizeof(len));
    if(len == 0)
	return 0;
    if(len < sizeof(len)){
	fprintf(2, "read error in pipe output.");
        exit(1);
    }
    fprintf(0, "prime %d\n", front);
    while((len = read(p[0], index + list, sizeof(len))) == sizeof(len)){
	if(list[index] % front != 0)
	    index++;
    }
    close(p[0]);
    return index;
}

void process(int p[2]){
    int num[N], length = output(p, num), next[2];
    if(length == 0)
	exit(0);
    pipe(next);
    input(next, num, length);
    if(fork() == 0)
	process(next);
}

int main(){
    int p[2], num[N];
    pipe(p);

    if(fork() == 0){
	process(p);
	wait(0);
	exit(0);
    }	
    else{
	for(int i = 2, j = 0; i <= N; i++, j++){
	    num[j] = i;
        }
        input(p, num, N - 1);
    }
    wait(0);
    exit(0);
}    





















