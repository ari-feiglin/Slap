#include "slap_commands.h"

/**
 * @brief: Checks if all files in the index are up-to-date
 * @param[IN] index_fd: The file descriptor of the index file
 * 
 * @returns: 1 if index file is up-to-date, 0 if it isn't, and -1 on error
 */
int can_checkout(IN int index_fd){
    int checkout_is_valid = 1;
    int error_check = 0;
    int difference = 0;
    unsigned char * file_hash = NULL;
    index_file_segement_t index_segment = {0};

    while(true){
        error_check = get_next_index_segment(index_fd, &index_segment);
        if(ERROR_CODE_SUCCESS != error_check && ERROR_CODE_EOF != error_check){
            checkout_is_valid = -1;
            goto cleanup;
        }
        
        if(ERROR_CODE_EOF == error_check){
            break;
        }

        difference = strncmp(index_segment.repo_sha, index_segment.wdir_sha, SHA_DIGEST_LENGTH);
        if(0 != difference){
            printf("\e[31mThe repository's version of \e[1m%s\e[0m\e[31m is not up to date.\e[0m\n", index_segment.name);
            checkout_is_valid = 0;
        }
    }

cleanup:
    if(NULL != file_hash){
        free(file_hash);
    }

    return checkout_is_valid;
}

/**
 * @brief: Checks out a commit
 * @param[IN] path: The path to the commit object
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t checkout(IN char * path){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    unsigned char * file_hash = NULL;
    int error_check = 0;
    int num_of_parents = 0;
    int commit_fd = -1;
    int index_fd = -1;
    int file_fd = -1;
    int blob_fd = -1;
    int up_to_date = 0;
    char * blob_path = NULL;
    commit_file_segment_t commit_segment = {0};

    printf("COMMIT PATH: %s\n", path);
    commit_fd = open(path, O_RDONLY);
    if(-1 == commit_fd){
        perror("CHECKOUT: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    index_fd = open(index_file_path, O_RDONLY);
    if(-1 == index_fd){
        perror("CHECKOUT: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    up_to_date = can_checkout(index_fd);
    if(-1 == up_to_date){
        goto cleanup;
    }

    if(0 == up_to_date){
        printf("\e[38;2;255;150;0mIn order to check out a commit, add non up-to-date files to the repository.\e[0m\n\n");
        return_value = ERROR_CODE_SUCCESS;
        goto cleanup;
    }

    error_check = read(commit_fd, &num_of_parents, sizeof(num_of_parents));
    if(-1 == error_check){
        perror("CHECKOUT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    error_check = lseek(commit_fd, num_of_parents * SHA_DIGEST_LENGTH, SEEK_CUR);
    if(-1 == error_check){
        perror("CHECKOUT: Lseek error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_LSEEK;
        goto cleanup;
    }

    while(true){
        if(-1 != blob_fd){
            close(blob_fd);
        }
        if(-1 != file_fd){
            close(file_fd);
        }

        return_value = get_next_commit_segment(commit_fd, &commit_segment);
        if(ERROR_CODE_SUCCESS != return_value && ERROR_CODE_EOF != return_value){
            goto cleanup;
        }

        if(ERROR_CODE_EOF == return_value){
            break;
        }

        if(NULL != blob_path){
            free(blob_path);
        }
        return_value = get_blob_path(commit_segment.sha, &blob_path, NULL);
        if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }

        printf("BLOB NAME: %s\n", blob_path);
        blob_fd = open(blob_path, O_RDONLY);
        if(-1 == blob_fd){
            perror("CHECKOUT: Open error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_OPEN;
            goto cleanup;
        }

        printf("FILE NAME: %s\n", commit_segment.name);
        file_fd = open(commit_segment.name, O_WRONLY | O_CREAT, 0666);
        if(-1 == file_fd){
            perror("CHECKOUT: Open error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_OPEN;
            goto cleanup;
        }

        error_check = ftruncate(file_fd, 0);
        if(-1 == error_check){
            perror("CHECKOUT: Ftruncate error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_TRUNCATE;
            goto cleanup;
        }

        error_check = copy_file_range(blob_fd, 0, file_fd, 0, -1);
        if(-1 == error_check){
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }

        error_check = fchmod(file_fd, commit_segment.mode);
        if(-1 == error_check){
            perror("CHECKOUT: Fchmod error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_CHMOD;
            goto cleanup;
        }
    }

cleanup:
    if(-1 != commit_fd){
        close(commit_fd);
    }

    return return_value;
}