#include "slap_commands.h"

/**
 * @brief: compares to strings
 * @param[IN] s1: String 1 to compare
 * @param[IN] s2: String 2 to compare
 * 
 * @returns: The difference between s1 and s2
 * @notes: This just calss strncmp but with the maximum length between s1 and s2
 */
int valid_strncmp(IN char * s1, IN char * s2){
    int difference = 0;

    difference = strncmp(s1, s2, max(strnlen(s1, BUFFER_SIZE), strnlen(s2, BUFFER_SIZE)));

    return difference;
}

/**
 * @brief: creates a directory
 * @param[IN] path: The path to the directory
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 * @notes: This calls mkdir on path and then chmod-s it to the normal directory mode
 */
error_code_t make_dir(IN const char * path){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    bool already_exists = false;

    error_check = mkdir(path, 0);
    if(-1 == error_check && EEXIST == errno){
        already_exists = true;
        errno = 0;
    }
    else if(-1 == error_check){
        perror("MAKE_DIR: Mkdir error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_CREATE;
        goto cleanup;
    }

    error_check = chmod(path, 0775);
    if(-1 == error_check){
        perror("MAKE_DIR: Chmod error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_CHMOD;
        goto cleanup;
    }

    if(already_exists){
        return_value = ERROR_CODE_ALREADY_EXISTS;
    }
    else{
        return_value = ERROR_CODE_SUCCESS;
    }

cleanup:
    return return_value;
}

/**
 * @brief: gets the name of the file in a path
 * @param[IN] file_path: The path to the file
 * @param[OUT] file_name: A pointer to the string to fill out with the file name
 * 
 * @returns: The length of file_name on success, else -1
 * @notes: file_name is NUL terminated
 */
int extract_file_name(IN char * file_path, OUT char ** file_name){
    int file_name_len = -1;
    int file_path_len = 0;
    int i = 0;

    file_path_len = strnlen(file_path, BUFFER_SIZE);
    if(-1 == file_path_len){
        perror("MAKE_DIR: Chmod error");
        printf("(Errno: %i)\n", errno);
        goto cleanup;
    }

    for(i=file_path_len-1; i>=0 && file_path[i] != '/'; i--);

    file_name_len = file_path_len - 1 - i;
    *file_name = malloc(file_name_len + 1);
    if(NULL == *file_name){
        perror("EXTRACT_FILE_NAME: Malloc error");
        printf("(Errno: %i)\n", errno);
        file_name_len = -1;
        goto cleanup;
    }
    memset(*file_name, 0, file_name_len+1);

    memcpy(*file_name, file_path + i + 1, file_name_len);

cleanup:    
    return file_name_len;
}

/**
 * @brief: gets the name of some directory in a path
 * @param[IN] path: A file path
 * @param[IN] dir_num: The index of the directory to extract
 * @param[OUT] dir_name: A pointer to the string to fill out with the directory name
 * 
 * @returns: The length of dir_name on success, else -1
 * @notes: dir_name is NUL terminated
 */
int extract_dir(IN char * path, IN int dir_num, OUT char ** dir_name){
    int dir_name_len = -1;
    int dir_path_len = 0;
    int part = 0;
    int i = 0;
    int start = 0;
    int end = 0;

    dir_path_len = strnlen(path, BUFFER_SIZE);
    if(-1 == dir_path_len){
        perror("MAKE_DIR: Chmod error");
        printf("(Errno: %i)\n", errno);
        goto cleanup;
    }

    for(i=0; i<dir_path_len; i++){
        if('/' == path[i]){
            part++;
            if(dir_num == part){
                start = i+1;
            }
            if(part - 1 == dir_num){
                end = i;
                break;
            }
        }
    }

    if(0 == end){
        end = i;
    }

    dir_name_len = end - start;
    *dir_name = malloc(dir_name_len + 1);
    if(NULL == *dir_name){
        perror("EXTRACT_FILE_NAME: Malloc error");
        printf("(Errno: %i)\n", errno);
        dir_name_len = -1;
        goto cleanup;
    }
    memset(*dir_name, 0, dir_name_len+1);

    memcpy(*dir_name, path + start, dir_name_len);
    
cleanup:    
    return dir_name_len;
}

/**
 * @brief: copies data between two files
 * @param[IN] in_fd: The file descriptor of the file to copy from
 * @param[IN] in_offset: The starting offset in the in file to start copying from
 * @param[IN] out_fd: The file descriptor of the file to copy to
 * @param[IN] out_offset: The starting offset in the out file to start copying to
 * @param[IN] length: The number of bytes to copy
 * 
 * @returns: The number of bytes copied on success, else -1
 * @notes: An input of length = -1 copies from in_offset until the end of the file
 */
int copy_file_range(IN int in_fd, IN loff_t in_offset, IN int out_fd, IN loff_t out_offset, IN int length){
    char buffer[BUFFER_SIZE] = {0};
    int error_check = 0;
    int bytes_written = 0;
    int bytes_read = 0;
    struct stat statbuf = {0};
    off_t current_in_offset = 0;

    if(-1 == length){
        error_check = fstat(in_fd, &statbuf);
        if(-1 == error_check){
            perror("COPY_FILE_RANGE: Fstat error");
            printf("(Errno: %i)\n", errno);
            bytes_written = -1;
            goto cleanup;
        }

        length = statbuf.st_size;
    }

    current_in_offset = lseek(in_fd, in_offset, SEEK_SET);
    if(-1 == current_in_offset){
        perror("COPY_FILE_RANGE: Lseek error");
        printf("(Errno: %i)\n", errno);
        bytes_written = -1;
        goto cleanup;
    }

    error_check = lseek(out_fd, out_offset, SEEK_SET);
    if(-1 == current_in_offset){
        perror("COPY_FILE_RANGE: Lseek error");
        printf("(Errno: %i)\n", errno);
        bytes_written = -1;
        goto cleanup;
    }

    do{
        current_in_offset = lseek(in_fd, 0, SEEK_CUR);
        if(-1 == current_in_offset){
            perror("COPY_FILE_RANGE: Lseek error");
            printf("(Errno: %i)\n", errno);
            bytes_written = -1;
            goto cleanup;
        }

        if(in_offset + length - current_in_offset >= BUFFER_SIZE){
            bytes_read = read(in_offset, buffer, BUFFER_SIZE);
            if(-1 == bytes_read){
                perror("COPY_FILE_RANGE: Read error");
                printf("(Errno: %i)\n", errno);
                bytes_written = -1;
                goto cleanup;
            }
        }
        else{
            bytes_read = read(in_fd, buffer, in_offset + length - current_in_offset);
            if(-1 == bytes_read){
                perror("COPY_FILE_RANGE: Read error");
                printf("(Errno: %i)\n", errno);
                bytes_written = -1;
                goto cleanup;
            }
        }

        if(bytes_read != 0){
            error_check = write(out_fd, buffer, bytes_read);
            if(-1 == bytes_read){
                perror("COPY_FILE_RANGE: Write error");
                printf("(Errno: %i)\n", errno);
                bytes_written = -1;
                goto cleanup;
            }

            bytes_written += error_check;
        }

    }while(current_in_offset < in_offset + length && bytes_read != 0);

cleanup:
    return bytes_written;
}

/**
 * @brief: inserts data into a file without overwriting data
 * @param[IN] in_fd: The file descriptor of the file to insert into
 * @param[IN] in_path: The path to the file to insert into
 * @param[IN] insertion: The data to insert
 * @param[IN] offset: The offset to insert to
 * @param[IN] length: The length of the data in insertion to insert
 * 
 * @returns: The number of bytes inserted on success, else -1
 */
int file_insertion(int in_fd, char * in_path, void * insertion, off_t offset, int length){
    int error_check = 0;
    int bytes_written = 0;
    int new_fd = 0;

    error_check = rename(in_path, "del");
    if(-1 == error_check){
        perror("FILE_INSERTION: Rename error");
        printf("(Errno: %i)\n", errno);
        bytes_written = -1;
        goto cleanup;
    }

    new_fd = open(in_path, O_RDWR | O_TRUNC | O_CREAT, 0666);
    if(-1 == new_fd){
        perror("FILE_INSERTION: Open error");
        printf("(Errno: %i)\n", errno);
        bytes_written = -1;
        goto cleanup;
    }

    bytes_written = copy_file_range(in_fd, 0, new_fd, 0, offset);
    if(-1 == bytes_written){
        goto cleanup;
    }

    error_check = write(new_fd, insertion, length);
    if(-1 == error_check){
        perror("FILE_INSERTION: Write error");
        printf("(Errno: %i)\n", errno);
        bytes_written = -1;
        goto cleanup;
    }
    bytes_written += error_check;

    error_check = copy_file_range(in_fd, offset, new_fd, bytes_written, -1);
    if(-1 == error_check){
        bytes_written = -1;
        goto cleanup;
    }
    bytes_written += error_check;

    error_check = dup2(new_fd, in_fd);
    if(-1 == error_check){
        perror("FILE_INSERTION: Dup2 error");
        printf("(Errno: %i)\n", errno);
        bytes_written = -1;
        goto cleanup;
    }

    error_check = remove("del");
    if(-1 == error_check){
        perror("FILE_INSERTION: Remove error");
        printf("(Errno: %i)\n", errno);
        bytes_written = -1;
        goto cleanup;
    }

cleanup:
    return bytes_written;
}
