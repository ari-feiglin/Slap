#include "hash.h"
#include "standard.h"

#define DETACHED (0) 
#define BRANCH (1)

typedef struct index_file_segement_s{
    unsigned char wdir_sha[SHA_DIGEST_LENGTH];
    unsigned char stage_sha[SHA_DIGEST_LENGTH];
    unsigned char repo_sha[SHA_DIGEST_LENGTH];
    mode_t mode;
    int name_len;
    char * name;
}index_file_segement_t;

typedef struct commit_file_segment_s{
    unsigned char sha[SHA_DIGEST_LENGTH];
    mode_t mode;
    int name_len;
    char * name;
}commit_file_segment_t;


extern const char * repo_dir_name;
extern char * object_dir_path;
extern char * index_file_path;
extern char * HEAD_file_path;

extern const char * delete_file_name;

error_code_t init();
error_code_t write_file_to_index(int index_fd, char * file_path, unsigned char * hash);
error_code_t s_add_file(int index_fd, char * file_path, off_t offset, bool insert);
error_code_t get_next_commit_segment(int commit_fd, commit_file_segment_t * file_segment);
error_code_t get_next_index_segment(int index_fd, index_file_segement_t * file_segment);
error_code_t add_file(char * relative_path);
error_code_t add_files(int argc, char ** argv);
error_code_t commit(char * message);
error_code_t get_head(int head_fd, unsigned char hash[SHA_DIGEST_LENGTH]);
error_code_t write_index_segment(int fd, index_file_segement_t index_segment);
error_code_t checkout(char * path);
error_code_t get_blob_path(unsigned char * hash, char ** blob_path, char ** parent_path);