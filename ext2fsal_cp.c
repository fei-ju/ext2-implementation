/*
 *------------
 * This code is provided solely for the personal and private use of
 * students taking the CSC369H5 course at the University of Toronto.
 * Copying for purposes other than this use is expressly prohibited.
 * All forms of distribution of this code, whether as given or with
 * any changes, are expressly prohibited.
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 MCS @ UTM
 * -------------
 */

#include "ext2fsal.h"
#include "e2fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_cp(const char *src,
                     const char *dst)
{
    /**
     * TODO: implement the ext2_cp command here ...
     * Arguments src and dst are the cp command arguments described in the handout.
     */

     /* This is just to avoid compilation warnings, remove these 2 lines when you're done. */
    // printf("src:%s\n",src);
    // printf("dest:%s\n",dst);
    if (check_absolute_path(dst)==0) return -ENOENT;
    unsigned int inode = find_inode_by_path(dst);
    

    char * parent_path = malloc(sizeof(char)*strlen(dst));
    char * last_name = malloc(sizeof(char)*strlen(dst));
    char * tmp = malloc(sizeof(char)*strlen(dst));
    if(inode&&inode_is_dir(inode)){
        // printf("inode:%d,inode_is_dir,%d,%d,%s\n",inode,inode_is_dir(inode),inode,dst);
        strncpy(parent_path,dst,strlen(dst));
        parent_path[strlen(dst)] = '\0';
        // strncpy(last_name,src,strlen(src));
        get_path_last(src,last_name);

    }else{
        strncpy(tmp,dst,strlen(dst));
        tmp[strlen(dst)] = '\0';
        if (strncmp(tmp+strlen(dst)-1,"/",1)==0) tmp[strlen(dst)-1] = '\0';
        get_path_parent(tmp,parent_path);
        get_path_last(tmp,last_name);
    }
    
    // printf("parent path:%s\nname:%s\n",parent_path,last_name);

    unsigned int parent_inode = find_inode_by_path(parent_path);
    pthread_mutex_lock(&inode_lock[parent_inode]);

    // parent path not exist
    // printf("parent_inode:%d\n",parent_inode);
    if (parent_inode==0){
        pthread_mutex_unlock(&inode_lock[parent_inode]);
        // printf("1\n");
        return -ENOENT;
    } 
    // parent path is not dir
    if (!inode_is_dir(parent_inode)){
        pthread_mutex_unlock(&inode_lock[parent_inode]);
                // printf("2\n");

        return -ENOENT;
    } 
    // if name is too long
    if(strlen(last_name)> EXT2_NAME_LEN)
    {pthread_mutex_unlock(&inode_lock[parent_inode]);
        return -ENAMETOOLONG;
    }
    // if name is null-terminate
    if(strlen(last_name)==0){
        pthread_mutex_unlock(&inode_lock[parent_inode]);
                // printf("3\n");

        return -ENOENT;
    } 

    FILE * fd =fopen(src,"rb");
    if (fd==NULL){
        pthread_mutex_unlock(&inode_lock[parent_inode]);
                // printf("4\n");
         return -ENOENT;
    }
    struct stat st;
    stat(src, &st);
    off_t size = st.st_size;

    // if not enough space
    // sub 3, 1 for file inode, 1 for block, 1 for indirect node.
    int max_size_ava = EXT2_BLOCK_SIZE * (sb->s_free_blocks_count-3);
    if (size > max_size_ava) {
        pthread_mutex_unlock(&inode_lock[parent_inode]);
        return -ENOSPC;
    }

    // if is already exist
    if(find_inode_in_parent_by_name(parent_inode,last_name)){
        remove_file(parent_inode,last_name);
    }

    //read all content
    char * buff = malloc(sizeof(char)*size+1);
    fread(buff,1,size,fd);
    fclose(fd);
    buff[size] = '\0';
    // allocate inode for file struct
    unsigned int new_inode = find_free_inode();
    if (!new_inode){
        pthread_mutex_unlock(&inode_lock[parent_inode]);
                // printf("5\n");

        return -ENOSPC;
    }

    init_inode(new_inode,0);

    struct ext2_inode * new_node = get_inode(new_inode);
    unsigned int result = create_dir(parent_inode,last_name,new_inode,EXT2_FT_REG_FILE);
    new_node->i_size = size;
    new_node->i_links_count = 1;
    new_node->i_blocks = 0;
    if (result <0) {
        pthread_mutex_unlock(&inode_lock[parent_inode]);
                // printf("6\n");

        return result;
    }
    // printf("entring..\n");
    result = copy(new_node,buff);
    // printf("exit..%d\n",result);
    pthread_mutex_unlock(&inode_lock[parent_inode]);

    if (result <=0) return result;

    return 0;
}