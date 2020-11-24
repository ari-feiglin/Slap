#ifndef _STANDARD_HEADER
#define _STANDARD_HEADER

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1024)
#endif

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

#define max(x, y) ((x < y) ? y : x)
#define min(x, y) ((x < y) ? x : y)

typedef enum _bool {false=0, true=1}bool;

typedef enum error_code_s {
    ERROR_CODE_UNINITIALIZED = -1,
    ERROR_CODE_SUCCESS = 0,

    ERROR_CODE_EOF,

    ERROR_CODE_COULDNT_OPEN,
    ERROR_CODE_COULDNT_CREATE, 
    ERROR_CODE_COULDNT_RENAME,
    ERROR_CODE_COULDNT_TRUNCATE,
    ERROR_CODE_COULDNT_READ,
    ERROR_CODE_COULDNT_WRITE,
    ERROR_CODE_COULDNT_LSEEK,
    ERROR_CODE_COULDNT_CHMOD,
    ERROR_CODE_COULDNT_GET_PATH,
    ERROR_CODE_COULDNT_GET_STAT,

    ERROR_CODE_COULDNT_ALLOCATE_MEMORY,
    ERROR_CODE_COULDNT_SPRINTF,
    ERROR_CODE_ALREADY_EXISTS,
    ERROR_CODE_COULDNT_GET_STRLEN,

    ERROR_CODE_COULDNT_EXTRACT_FILE_NAME,
    ERROR_CODE_COULDNT_GET_HASH,

    ERROR_CODE_INVALID_INPUT,
    ERROR_CODE_UNDEFINED,
    ERROR_CODE_UNKNOWN
}error_code_t;

int valid_strncmp(char * s1, char * s2);
error_code_t make_dir(const char * path);
int extract_file_name(char * file_path, char ** file_name);
int extract_dir(char * path, int dir_num, char ** dir_name);
int copy_file_range(int in_fd, loff_t in_offset, int out_fd, loff_t out_offset, int length);

#endif