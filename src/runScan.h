#ifndef RUNSCAN_H
#define RUNSCAN_H

#include "read_ext2.h"

typedef struct inode_locate {
    int     ngroup;     // the number of group
    off_t   offset;     // the offset of the inode table
    int     inode_no;   // the inode number
} inode_loc_t;

int is_jpg(char *buffer);

int create_file_ino(int inode_num, char *out_dir);

int create_file_filename(char *file_name, char *out_dir);

void copy_to_file(int jpg_fd, struct ext2_super_block *super, int disk_fd, struct ext2_inode *inode, int grp_idx);

void find_and_copy_jpg_inode(node_t **dir_locator, node_t **jpg_inodes, struct ext2_super_block *super, int disk_fd, char *out_dir);

void copy_direct_blocks(int block_num, int disk_fd, int grp_idx, struct ext2_inode *inode, char *buffer, int jpg_fd, int block_offset);

void copy_single_indirect(int single_offset, int disk_fd, int grp_idx, struct ext2_inode *inode, char *buffer, int jpg_fd, int block_offset);

void copy_double_indirect(int double_offset_1, int double_offset_2, int b_per_b, int disk_fd, int grp_idx, struct ext2_inode *inode, char *buffer, int jpg_fd, int block_offset);

int search_for_inode(struct ext2_dir_entry_2 *dir_entry, node_t **jpg_inodes, char *file_name);

void find_and_copy_jpg_filename(node_t **dir_locator, node_t **jpg_inodes, struct ext2_super_block *super, int disk_fd, char *out_dir);

#endif