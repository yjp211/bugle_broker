#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>

 
#include "include.h"
#include "log.h"


 

char *log_file_name;
char *old_log_file_name;
int max_log_file_size;
int fd_log_file = -1;

int max_debug_level = 0;


int debug_log(int level, const char *fmt, ...)
{
    time_t log_time = time(NULL);
    int len1, len2;
    char buf[1024];
    va_list ap;

    if(level > max_debug_level) {
        return 0;
    }

    len1 = strftime(buf,  sizeof(buf), "[%Y-%m-%d %H:%M:%S] ", localtime(&log_time));
    va_start(ap, fmt);
    len2 = vsnprintf(buf + len1, sizeof(buf) - len1, fmt, ap);
    va_end(ap);

    buf[len1 + len2] = '\0';

    fprintf(stdout, "%s", buf);

    return 0;
}

int get_file_size()
{
    int ret = 0;
    struct stat buf;

    ret = stat(log_file_name, &buf);
    if (ret){
        return -1;

    }

    return buf.st_size;
}

int init_log(char *file_name, int file_size, int debug_level)
{
    int file_name_buf_len;

    max_log_file_size = file_size;
    max_debug_level = debug_level;

    file_name_buf_len = strlen(file_name) + 16;
    log_file_name = malloc(file_name_buf_len);
    old_log_file_name = malloc(file_name_buf_len);

    if ((NULL == log_file_name)
        || (NULL == old_log_file_name)) {
        fprintf(stderr, "malloc failed, %m\n");
        return -1;
    }
    
    sprintf(log_file_name, "%s", file_name);
    sprintf(old_log_file_name, "%s.old", file_name);
      

    fd_log_file = open(log_file_name,
            O_CREAT|O_WRONLY|O_APPEND,
            S_IRUSR | S_IWUSR | S_IRGRP);

    if (fd_log_file < 0){
        fprintf(stderr, "open %s failed, %m\n", log_file_name);
        return -1;
    }

    return 0;
}

void close_file()
{
    if (fd_log_file > 0){
         close(fd_log_file);
    }
    fd_log_file = -1;
}

int write_file(void *buf, int len)
{
    //int file_size;
    int ret;
    
    /*
    file_size = get_file_size();
    
    if (file_size >= max_log_file_size){
        unlink(old_log_file_name);
        close_file();

        rename(log_file_name, old_log_file_name);

        fd_log_file = open(log_file_name,
            O_CREAT|O_WRONLY|O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP);

        if (fd_log_file < 0){
            //fprintf(stderr, "open %s failed, %m\n", log_file_name);
            return -1;
        }

    }
    */
    
    ret = write(fd_log_file, buf, len);
    if (ret < 0){
        return -1;
    }
    return 0;
}

int write_log(const char *fmt, ...)
{
    time_t log_time = time(NULL);
    int len1, len2;
    char buf[1024];
    va_list ap;

    len1 = strftime(buf,  sizeof(buf), "[%Y-%m-%d %H:%M:%S] ", localtime(&log_time));
    va_start(ap, fmt);
    len2 = vsnprintf(buf + len1, sizeof(buf) - len1, fmt, ap);
    va_end(ap);

    buf[len1 + len2] = '\0';
    return write_file(buf, len1 + len2 + 1);
}

int log_test(int argc, char *argv[])
{
    int i, cnt;
 
    
    init_log(argv[1], atoi(argv[2]), 10);

    cnt = atoi(argv[3]);
    for (i=0; i<cnt; i++){
        INFO("the %d time write %s", i, argv[4]);
}

    
    return 0;
}
