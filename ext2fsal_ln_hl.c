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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_ln_hl(const char *src,
                        const char *dst)
{
    /**
     * TODO: implement the ext2_ln_hl command here ...
     * src and dst are the ln command arguments described in the handout.
     */

     /* This is just to avoid compilation warnings, remove these 3 lines when you're done. */
    unsigned int src_inode = find_inode_by_path(src);
    // src not exist
    if (src_inode==0){
        return -ENOENT;
    }
    // src is dir
    if(inode_is_dir(src_inode)){
        return -EISDIR;
    }
    unsigned int dst_inode = find_inode_by_path(dst);
    // dst exist, not overwrite
    if(dst_inode){
        return -EEXIST;
    }
    // get parent path and child name for dst.
    char * parent_path = malloc(sizeof(char)*strlen(dst));
    char * last_name = malloc(sizeof(char)*strlen(dst));
    char * tmp = malloc(sizeof(char)*strlen(dst));
    strncpy(tmp,dst,strlen(dst));
    tmp[strlen(dst)] = '\0';
    if (strncmp(tmp+strlen(dst)-1,"/",1)==0) tmp[strlen(dst)-1] = '\0';
    get_path_parent(tmp,parent_path);
    get_path_last(tmp,last_name);
    // name too long
    if(strlen(last_name)> EXT2_NAME_LEN)return -ENAMETOOLONG;
    // if name is null-terminate
    if(strlen(last_name)==0) return -ENOENT;
    int parent_inode = find_inode_by_path(parent_path);  
    //create hard link.
    pthread_mutex_lock(&inode_lock[parent_inode]);
    unsigned int result = create_dir(parent_inode,last_name,src_inode,EXT2_FT_REG_FILE);
    pthread_mutex_unlock(&inode_lock[parent_inode]);

    if (result <=0) return result;
    
    return 0;
}
