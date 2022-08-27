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


int32_t ext2_fsal_rm(const char *path)
{
    /**
     * TODO: implement the ext2_rm command here ...
     * the argument 'path' is the path to the file to be removed.
     */

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
    //     printf("0\n");
    // printf("path:%s\n",path);
    int inode = find_inode_by_path(path);  
    // printf("inode:%d\n",inode);
    if(!inode) return -ENOENT;
    // printf("2\n");

    if (inode_is_dir(inode)) return -EISDIR;
    // printf("3\n");

    int parent_inode = find_inode_by_path(parent_path);  
    pthread_mutex_lock(&inode_lock[parent_inode]);
    // printf("about to entering\n");
    remove_file(parent_inode,last_name);
    pthread_mutex_unlock(&inode_lock[parent_inode]);
    // printf("done rm\n");



    return 0;
}