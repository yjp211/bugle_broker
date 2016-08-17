#ifndef _LOG_H_
#define _LOG_H_


#define DEBUG(level, format, ...) \
    do \
    { \
        debug_log(level, "%s:%d <%s> DEBUG-%d - " format ,__FILE__, __LINE__, __FUNCTION__, level, ##__VA_ARGS__); \
    } while (0)

#define INFO(format, ...) \
    do \
    { \
        write_log( "%s:%d <%s> INFO  - " format ,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)

#define ERROR(format, ...) \
    do \
    { \
        write_log( "%s:%d <%s> ERROR - " format ,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)

int init_log(char *file_name, int file_size, int debug_level);
int write_log(const char *fmt, ...);
int debug_log(int level, const char *fmt, ...);
#endif
