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


int32_t ext2_fsal_ln_sl(const char *src,
                        const char *dst)
{
    /**
     * TODO: implement the ext2_ln_sl command here ...
     * src and dst are the ln command arguments described in the handout.
     */

     /* This is just to avoid compilation warnings, remove these 3 lines when you're done. */
    unsigned int src_inode = find_inode_by_path(src);
    // src does not exist
    if (src_inode==0){
        return -ENOENT;
    }
    // src is dir
    if(inode_is_dir(src_inode)){
        return -EISDIR;
    }
    unsigned int dst_inode = find_inode_by_path(dst);
    // dst is exist
    if(dst_inode){
        return -EEXIST;
    }
    // get dst parent path and name.
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
    // get new inode for soft link.
    int parent_inode = find_inode_by_path(parent_path);  
    unsigned int new_inode = find_free_inode();
    init_inode(new_inode,0);
    if (!new_inode)return -ENOSPC;
    // create new soft link entry
        pthread_mutex_lock(&inode_lock[parent_inode]);

    unsigned int result = create_dir(parent_inode,last_name,new_inode,EXT2_FT_SYMLINK);
    // set variables for new inode
    struct ext2_inode * new_node = get_inode(new_inode);
    new_node->i_mode = EXT2_S_IFLNK;
    new_node->i_size = strlen(src);
    new_node->i_links_count = 1;
    // struct ext2_inode * src_node = get_inode(src_inode);
    // src_node->i_links_count ++;
    // write the link to the inode content.
    copy(new_node,(char *)src);
    pthread_mutex_unlock(&inode_lock[parent_inode]);

    if (result <=0) return result;
    
    return 0;
}
