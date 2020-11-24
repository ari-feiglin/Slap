#include "hash.h"

/**
 * @brief: Gets the hash of a file using the SHA1 functions provided by the openssl library
 * @param[IN] path: The path to the file
 * @param[OUT] hash: A pointer to teh array of bytes to return the hash into
 * 
 * @returns: 0 on success, else -1
 */
int get_hash(IN char * path, OUT unsigned char ** hash){
	char buffer[BUFFER_SIZE] = {0};
	SHA_CTX sha_struct = {0};
	FILE * file = NULL;
	int bytes_read = 0;
    int error_check = 0;

	file = fopen(path, "r");
    if(NULL == file){
        perror("GET_HASH: Fopen error");
        printf("(Errno %i)\n", errno);
        error_check = -1;
        goto cleanup;
    }

	error_check = SHA1_Init(&sha_struct);
    if(0 == error_check){
        perror("GET_HASH: SHA1_Update error");
        printf("(Errno %i)\n", errno);
        error_check = -1;
    }

	do{
		bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
		if(bytes_read != 0){
			error_check = SHA1_Update(&sha_struct, buffer, bytes_read);
            if(0 == error_check){
                perror("GET_HASH: SHA1_Update error");
                printf("(Errno %i)\n", errno);
                error_check = -1;
            }
		}
        else if(feof(file) == 0){
            perror("GET_HASH: Fread error");
            printf("(Errno %i)\n", errno);
            error_check = -1;
            goto cleanup;
        }
	}while(bytes_read != 0);

    if(NULL != *hash){
        free(*hash);
    }
    *hash = malloc(SHA_DIGEST_LENGTH);
    if(NULL == *hash){
        perror("GET_HASH: Malloc error");
        printf("(Errno %i)\n", errno);
        error_check = -1;
        goto cleanup;
    }

	error_check = SHA1_Final(*hash, &sha_struct);
    if(0 == error_check){
        perror("GET_HASH: SHA1_Update error");
        printf("(Errno %i)\n", errno);
        error_check = -1;
    }

cleanup:
    if(NULL != file){
        fclose(file);
    }
    return error_check;
}
