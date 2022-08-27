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


int32_t ext2_fsal_mkdir(const char *path)
{
    /**
     * TODO: implement the ext2_mkdir command here ...
     * the argument path is the path to the directory that is to be created.
     */

    // check if input is valid absolute path.
    // printf("Making path for %s\n",path);
    if (check_absolute_path(path)==0) return -ENOENT;
    char * parent_path = malloc(sizeof(char)*strlen(path));
    char * last_name = malloc(sizeof(char)*strlen(path));
    char * tmp = malloc(sizeof(char)*strlen(path));
    strncpy(tmp,path,strlen(path));
    tmp[strlen(path)] = '\0';
    if (strncmp(tmp+strlen(path)-1,"/",1)==0) tmp[strlen(path)-1] = '\0';
    get_path_parent(tmp,parent_path);
    get_path_last(tmp,last_name);
    // path already exist        
    // printf("parent path:%s\nname:%s\n",parent_path,last_name);
    if(find_inode_by_path(path)) return -EEXIST;
    unsigned int parent_inode = find_inode_by_path(parent_path);
    // parent path not exist
    if (parent_inode==0) return -ENOENT;
    // parent path is not dir
        // printf("parent_inode %d \n",parent_inode);

    if (!inode_is_dir(parent_inode)) return -ENOENT;
    // if name is too long

    if(strlen(last_name)> EXT2_NAME_LEN)return -ENAMETOOLONG;
    // if name is null-terminate

    if(strlen(last_name)==0) return -ENOENT;
    unsigned int new_inode = find_free_inode();
    pthread_mutex_lock(&inode_lock[parent_inode]);
    if (!new_inode)return -ENOSPC;
    // printf("about to enter\n");
    unsigned int result = create_dir(parent_inode,last_name,new_inode,EXT2_FT_DIR);
    pthread_mutex_unlock(&inode_lock[parent_inode]);

    // printf("exit with %d\n",result);
    if (result <=0) return result;
    
    return 0;
}