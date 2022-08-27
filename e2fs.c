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

/**
 * TODO: Make sure to add all necessary includes here
 */

#include "e2fs.h"

 /**
  * TODO: Add any helper implementations here
  */

  // .....

// Check if path is a valid absolute path, return 1 if valid.
int check_absolute_path(const char *path){
    int ret = strncmp(path,"/",1);
    if (ret==0) return 1;
    return 0;
}

// get all char before last /
void get_path_parent(const char *path,char * parent){
    char * name = strrchr(path,'/')+1;
    strncpy(parent,path,strlen(path));
    parent[strlen(path)] = '\0';
    parent[strlen(path)-strlen(name)] = '\0';
}

//get char after last /
void get_path_last(const char *path,char * name){
    // printf("path before: %s",path);
    char * name2 = strrchr(path,'/')+1;
    memcpy(name,name2,strlen(name2));
    // printf("get_path_last;path:%s;name;%s;name2:%s,len:%d",path,name,name2,strlen(name2));
    name[strlen(name2)]  = '\0';

}

// get node by inode index
struct ext2_inode * get_inode(int i){
    unsigned char * innode_tables = (unsigned char *) (disk + EXT2_BLOCK_SIZE * bg->bg_inode_table);  
    struct ext2_inode * node = (struct ext2_inode *) (innode_tables + (i-1) * sizeof(struct ext2_inode));
    return node;
}

// 1 if inode is a dir
int inode_is_dir(int i){
    struct ext2_inode * node = get_inode(i);
    // printf("%d\n",node->i_size);
    // printf("dd:%d\n",node->i_mode);
    // printf("result:%d\n",node->i_mode &EXT2_S_IFDIR);
    if(node->i_mode &EXT2_S_IFDIR) return 1;
    return 0;
}


// return the inode of name in path parent.
unsigned int find_inode_in_parent_by_name(unsigned int parent, char * name){
    struct ext2_inode * parent_node = get_inode(parent);
    for(int i=0;i<12;i++){
        if (parent_node->i_block[i]==0) continue;
        int count = 0;
        while (count < EXT2_BLOCK_SIZE){
            struct ext2_dir_entry * entry = (struct ext2_dir_entry *) (disk+count+EXT2_BLOCK_SIZE * parent_node->i_block[i]);
            
            if (strncmp(entry->name,name,strlen(name))==0){
                // printf("found%d;entry:%s;name:%s;%ld\n",parent,entry->name,name,strlen(name));
                 return entry->inode;
            }
            count += entry->rec_len;  
        }
    }
    return 0;
}


// start at root, find the abslute path.
unsigned int find_inode_by_path( const char *path){
    pthread_mutex_lock(&inode_lock[EXT2_ROOT_INO]);

    // start travasal at root
    unsigned int index = EXT2_ROOT_INO;
    char * curr;
    char tmp[strlen(path)];
    strncpy(tmp,path,strlen(path));
    tmp[strlen(path)]= '\0';
    curr = strtok(tmp,"/");
    while(curr !=NULL){
        // printf("curr:%s\n",curr);
        char * next_curr = strtok(NULL,"/");
        // printf("finding:%s,next:%s",curr,next_curr);
        // if meet file before meet the last one, return 0
        if (next_curr!=NULL && !inode_is_dir(index)) {
            pthread_mutex_unlock(&inode_lock[EXT2_ROOT_INO]);
            return 0;
        }
        index = find_inode_in_parent_by_name(index,curr);
        // can not find inode with that name. path not exist
        // printf("find_inode_in_parent_by_name:%s,index,%d,curr:%s\n",path,index,curr);
        if(index==0){
            pthread_mutex_unlock(&inode_lock[EXT2_ROOT_INO]);
             return 0;
        }
        
        curr = next_curr;
    }
    pthread_mutex_unlock(&inode_lock[EXT2_ROOT_INO]);
    return index;
}


// set bitmap bit as used
void set_used(unsigned char * innode_bitmap,int x){
    int a = x-1;
    int i = a/8;
    int j = a % 8;
    *(innode_bitmap+i) |= 1<<j;
}

//set bitmap bit as free
void set_free(unsigned char * innode_bitmap,int x){
    int a = x-1;
    int i = a/8;
    int j = a % 8;
    *(innode_bitmap+i) &= ~(1<<j);
}



// check if bitmap bit is free
int in_use(unsigned char * innode_bitmap,int x){
    int a = x-1;
    int i = a/8;
    int j = a % 8;
    return (* (innode_bitmap+i) >> j)&1;
}


// find a free inode
unsigned int find_free_inode(){
    unsigned char * innode_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * bg->bg_inode_bitmap);  
    for(int i =0;i<sb->s_inodes_count/8;i++){
        for(int j=0;j<8;j++){
            if(((* (innode_bitmap+i) >> j)&1)==0){
                //found unsed
                int inode  = i*8+j;
                // reserved
                if (inode <sb->s_first_ino) continue;

                return inode+1;
            }
        }
    }    
    return 0;
}



// find a free block
unsigned int find_free_block(){
    unsigned char * block_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * bg->bg_block_bitmap);   
    for(int i =0;i<sb->s_blocks_count/8;i++){
        for(int j=0;j<8;j++){
            if(((* (block_bitmap+i) >> j)&1)==0){
                //found unsed
                int inode  = i*8+j;
                return inode+1;
            }
        }
    }    
    return 0;
}



// round up number to 4.
int round_up(int x){
    if (x%4==0) return x;
    return x+ (4-x%4);
}

// set the block as using and update relative count
void init_block(unsigned int new_block){
    unsigned char * block_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * bg->bg_block_bitmap);   
    if(!in_use(block_bitmap,new_block)){
        set_used(block_bitmap,new_block);
        sb->s_free_blocks_count--;
        bg->bg_free_blocks_count--;
    }
}

// set the inode as using and update relative count
void init_inode(unsigned int new_inode,int is_dir){
    unsigned char * innode_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * bg->bg_inode_bitmap);  
    if(!in_use(innode_bitmap,new_inode)){
        set_used(innode_bitmap,new_inode);
        sb->s_free_inodes_count--;
        bg->bg_free_inodes_count--;
        if (is_dir){
            bg->bg_used_dirs_count++;
        }
        struct ext2_inode * node = get_inode(new_inode);
        node->i_links_count = 0;
        node->i_dtime = 0;
    }
}

// free the inode and update relative count
void free_inode(unsigned int new_inode){
    unsigned char * innode_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * bg->bg_inode_bitmap);  
    if(in_use(innode_bitmap,new_inode)){
        set_free(innode_bitmap,new_inode);
        sb->s_free_inodes_count++;
        bg->bg_free_inodes_count++;
    }
}

// free the block and update relative count
void free_block(unsigned int new_block){
    unsigned char * block_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE * bg->bg_block_bitmap);   
    if(in_use(block_bitmap,new_block)){
        set_free(block_bitmap,new_block);
        sb->s_free_blocks_count++;
        bg->bg_free_blocks_count++;
    }
}


// used to create dir,file entry, links.
unsigned int create_dir(unsigned int parent_inode, char * name, unsigned int new_inode,unsigned char type){
    struct ext2_inode * parent_node = get_inode(parent_inode);
    struct ext2_dir_entry * new_entry;
    int empty_block = -1;
    int done = 0;

    // travasal all direct blocks
    for(int i=0;i<12;i++){
        // printf("hi6\n");
        if (parent_node->i_block[i]==0 ){
            if (empty_block==-1) empty_block= i;            
            continue;
        }

        int count = 0;
        struct ext2_dir_entry * entry;
        while (count < EXT2_BLOCK_SIZE){
            entry = (struct ext2_dir_entry *) (disk+count+EXT2_BLOCK_SIZE * parent_node->i_block[i]);
            count += entry->rec_len;  
        }

        // now entry is the last item in the block.
        // printf("before: base=%d,",sizeof(struct ext2_dir_entry));
        int last_entry_size =  round_up(sizeof(struct ext2_dir_entry) + entry->name_len*sizeof(char));
        // printf("roundup:%d\n",last_entry_size);
        int need_size = round_up(sizeof(struct ext2_dir_entry) +entry->name_len*sizeof(char));

        // have enough space for entry
        if(last_entry_size + need_size <= entry->rec_len){
            new_entry = (struct ext2_dir_entry *) ((unsigned char *)entry + last_entry_size);
            new_entry->rec_len = entry->rec_len -last_entry_size;
            entry->rec_len = last_entry_size;
            done = 1;
        }
    }

    // no space left.
    if (!done && empty_block ==-1) return -ENOSPC;


    // start a new block.
    if (done ==0){
        unsigned int new_block = find_free_block();
        if (!new_block)return -ENOSPC;
        init_block(new_block);
        // printf("entering dan\n");
        parent_node->i_block[empty_block] = new_block;
        parent_node->i_blocks+=2;
        parent_node->i_size+=EXT2_BLOCK_SIZE;
        parent_node->i_links_count++;
        new_entry = (struct ext2_dir_entry *) ((unsigned char *)disk + new_block*EXT2_BLOCK_SIZE);
        new_entry->rec_len =  EXT2_BLOCK_SIZE;
    }
    // printf("before:%d\n",sb->s_free_inodes_count);
    init_inode(new_inode,1);
    // printf("after:%d\n",sb->s_free_inodes_count);
    struct ext2_inode * node = get_inode(new_inode);
    // init the node.
    if (node->i_links_count==0){
        if (type ==EXT2_FT_DIR){
            node->i_mode = EXT2_S_IFDIR;    
        }
        if (type ==EXT2_FT_REG_FILE){
            node->i_mode = EXT2_S_IFREG;    
        }
        if (type == EXT2_FT_SYMLINK){
            node->i_mode = EXT2_S_IFLNK;
        }
        
        node->i_size = 0;
        node->i_links_count = 0;
        node->i_blocks = 0;
    }else{
        node->i_links_count++;
    }
    // init the new entry variables.
    new_entry->inode = new_inode;
    new_entry->name_len = strlen(name);
    new_entry->file_type = type;
    memcpy(new_entry->name,name,strlen(name));
    new_entry->name[strlen(name)] = '\0';
    // printf("here\n");
    if(strncmp(name,".",1)&&strncmp(name,"..",2)&&type==EXT2_FT_DIR){
        // printf("creating alies\n");
        create_dir(new_inode,".",new_inode,EXT2_FT_DIR);
        create_dir(new_inode,"..",parent_inode,EXT2_FT_DIR);

    }
    return 0;
}

// remove file name unser parent_inode
unsigned int remove_file(unsigned int parent_inode, char * name){
    struct ext2_inode * parent_node = get_inode(parent_inode);
    for(int i=0;i<12 && parent_node->i_block[i] ;i++){
        // printf("parent_node->i_block[i] %d\n",parent_node->i_block[i]);
        // printf("hi\n");
        int count = 0;
        struct ext2_dir_entry * entry;
        while (count < EXT2_BLOCK_SIZE){
            // printf("curr\n");
            int previous = entry->rec_len;
            entry = (struct ext2_dir_entry *) (disk+count+EXT2_BLOCK_SIZE * parent_node->i_block[i]);
            if(strncmp(entry->name ,name,strlen(name))==0){
                // printf("found\n");
                // found entry
                if (count==0){
                    entry->inode = 0;
                }
                unsigned int node_i = find_inode_in_parent_by_name(parent_inode,name);
                struct ext2_inode * node = get_inode(node_i);
                
                
                struct ext2_dir_entry *  previous_ent = (struct ext2_dir_entry *) (disk+count-previous+EXT2_BLOCK_SIZE * parent_node->i_block[i]);
                previous_ent->rec_len +=entry->rec_len;
                // printf("parent_inode:%d,name:%s\n",parent_inode,name);
                // printf("%d\n",node_i);
                // printf("%d\n",node->i_links_count);
                if (node->i_links_count==1){
                    // printf("free\n");
                    free_inode(node_i);
                    for(int i=0;i<12;i++){
                        if (node->i_block[i]){
                            // printf("free block:%d",node->i_block[i]);
                            free_block(node->i_block[i]);
                        }
                    }
                    if(node->i_block[12]){
                        unsigned int * blocks = (unsigned int *) (disk + node->i_block[12] * EXT2_BLOCK_SIZE);
                        for(int i=0;i<EXT2_BLOCK_SIZE/sizeof(unsigned int);i++){
                            unsigned int b = *(blocks+i);
                            if (b){
                                //  printf("free block:%d",b);
                                free_block(b);
                            }
                        }
                        free_block(node->i_block[12]);
                    }
                }

               
                return 0;
            }
            count += entry->rec_len;  
        }
    }
    return 0;
}



// copy the buff contents on to new_node
int copy(struct ext2_inode * new_node,char* buff){
    int curr = 0;
    int total = strlen(buff);
    int block_i = 0;
    int indir_i = 0;
    while(curr<total){
        // printf("total:%d,current:%d\n",total,curr);
        unsigned int new_block = find_free_block();
        if (!new_block)return -ENOSPC;
        init_block(new_block);
        if (block_i<12){
            new_node->i_block[block_i] = new_block;
            new_node->i_blocks+=2;
            unsigned char * block = disk+new_block * EXT2_BLOCK_SIZE;
            if(total-curr <EXT2_BLOCK_SIZE){
                memcpy(block,buff+curr,total - curr);       
            }else{
                memcpy(block,buff+curr,EXT2_BLOCK_SIZE);
            }
            curr+=EXT2_BLOCK_SIZE;
            block_i++;
        }else if(block_i==12){
            new_node->i_block[block_i] = new_block;
            new_node->i_blocks+=2;
            block_i++;
            unsigned char * block = disk+new_block * EXT2_BLOCK_SIZE;
            memset(block,0,EXT2_BLOCK_SIZE);
        }else{
            unsigned int * blocks = (unsigned int *) (disk + new_node->i_block[12] * EXT2_BLOCK_SIZE);
            *(blocks+indir_i) = new_block;
            new_node->i_blocks+=2;
            unsigned char * block = disk+new_block * EXT2_BLOCK_SIZE;
            if(total-curr <EXT2_BLOCK_SIZE){
                memcpy(block,buff+curr,total - curr);       
            }else{
                memcpy(block,buff+curr,EXT2_BLOCK_SIZE);
            }

            curr+=EXT2_BLOCK_SIZE;
            indir_i++;

        }
        
    }
    return 0;
}








