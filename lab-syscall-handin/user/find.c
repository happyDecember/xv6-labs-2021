#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int match(const char *s, const char *pattern)
{
    for (int i = 0; i <= strlen(s) - strlen(pattern); i++)
	for (int j = 0; j < strlen(pattern); j++)
        {
	    if (s[i + j] != pattern[j])
                break;
	    if (j == strlen(pattern) - 1)
		return 1;
     }
    return 0;
}

void find(char *current_pos, char *target)
{
    int fd;
    char buf[512] = {}, *p;
    struct stat st;
    struct dirent de;

    if ((fd = open(current_pos, 0)) < 0)
    {
	fprintf(2, "this position is wrong.");
        exit(1);
    }
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "this position can not stat.");
        close(fd);
        exit(1);
    }	

    switch (st.type)
    {
    case T_FILE:
        if (match(current_pos, target))
	    printf("%s\n", current_pos);
        break;
    case T_DIR:
        strcpy(buf, current_pos);
        p = buf + strlen(buf);
	*p++ = '/';
	while (read(fd, &de, sizeof(de)) == sizeof(de)){
	    if (de.inum == 0)
	        continue;
            strcpy(p, de.name); 
            if (strcmp(p, "..") == 0 || strcmp(p, ".") == 0)
	        continue;
            find(buf, target);
        }
        break;
    }
    close(fd);
}

int main(int argc, char **argv)
{
    if (argc < 2){
        printf("please give the current position and the target file name.");
        exit(1);
    }
    find(argv[1], argv[2]);
    return 0;
}    
























    	    
