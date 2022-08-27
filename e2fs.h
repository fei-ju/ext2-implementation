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

#ifndef CSC369_E2FS_H
#define CSC369_E2FS_H

#include "ext2.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <unistd.h>  
#include <pthread.h>  
/**
 * TODO: add in here prototypes for any helpers you might need.
 * Implement the helpers in e2fs.c
 */

 // .....
unsigned char *disk;  
struct ext2_group_desc *bg;
struct ext2_super_block *sb;
pthread_mutex_t inode_lock[32];

int check_absolute_path( const char *path);
void get_path_parent(const char *path,char * parent);
void get_path_last(const char *path,char * name);
struct ext2_inode * get_inode(int i);
int inode_is_dir(int i);
unsigned int find_inode_in_parent_by_name(unsigned int parent, char * name);
unsigned int find_inode_by_path(const char *path);
void set_used(unsigned char * innode_bitmap,int x);
unsigned int find_free_inode();
unsigned int find_free_block();
int round_up(int x);
void init_block(unsigned int new_block);
void init_inode(unsigned int new_inode,int is_dir);
unsigned int create_dir(unsigned int parent_inode, char * name, unsigned int new_inode,unsigned char type);
unsigned int remove_file(unsigned int parent_inode, char * name);
int copy(struct ext2_inode * new_node,char* buff);
#endif