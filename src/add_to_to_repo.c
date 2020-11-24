#include "slap_commands.h"

/**
 * @brief: Initializes an empty repository in the working directory
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 * @notes: If a repository already exists, nothing occurs.
 */
error_code_t init(){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    bool already_exists = false;
    char * real_path = NULL;

    return_value = make_dir(repo_dir_name);
    if(ERROR_CODE_ALREADY_EXISTS == return_value){
        already_exists = true;
    }
    else if(ERROR_CODE_SUCCESS != return_value){
        goto cleanup;
    }

    if(!already_exists){
        return_value = make_dir(object_dir_path);
        if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }

        error_check = creat(index_file_path, 0666);
        if(-1 == error_check){
            perror("INIT: Creat error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_CREATE;
            goto cleanup;
        }

        error_check = creat(HEAD_file_path, 0666);
        if(-1 == error_check){
            perror("INIT: Creat error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_CREATE;
            goto cleanup;
        }
    }

    real_path = realpath("./", real_path);
    if(NULL == real_path){
        perror("INIT: Realpath error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_GET_PATH;
        goto cleanup;
    }

    if(already_exists){
        printf("Reinitialized existing repository at %s/%s\n", real_path, repo_dir_name);
    }
    else{
        printf("Initialized empty repository at %s/%s\n", real_path, repo_dir_name);
    }

cleanup:
    if(NULL != real_path){
        free(real_path);
    }
    return return_value;
}

/**
 * @brief: Writes a file segement to the index file
 * @param[IN] index_fd: The file descriptor of the index file
 * @param[IN] file_path: The path of the file to write to the index file
 * @param[IN] hash: The hash of the file specified by file_path
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t write_file_to_index(IN int index_fd, IN char * file_path, IN unsigned char * hash){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int path_len = 0;
    int error_check = 0;
    int head_fd = -1;
    int commit_fd = -1;
    int i = 0;
    int difference = 0;
    int num_of_parents = 0;
    unsigned char commit_hash[SHA_DIGEST_LENGTH] = {0};
    unsigned char repo_blob_hash[SHA_DIGEST_LENGTH] = {0};
    char * commit_path = NULL;
    commit_file_segment_t file_segment = {0};
    struct stat statbuf = {0};

    commit_path = malloc(SHA_DIGEST_LENGTH * 2 + strnlen(object_dir_path, BUFFER_SIZE) + 3);
    if(NULL == commit_path){
        perror("WRITE_FILE_TO_INDEX: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    path_len = strnlen(file_path, BUFFER_SIZE);

    if(NULL == hash){
        error_check = get_hash(file_path, &hash);
        if(-1 == error_check){
            return_value = ERROR_CODE_UNDEFINED;
            goto cleanup;
        }
    }

    head_fd = open(HEAD_file_path, O_RDONLY);
    if(-1 == head_fd){
        perror("WRITE_FILE_TO_INDEX: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    error_check = read(head_fd, commit_hash, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("WRITE_FILE_TO_INDEX: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    if(0 != error_check){
        sprintf(commit_path, "%s/%.2x/", object_dir_path, commit_hash[0]);
        for(i=1; i<SHA_DIGEST_LENGTH; i++){
            sprintf(commit_path, "%s%.2x", commit_path, commit_hash[i]);
        }

        commit_fd = open(commit_path, O_RDONLY);
        if(-1 == commit_fd){
            perror("WRITE_FILE_TO_INDEX: Open error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_OPEN;
            goto cleanup;
        }

        error_check = read(commit_fd, &num_of_parents, sizeof(num_of_parents));
        if(-1 == error_check){
            perror("WRITE_FILE_TO_INDEX: Read error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_READ;
            goto cleanup;
        }

        error_check = lseek(commit_fd, SHA_DIGEST_LENGTH * num_of_parents, SEEK_CUR);
        if(-1 == commit_fd){
            perror("WRITE_FILE_TO_INDEX: Lseek error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_LSEEK;
            goto cleanup;
        }

        do{
            if(NULL != file_segment.name){
                free(file_segment.name);
                file_segment.name = NULL;
            }
            return_value = get_next_commit_segment(commit_fd, &file_segment);
            if(ERROR_CODE_SUCCESS != return_value && ERROR_CODE_EOF != return_value){
                goto cleanup;
            }

            if(ERROR_CODE_EOF == return_value){
                break;
            }

            difference = valid_strncmp(file_segment.name, file_path);

        }while(difference != 0);

        if(0 == difference){
            memcpy(repo_blob_hash, file_segment.sha, SHA_DIGEST_LENGTH);
        }
    }

    for(i=0; i<2; i++){
        error_check = write(index_fd, hash, SHA_DIGEST_LENGTH);
        if(-1 == error_check){
            perror("WRITE_FILE_TO_INDEX: Write error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }
    }

    error_check = write(index_fd, repo_blob_hash, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("WRITE_FILE_TO_INDEX: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = stat(file_path, &statbuf);
    if(-1 == error_check){
        perror("WRITE_FILE_TO_INDEX: stat error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_GET_STAT;
        goto cleanup;
    }

    error_check = write(index_fd, &statbuf.st_mode, sizeof(statbuf.st_mode));
    if(-1 == error_check){
        perror("WRITE_FILE_TO_INDEX: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = write(index_fd, &path_len, sizeof(path_len));
    if(-1 == error_check){
        perror("WRITE_FILE_TO_INDEX: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = write(index_fd, file_path, path_len);
    if(-1 == error_check){
        perror("WRITE_FILE_TO_INDEX: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    return_value = ERROR_CODE_SUCCESS;

cleanup:
    if(NULL != file_segment.name){
        free(file_segment.name);
    }
    if(NULL != commit_path){
        free(commit_path);
    }
    if(-1 != head_fd){
        close(head_fd);
    }
    if(-1 != commit_fd){
        close(commit_fd);
    }

    return return_value;
}

/**
 * @brief: Gets the path of a blob (if one were to exist) based on the hash of the file
 * @param[IN] hash: The hash of the file
 * @param[OUT] blob_path: The path to the blob
 * @param[OUT] parent_path: The path to the parent directory of the blob file
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t get_blob_path(IN unsigned char * hash, OUT char ** blob_path, OUT char ** parent_path){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    int i = 0;

    *blob_path = malloc(strnlen(object_dir_path, BUFFER_SIZE) + SHA_DIGEST_LENGTH*2 + 3);
    if(NULL == *blob_path){
        perror("S_ADD_FILE: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    if(NULL != parent_path){
        *parent_path = malloc(strnlen(object_dir_path, BUFFER_SIZE) + 4 + 2);
        if(NULL == *blob_path){
            perror("S_ADD_FILE: Malloc error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
            goto cleanup;
        }
    }

    sprintf(*blob_path, "%s/%.2x/", object_dir_path, hash[0]);
    if(NULL != parent_path){
        sprintf(*parent_path, "%s/%.2x", object_dir_path, hash[0]);
    }
    for(i=1; i<SHA_DIGEST_LENGTH; i++){
        sprintf(*blob_path, "%s%.2x", *blob_path, hash[i]);
    }

    return_value = ERROR_CODE_SUCCESS;

cleanup:
    return return_value;
}

/**
 * @brief: Adds a file to the repository
 * @param[IN] index_fd: The file descriptor of the index file
 * @param[IN] file_path: The path to the file to add
 * @param[IN] offset: The offset in the index file to add the file segment to
 * @param[IN] insert: If the segment should be inserted (without overwriting) (This is probably redundant)
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t s_add_file(IN int index_fd, IN char * file_path, IN off_t offset, IN bool insert){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    int new_index_fd = -1;
    int i = 0;
    int bytes_read = 0;
    int file_fd = -1;
    int blob_fd = -1;
    loff_t out_offset = 0;
    loff_t in_offset = 0;
    unsigned char * hash = NULL;
    char * blob_path = NULL;
    char * blob_parent = NULL;
    char buffer[BUFFER_SIZE] = {0};
    struct stat index_statbuf = {0};

    error_check = get_hash(file_path, &hash);
    if(-1 == error_check){
        return_value = ERROR_CODE_UNDEFINED;
        goto cleanup;
    }

    return_value = get_blob_path(hash, &blob_path, &blob_parent);
    if(ERROR_CODE_SUCCESS != return_value){
        goto cleanup;
    }

    file_fd = open(file_path, O_RDWR);
    if(-1 == error_check){
        perror("S_ADD_FILE: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    errno = 0;
    return_value = make_dir(blob_parent);
    if(ERROR_CODE_SUCCESS != return_value && ERROR_CODE_ALREADY_EXISTS != return_value){
        goto cleanup;
    }
    blob_fd = open(blob_path, O_RDWR | O_EXCL | O_CREAT, 0666);
    if(-1 == blob_fd && EEXIST != errno){
        perror("S_ADD_FILE: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    if(EEXIST != errno){
        do{
            bytes_read = read(file_fd, buffer, BUFFER_SIZE);
            if(-1 == bytes_read){
                perror("S_ADD_FILE: Read error");
                printf("(Errno: %i)\n", errno);
                return_value = ERROR_CODE_COULDNT_READ;
                goto cleanup;
            }

            error_check = write(blob_fd, buffer, bytes_read);
            if(-1 == error_check){
                perror("S_ADD_FILE: Write error");
                printf("(Errno: %i)\n", errno);
                return_value = ERROR_CODE_COULDNT_WRITE;
                goto cleanup;
            }
        }while(bytes_read != 0);
    }


    /* Adding file to the index */
    error_check = stat(index_file_path, &index_statbuf);
    if(-1 == error_check){
        perror("S_ADD_FILE: Stat error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_GET_STAT;
        goto cleanup;
    }

    if(offset >= index_statbuf.st_size || offset < 0){
        error_check = lseek(index_fd, 0, SEEK_END);
        if(-1 == error_check){
            perror("S_ADD_FILE: Lseek error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_LSEEK;
            goto cleanup;
        }

        return_value = write_file_to_index(index_fd, file_path, hash);
        if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }
    }
    else if(!insert){
        error_check = lseek(index_fd, offset, SEEK_SET);
        if(-1 == error_check){
            perror("S_ADD_FILE: Lseek error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_LSEEK;
            goto cleanup;
        }

        return_value = write_file_to_index(index_fd, file_path, hash);
        if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }
    }
    else{
        error_check = rename(index_file_path, ".repo/del");
        if(-1 == error_check){
            perror("S_ADD_FILE: Rename error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_RENAME;
            goto cleanup;
        }
        
        new_index_fd = creat(index_file_path, 0666);
        if(-1 == error_check){
            perror("S_ADD_FILE: Creat error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_CREATE;
            goto cleanup;
        }

        errno = 0;
        error_check = copy_file_range(index_fd, in_offset, new_index_fd, out_offset, offset);
        if(-1 == error_check){
            perror("S_ADD_FILE: Copy_file_range error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }

        return_value = write_file_to_index(new_index_fd, file_path, hash);
        if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }

        out_offset = lseek(new_index_fd, 0, SEEK_CUR);
        if(-1 == out_offset){
            perror("S_ADD_FILE: NEW Lseek error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_LSEEK;
            goto cleanup;
        }

        in_offset = offset;

        error_check = copy_file_range(index_fd, in_offset, new_index_fd, out_offset, index_statbuf.st_size - offset);
        if(-1 == error_check){
            perror("S_ADD_FILE: Copy_file_range error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }

        remove(".repo/del");
    }

cleanup:
    if(NULL != hash){
        free(hash);
    }
    if(NULL != blob_path){
        free(blob_path);
    }
    if(NULL != blob_parent){
        free(blob_parent);
    }

    if(-1 != blob_fd){
        close(blob_fd);
    }
    if(-1 != file_fd){
        close(file_fd);
    }

    return return_value;
}

/**
 * @brief: Gets the next file segment from a commit blob
 * @param[IN] commit_fd: The file descriptor of the commit to read from
 * @param[OUT] file_segment: A pointer to the file_segment to write to
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t get_next_commit_segment(IN int commit_fd, OUT commit_file_segment_t * file_segment){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    int name_len = 0;
    mode_t mode = 0;
    unsigned char hash[SHA_DIGEST_LENGTH] = {0};
    char * name = NULL;

    error_check = read(commit_fd, hash, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("GET_NEXT_COMMIT_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    error_check = read(commit_fd, &mode, sizeof(mode));
    if(-1 == error_check){
        perror("GET_NEXT_COMMIT_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    error_check = read(commit_fd, &name_len, sizeof(name_len));
    if(-1 == error_check){
        perror("GET_NEXT_COMMIT_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    name = malloc(name_len);
    if(NULL == name){
        perror("GET_NEXT_COMMIT_SEGMENT: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    error_check = read(commit_fd, name, name_len);
    if(-1 == error_check){
        perror("GET_NEXT_COMMIT_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    file_segment->mode = mode;
    file_segment->name_len = name_len;

    file_segment->name = malloc(name_len + 1);
    if(NULL == file_segment->name){
        perror("GET_NEXT_COMMIT_SEGMENT: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }
    memset(file_segment->name, 0, name_len+1);

    memcpy(file_segment->name, name, name_len);
    memcpy(&file_segment->sha, hash, SHA_DIGEST_LENGTH);

    return_value = ERROR_CODE_SUCCESS;

    if(0 == error_check){
        return_value = ERROR_CODE_EOF;
    }

cleanup:
    if(NULL != name){
        free(name);
    }

    return return_value;
}

/**
 * @brief: Gets the next file segment from the index
 * @param[IN] index_fd: The file descriptor of the index
 * @param[OUT] file_segment: A pointer to the file_segment to write to
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t get_next_index_segment(int index_fd, index_file_segement_t * file_segment){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    int name_len = 0;
    mode_t mode = 0;
    unsigned char wdir_sha[SHA_DIGEST_LENGTH] = {0};
    unsigned char stage_sha[SHA_DIGEST_LENGTH] = {0};
    unsigned char repo_sha[SHA_DIGEST_LENGTH] = {0};
    char * name = NULL;

    error_check = read(index_fd, wdir_sha, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("GET_NEXT_INDEX_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    error_check = read(index_fd, stage_sha, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("GET_NEXT_INDEX_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    error_check = read(index_fd, repo_sha, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("GET_NEXT_INDEX_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    error_check = read(index_fd, &mode, sizeof(mode));
    if(-1 == error_check){
        perror("GET_NEXT_INDEX_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    error_check = read(index_fd, &name_len, sizeof(name_len));
    if(-1 == error_check){
        perror("GET_NEXT_INDEX_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    name = malloc(name_len);
    if(NULL == name){
        perror("GET_NEXT_INDEX_SEGMENT: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    error_check = read(index_fd, name, name_len);
    if(-1 == error_check){
        perror("GET_NEXT_INDEX_SEGMENT: Read error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    file_segment->mode = mode;
    file_segment->name_len = name_len;

    file_segment->name = malloc(name_len + 1);
    if(NULL == file_segment->name){
        perror("GET_NEXT_INDEX_SEGMENT: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }
    memset(file_segment->name, 0, name_len+1);

    memcpy(file_segment->name, name, name_len);
    memcpy(&file_segment->wdir_sha, wdir_sha, SHA_DIGEST_LENGTH);
    memcpy(&file_segment->stage_sha, stage_sha, SHA_DIGEST_LENGTH);
    memcpy(&file_segment->repo_sha, repo_sha, SHA_DIGEST_LENGTH);

    return_value = ERROR_CODE_SUCCESS;
    if(0 == error_check){
        return_value = ERROR_CODE_EOF;
    }

cleanup:
    if(NULL != name){
        free(name);
    }

    return return_value;
}

/**
 * @brief: Adds a file to the index
 * @param[IN] relative_path: The path to the file to add
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 * @notes: This function just gets the necessary information to call s_add_file
 */
error_code_t add_file(IN char * relative_path){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    int i = 0;
    int relative_path_len = 0;
    int index_fd = -1;
    int difference = 0;
    off_t index_offset = 0;
    unsigned char * hash = NULL;
    index_file_segement_t curr_file_segment = {0};
    struct stat index_statbuf = {0};


    index_fd = open(index_file_path, O_RDWR);
    if(-1 == index_fd){
        perror("ADD_FILE: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    error_check = stat(index_file_path, &index_statbuf);
    if(-1 == error_check){
        perror("ADD_FILE: Stat error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_GET_STAT;
        goto cleanup;
    }

    relative_path_len = strnlen(relative_path, BUFFER_SIZE);

    while(true){
        index_offset = lseek(index_fd, 0, SEEK_CUR);
        if(-1 == index_offset){
            perror("ADD_FILE: Lseek error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_LSEEK;
            goto cleanup;
        }

        if(index_offset > index_statbuf.st_size){
            printf("File error..?");
            return_value = ERROR_CODE_UNKNOWN;
            goto cleanup;
        }

        if(index_offset == index_statbuf.st_size){
            break;
        }
        if(NULL != curr_file_segment.name){
            free(curr_file_segment.name);
        }
        return_value = get_next_index_segment(index_fd, &curr_file_segment);
        if(ERROR_CODE_SUCCESS != return_value && ERROR_CODE_EOF != return_value){
            goto cleanup;
        }

        difference = strncmp(relative_path, curr_file_segment.name, max(relative_path_len, curr_file_segment.name_len));
        if(0 == difference){
            error_check = lseek(index_fd, index_offset, SEEK_SET);
            if(-1 == error_check){
                perror("ADD_FILE: Lseek error");
                printf("(Errno: %i)\n", errno);
                return_value = ERROR_CODE_COULDNT_LSEEK;
                goto cleanup;
            }

            break;
        }
    }

    return_value = s_add_file(index_fd, relative_path, index_offset, false);
    if(ERROR_CODE_SUCCESS != return_value){
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

cleanup:
    if(NULL != curr_file_segment.name){
        free(curr_file_segment.name);
    }

    return return_value;
}

/**
 * @brief: Adds an array of files to the index
 * @param[IN] argc: The number of files to add (the number of elements in argv)
 * @param[IN] argv: The file paths to add to the index
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 * @notes: This function just iterates over argv and calls add_file with each element
 */
error_code_t add_files(IN int argc, IN char ** argv){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int i = 0;

    for(i=0; i<argc; i++){
        return_value = add_file(argv[i]);
        if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }
    }

    return_value = ERROR_CODE_SUCCESS;

cleanup:
    return return_value;
}

/**
 * @brief: Commits an index to the repository
 * @param[IN] message: The commit message to add to the commit object (redundant for now, set to NULL)
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t commit(IN char * message){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    unsigned char hash[SHA_DIGEST_LENGTH] = {0};
    unsigned char head_hash[SHA_DIGEST_LENGTH] = {0};
    unsigned char * file_hash = NULL;
    char * blob_path = NULL;
    char * blob_parent = NULL;
    int error_check = 0;
    int i = 0;
    int num_of_parents = 0;
    int blob_fd = -1;
    int temp_fd = -1;
    int index_fd = -1;
    int head_fd = 0;
    int difference = 0;
    char input = 0;
    char * temp_commit_name = NULL;
    struct stat statbuf = {0};
    SHA_CTX sha_struct = {0};
    index_file_segement_t file_segment = {0};
    commit_file_segment_t commit_segment = {0};

    temp_commit_name = malloc(strnlen(object_dir_path, BUFFER_SIZE) + strlen("temp") + 2);
    if(NULL == temp_commit_name){
        perror("COMMIT: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    sprintf(temp_commit_name, "%s/temp", object_dir_path);

    index_fd = open(index_file_path, O_RDWR);
    if(-1 == index_fd){
        perror("COMMIT: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    head_fd = open(HEAD_file_path, O_RDWR);
    if(-1 == head_fd){
        perror("COMMIT: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    while(true){
        if(NULL != file_segment.name){
            free(file_segment.name);
            file_segment.name = NULL;
        }
        return_value = get_next_index_segment(index_fd, &file_segment);
        if(ERROR_CODE_EOF == return_value){
            break;
        }
        else if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }

        if(NULL != file_hash){
            free(file_hash);
            file_hash = NULL;
        }
        error_check = get_hash(file_segment.name, &file_hash);
        if(-1 == error_check){
            return_value = ERROR_CODE_COULDNT_GET_HASH;
            goto cleanup;
        }

        difference = strncmp(file_hash, file_segment.stage_sha, SHA_DIGEST_LENGTH);
        if(0 != difference){
            printf("\e[38;2;200;100;0m%s is not up to date\e[0m Commit anyway? ([y]/n): ", file_segment.name);

            input = getchar();
            if('y' != input){
                goto cleanup;
            }
        }
    }

    error_check = lseek(index_fd, 0, SEEK_SET);
    if(-1 == error_check){
        perror("COMMIT: Lseek error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_LSEEK;
        goto cleanup;
    }

    error_check = SHA1_Init(&sha_struct);
    if(0 == error_check){
        perror("COMMIT: SHA1_Init error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_GET_HASH;
        goto cleanup;
    }

    temp_fd = open(temp_commit_name, O_RDWR | O_CREAT, 0666);
    if(-1 == temp_fd){
        perror("COMMIT: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    return_value = get_head(head_fd, head_hash);
    if(ERROR_CODE_SUCCESS != return_value){
        goto cleanup;
    }
    for(i=0; i<SHA_DIGEST_LENGTH; i++){
        if(head_hash[i] != 0){
            num_of_parents = 1;
        }
    }

    error_check = write(temp_fd, &num_of_parents, sizeof(num_of_parents));
    if(-1 == error_check){
        perror("COMMIT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = SHA1_Update(&sha_struct, &num_of_parents, sizeof(num_of_parents));
    if(0 == error_check){
        perror("COMMIT: SHA1_Update error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_GET_HASH;
        goto cleanup;
    }

    if(num_of_parents != 0){
        error_check = write(temp_fd, head_hash, SHA_DIGEST_LENGTH);
        if(-1 == error_check){
            perror("COMMIT: Write error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }

        error_check = SHA1_Update(&sha_struct, head_hash, SHA_DIGEST_LENGTH);
        if(0 == error_check){
            perror("COMMIT: SHA1_Update error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_GET_HASH;
            goto cleanup;
        }
    }

    while(true){
        error_check = lseek(index_fd, 0, SEEK_CUR);
        if(-1 == error_check){
            perror("COMMIT: Lseek error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_LSEEK;
            goto cleanup;
        }

        if(NULL != file_segment.name){
            free(file_segment.name);
            file_segment.name = NULL;
        }
        return_value = get_next_index_segment(index_fd, &file_segment);
        if(ERROR_CODE_EOF == return_value){
            break;
        }
        else if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }

        commit_segment.mode = file_segment.mode;
        commit_segment.name = file_segment.name;
        commit_segment.name_len = file_segment.name_len;
        memcpy(commit_segment.sha, file_segment.stage_sha, SHA_DIGEST_LENGTH);
        memcpy(file_segment.repo_sha, file_segment.stage_sha, SHA_DIGEST_LENGTH);
        
        error_check = lseek(index_fd, error_check, SEEK_SET);
        if(-1 == error_check){
            perror("COMMIT: Lseek error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_LSEEK;
            goto cleanup;
        }

        return_value = write_index_segment(index_fd, file_segment);
        if(ERROR_CODE_SUCCESS != return_value){
            goto cleanup;
        }

        error_check = SHA1_Update(&sha_struct, &commit_segment, SHA_DIGEST_LENGTH + 2 * sizeof(int));
        if(0 == error_check){
            perror("COMMIT: SHA1_Update error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_GET_HASH;
            goto cleanup;
        }

        error_check = SHA1_Update(&sha_struct, commit_segment.name, commit_segment.name_len);
        if(0 == error_check){
            perror("COMMIT: SHA1_Update error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_GET_HASH;
            goto cleanup;
        }

        error_check = write(temp_fd, &commit_segment, SHA_DIGEST_LENGTH + 2 * sizeof(int));
        if(-1 == error_check){
            perror("COMMIT: Write error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }

        error_check = write(temp_fd, commit_segment.name, commit_segment.name_len);
        if(-1 == error_check){
            perror("COMMIT: Write error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }
    }

    error_check = SHA1_Final(hash, &sha_struct);
    if(0 == error_check){
        perror("COMMIT: SHA1_Final error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_GET_HASH;
        goto cleanup;
    }

    return_value = get_blob_path(hash, &blob_path, &blob_parent);
    if(ERROR_CODE_SUCCESS != return_value){
        goto cleanup;
    }

    return_value = make_dir(blob_parent);
    if(ERROR_CODE_SUCCESS != return_value && ERROR_CODE_ALREADY_EXISTS != return_value){
        goto cleanup;
    }
    blob_fd = open(blob_path, O_RDWR | O_EXCL | O_CREAT, 0666);
    if(-1 == blob_fd && EEXIST != errno){
        perror("COMMIT: Open error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_OPEN;
        goto cleanup;
    }

    if(EEXIST != errno){
        error_check = stat(temp_commit_name, &statbuf);
        if(-1 == error_check){
            perror("COMMIT: Stat error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_GET_STAT;
            goto cleanup;
        }

        error_check = copy_file_range(temp_fd, 0, blob_fd, 0, statbuf.st_size);
        if(-1 == error_check){
            return_value = ERROR_CODE_COULDNT_WRITE;
            goto cleanup;
        }

        error_check = fchmod(blob_fd, 0444);
        if(-1 == error_check){
            perror("COMMIT: Write error");
            printf("(Errno: %i)\n", errno);
            return_value = ERROR_CODE_COULDNT_CHMOD;
            goto cleanup;
        }
    }

    remove(temp_commit_name);

    error_check = ftruncate(head_fd, 0);
    if(-1 == error_check){
        perror("COMMIT: Ftruncate error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_TRUNCATE;
        goto cleanup;
    }

    error_check = lseek(head_fd, 0, SEEK_SET);
    if(-1 == error_check){
        perror("COMMIT: Lseek error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_LSEEK;
        goto cleanup;
    }

    error_check = write(head_fd, hash, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("COMMIT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    printf("Commit located at: %s\n", blob_path);

    return_value = ERROR_CODE_SUCCESS;

cleanup:
    if(NULL != file_segment.name){
        free(file_segment.name);
    }
    if(NULL != file_hash){
        free(file_hash);
    }
    if(NULL != blob_parent){
        free(blob_path);
    }
    if(NULL != blob_parent){
        free(blob_parent);
    }
    if(NULL != temp_commit_name){
        free(temp_commit_name);
    }

    if(-1 != index_fd){
        close(index_fd);
    }
    if(-1 != blob_fd){
        close(blob_fd);
    }
    if(-1 != head_fd){
        close(head_fd);
    }

    return return_value;
}

/**
 * @brief: Gets the hash stored in HEAD
 * @param[IN] head_fd: The file descriptor of HEAD
 * @param[OUT] hash: The hash to fill out
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t get_head(IN int head_fd, OUT unsigned char hash[SHA_DIGEST_LENGTH]){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;

    error_check = lseek(head_fd, 0, SEEK_SET);
    if(-1 == error_check){
        perror("GET_HEAD: Lseek error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_LSEEK;
        goto cleanup;
    }

    error_check = read(head_fd, (void *)hash, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("GET_HEAD: Lseek error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_READ;
        goto cleanup;
    }

    return_value = ERROR_CODE_SUCCESS;

cleanup:
    return return_value;
}

/**
 * @brief: Writes an index segment structure to the index file
 * @param[IN] fd: The file descriptor of the index file
 * @param[IN] index_segment: The index segment structure to add
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 */
error_code_t write_index_segment(int fd, index_file_segement_t index_segment){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;

    error_check = write(fd, index_segment.wdir_sha, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("WRITE_INDEX_SEGMENT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = write(fd, index_segment.stage_sha, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("WRITE_INDEX_SEGMENT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = write(fd, index_segment.repo_sha, SHA_DIGEST_LENGTH);
    if(-1 == error_check){
        perror("WRITE_INDEX_SEGMENT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = write(fd, &index_segment.mode, sizeof(index_segment.mode));
    if(-1 == error_check){
        perror("WRITE_INDEX_SEGMENT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = write(fd, &index_segment.name_len, sizeof(index_segment.name_len));
    if(-1 == error_check){
        perror("WRITE_INDEX_SEGMENT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    error_check = write(fd, index_segment.name, index_segment.name_len);
    if(-1 == error_check){
        perror("WRITE_INDEX_SEGMENT: Write error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_WRITE;
        goto cleanup;
    }

    return_value = ERROR_CODE_SUCCESS;

cleanup:    
    return return_value;
}
