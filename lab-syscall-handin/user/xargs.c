#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

const int ARG_SIZE = 64;

char *getWord()
{
    char buf;
    char *ret = (char *)malloc(ARG_SIZE), *p = ret;
    while (read(0, &buf, sizeof(char)) == sizeof(char)){
	if (buf == ' ' || buf == '\n')
	     break;
        else
	    *p++ = buf;
    }
    *p++ = '\0';
    return ret;
}


char **getInstruction(int argc, char **argv, int mode)
{
    char **ret = (char **)malloc(MAXARG * sizeof(char *)), **p = ret + argc;
    for (int i = 0; i < argc; i++){
	ret[i] = (char *)malloc(ARG_SIZE);
        strcpy(ret[i], argv[i]);
    }
    int i = 0;
    while (1){
	 p[i++] = getWord();
         if (strlen(p[i - 1]) == 0 || mode == i || argc + i == MAXARG)
	     break;
    }
    p[i] = (char *)malloc(sizeof(char));
    p[i] = "";
    return ret;
}

int getInstructionLength(char **list)
{
    char **p = list;
    while (strlen(*p++) > 0);
    return p - list - 1;

}
	
void freeInstruction(int length, char **list)
{
    for (int i = 0; i < length; i++)
        free(list[i]);
    free(list);
}

void printInstruction(char **list)
{
    char **p = list;
    while (strlen(*p) > 0){
	fprintf(2, *p++);
        fprintf(2, " ");
    }
    fprintf(2, "\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(2, "please input essential args.");
        exit(1);
    }

    int flag = 1, mode, dx;
    if (argc < 3 || strcmp(argv[1], "-n") != 0){
	dx = 1;
        mode = MAXARG;
    }
    else
    {
	dx = 3;
        mode = atoi(argv[2]);	
    }

    for (int i = dx; i < argc; i++)
	argv[i - dx] = argv[i];
    argc -= dx; 

    while (flag){
	char **list = getInstruction(argc, argv, mode);
        int length = getInstructionLength(list);
        if (length != argc){
	    if (fork() == 0)
	    {
	        exec(list[0], list);
                exit(0);
            }	
            else
	    {
	        wait(0);
            }
        }	
        else
            flag = 0;
        freeInstruction(length, list);
    }
    return 0;
}    












	
