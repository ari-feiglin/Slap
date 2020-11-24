#include "slap_commands.h"

const char * repo_dir_name = ".slap";
const char * delete_file_name = "del";
char * object_dir_path = NULL;
char * index_file_path = NULL;
char * HEAD_file_path = NULL;

/**
 * @brief: defines all global variables for future use in the program.
 * 
 * @returns: ERROR_CODE_SUCCESS upon success, else an indicative error code
 * @notes: This will be removed once I am set on a name for the program
 */
error_code_t init_program(){
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;
    int error_check = 0;
    commit_file_segment_t test = {0};

    object_dir_path = malloc(strnlen(repo_dir_name, BUFFER_SIZE) + 2 + strnlen("objects", BUFFER_SIZE));
    if(NULL == object_dir_path){
        perror("MAIN: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    index_file_path = malloc(strnlen(repo_dir_name, BUFFER_SIZE) + 2 + strnlen("index", BUFFER_SIZE));
    if(NULL == object_dir_path){
        perror("MAIN: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    HEAD_file_path = malloc(strnlen(repo_dir_name, BUFFER_SIZE) + 2 + strnlen("HEAD", BUFFER_SIZE));
    if(NULL == object_dir_path){
        perror("MAIN: Malloc error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_ALLOCATE_MEMORY;
        goto cleanup;
    }

    error_check = sprintf(object_dir_path, "%s/objects", repo_dir_name);
    if(error_check < 0){
        perror("MAIN: Sprintf error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_SPRINTF;
        goto cleanup;
    }
    
    error_check = sprintf(index_file_path, "%s/index", repo_dir_name);
    if(error_check < 0){
        perror("MAIN: Sprintf error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_SPRINTF;
        goto cleanup;
    }

    error_check = sprintf(HEAD_file_path, "%s/HEAD", repo_dir_name);
    if(error_check < 0){
        perror("MAIN: Sprintf error");
        printf("(Errno: %i)\n", errno);
        return_value = ERROR_CODE_COULDNT_SPRINTF;
        goto cleanup;
    }

    return_value = ERROR_CODE_SUCCESS;

cleanup:
    return return_value;
}

int main(int argc, char ** argv){
    int difference = 0;
    int error_check = 0;
    error_code_t return_value = ERROR_CODE_UNINITIALIZED;

    return_value = init_program();
    if(ERROR_CODE_SUCCESS != return_value){
        goto cleanup;
    }
    
    if(argc < 2){
        printf("USAGE: %s: <command> [options]\n", argv[0]);
        return_value = ERROR_CODE_INVALID_INPUT;
        goto cleanup;
    }

    difference = valid_strncmp(argv[1], "init");
    if(0 == difference){
        return_value = init();
        goto cleanup;
    }

    difference = valid_strncmp(argv[1], "add");
    if(0 == difference){
        if(argc < 3){
            printf("USAGE: %s add: <files>\n", argv[0]);
            return_value = ERROR_CODE_INVALID_INPUT;
            goto cleanup;
        }

        return_value = add_files(argc - 2, &argv[2]);
        goto cleanup;
    }

    difference = valid_strncmp(argv[1], "commit");
    if(0 == difference){
        return_value = commit(NULL);
        goto cleanup;
    }

    difference = valid_strncmp(argv[1], "checkout");
    if(0 == difference){
        return_value = checkout(argv[2]);
        goto cleanup;
    }

cleanup:
    if(NULL != object_dir_path){
        free(object_dir_path);
    }
    if(NULL != index_file_path){
        free(index_file_path);
    }
    if(NULL != HEAD_file_path){
        free(HEAD_file_path);
    }
}
