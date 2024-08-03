#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(){

    int p1[2], p2[2];
    pipe(p1);  // from parent to child
    pipe(p2);  // from child to parent 
    char buf[64] = {};
    if(fork() == 0){
        if(read(p1[0], buf, 4) != 4){
	    fprintf(2, "child read error\n");
	    exit(1);
	}
        close(p1[0]);
        fprintf(0, "%d: received ping\n", getpid());

	if(write(p2[1], "pong", 4) != 4){
	     fprintf(2, "child write error\n");
             exit(1);
        }
        close(p2[1]);
    }  
    else{
	if(write(p1[1], "ping", 4) != 4){
	    fprintf(2, "parent write error\n");
            exit(1);
        }	
        close(p1[1]);
        if(read(p2[0], buf, 4) != 4){
	    fprintf(2, "parent read error\n");
            exit(1);
        }	
        fprintf(0, "%d: received pong\n", getpid());
        close(p2[0]);
    }	
    exit(0);
}

