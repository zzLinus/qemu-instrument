#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#define check(x) if (!(x)) { fatal("check %s failed", #x); }

// #define log_on
#ifdef log_on
#define Log printf
#else
#define Log if (0) printf
#endif

#ifdef __cplusplus
extern "C" {
#endif
    void addr2line(uint64_t address, int* line, int* column, char* filename);
    void prep_addr2line(int fd);
#ifdef __cplusplus
}
#endif