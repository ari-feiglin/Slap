#ifndef _HASH_HEADER
#define _HASH_HEADER

#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1024)
#endif

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

int get_hash(char * path, unsigned char ** hash);

#endif
