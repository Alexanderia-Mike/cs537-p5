#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include "ext2_fs.h"
#include "read_ext2.h"
#include "linkedList.h"
#include "runScan.h"
#include "string.h"

static inline int real_inode_num(int ngroup, int inode_no) {
    return ngroup * (inodes_per_block * blocks_per_group) + inode_no;
}

static void print_dir_locator(void *elmt) {
    inode_loc_t *loc = (inode_loc_t *) elmt;
    printf("ngroup=%d, offset=%ld, inode_num=%d\n", loc->ngroup, loc->offset, loc->inode_no);
}

static void print_jpg_inode(void *elmt) {
    int *inode_no = (int *) elmt;
    printf("inode number = %d\n", *inode_no);
}


int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}

    /* create output directory */
    if (mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno == EEXIST)
            printf("the output directory %s already exists!\n", argv[2]);
        else
            printf("cannot create output directory %s with errno %d\n", argv[2], errno);
        exit(-1);
    }

    /* open disk image and read super */
	int disk_fd = open(argv[1], O_RDONLY);
	ext2_read_init(disk_fd);
    struct ext2_super_block *super = malloc(sizeof(struct ext2_super_block));
    read_super_block(disk_fd, 0, super);

	/* dir_locator for faster directory search */
    node_t **dir_locator = newList();
	/* jpg_inodes for inodes cache */
    node_t **jpg_inodes = newList();

    /* find the inodes representing jpgs and write to outdir using inode numbers as file names.
       Store information in dir_locator and jpg_inodes throughout the process */
    find_and_copy_jpg_inode(dir_locator, jpg_inodes, super, disk_fd, argv[2]);

    // debug
    if (debug) {
        print(dir_locator, print_dir_locator);
        print(jpg_inodes, print_jpg_inode);
    }

    /* scan directories to find the actual filenames of the jpg files and write them to outdir */
    find_and_copy_jpg_filename(dir_locator, jpg_inodes, super, disk_fd, argv[2]);

    close(disk_fd);
    free(super);
    clear(dir_locator, NULL);
    clear(jpg_inodes, NULL);
}


int is_jpg(char *buffer) {
    if (buffer[0] == (char)0xff &&
        buffer[1] == (char)0xd8 &&
        buffer[2] == (char)0xff &&
        (buffer[3] == (char)0xe0 ||
        buffer[3] == (char)0xe1 ||
        buffer[3] == (char)0xe8)) 
    {
        return 1;
    }
    return 0;
}


int create_file_ino(int inode_num, char *out_dir) {
    char path_name[strlen(out_dir) + 20];
    char file_name[20] = "/file-";
    char inode_num_str[14];
    char suffix[5] = ".jpg";
    strcpy(path_name, out_dir);
    strcat(path_name, file_name);
    sprintf(inode_num_str, "%d", inode_num);
    strcat(path_name, inode_num_str);
    strcat(path_name, suffix);
    int jpg_fd = open(path_name, O_CREAT | O_WRONLY, S_IRWXU);
    if (jpg_fd == -1) {
        printf("create_file_ino: cannot open file %s! errno=%d\n", path_name, errno);
        exit(-1);
    }

    return jpg_fd;
}

int create_file_filename(char *file_name, char *out_dir) {
    char path_name[strlen(out_dir) + 1 + strlen(file_name) + 1];
    char *slash = "/";
    strcpy(path_name, out_dir);
    strcat(path_name, slash);
    strcat(path_name, file_name);
    int jpg_fd = open(path_name, O_CREAT | O_WRONLY, S_IRWXU);
    if (jpg_fd == -1) {
        printf("create_file_filename: cannot open file %s! errno=%d\n", path_name, errno);
        exit(-1);
    }

    return jpg_fd;
}


void copy_to_file(int jpg_fd, struct ext2_super_block *super, int disk_fd, struct ext2_inode *inode, int grp_idx) {
    unsigned int block_num = inode->i_blocks / (2 << super->s_log_block_size);
    int b_per_b = block_size / 4;   // block per block
    char *buffer = (char *) malloc(block_size);
    /* tell the indirect block usage */
    int single = 0, doublee = 0;
    int single_offset = 0, double_offset_1 = 0, double_offset_2 = 0;
    int block_offset = inode->i_size % block_size;
    if (block_num > EXT2_NDIR_BLOCKS) {
        single = 1;
        if (block_num > (unsigned) EXT2_NDIR_BLOCKS + b_per_b + b_per_b*b_per_b) {
            printf("image is too large!\n");
            return;
        } else if (block_num > (unsigned) EXT2_NDIR_BLOCKS + b_per_b) {
            doublee = 1;
            int double_block_num = block_num - (EXT2_NDIR_BLOCKS + b_per_b);
            single_offset = b_per_b;
            double_offset_1 = double_block_num / b_per_b;
            double_offset_2 = double_block_num % b_per_b;
        } else {
            doublee = 0;
            single_offset = block_num - EXT2_NDIR_BLOCKS;
        }
    }

    /* read the data blocks of a file one by one */
    if (single == 0) {
        copy_direct_blocks(block_num, disk_fd, grp_idx, inode, buffer, jpg_fd, block_offset);
    } else if (doublee == 0) {
        copy_direct_blocks(EXT2_NDIR_BLOCKS, disk_fd, grp_idx, inode, buffer, jpg_fd, block_size);
        copy_single_indirect(single_offset, disk_fd, grp_idx, inode, buffer, jpg_fd, block_offset);
    } else {
        copy_direct_blocks(EXT2_NDIR_BLOCKS, disk_fd, grp_idx, inode, buffer, jpg_fd, block_size);
        copy_single_indirect(b_per_b, disk_fd, grp_idx, inode, buffer, jpg_fd, block_size);
        copy_double_indirect(double_offset_1, double_offset_2, b_per_b, disk_fd, grp_idx, inode, buffer, jpg_fd, block_offset);
    }
    free(buffer);
}


void find_and_copy_jpg_inode(node_t **dir_locator, node_t **jpg_inodes, struct ext2_super_block *super, int disk_fd, char *out_dir) {
    /* iterate through all the groups */
    struct ext2_group_desc *group = (struct ext2_group_desc *) malloc(sizeof(struct ext2_group_desc));
    for (unsigned int grp_idx = 0; grp_idx < num_groups; ++grp_idx) {
	    read_group_desc(disk_fd, grp_idx, group);
        off_t start_inode_table = locate_inode_table(grp_idx, group);

        /* iterate through all the inodes in inode_table in the current group */
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
        char *buffer = (char *) malloc(block_size);
        for (unsigned int ind_idx = 1; ind_idx <= itable_blocks * inodes_per_block; ++ind_idx) {
            my_read_inode(disk_fd, start_inode_table, (int) ind_idx, inode);

            /* tell whether it's a directory or a file or nothing */
            if (!S_ISREG(inode->i_mode) && !S_ISDIR(inode->i_mode)) {
                continue;
            } else if (S_ISDIR(inode->i_mode)) {
                /* store it into dir_locator */
                inode_loc_t ind_loc = { .ngroup=grp_idx, .offset=start_inode_table, .inode_no=ind_idx };
                push(dir_locator, 0, &ind_loc, sizeof(inode_loc_t));
            } else {
                /* read the first data block */
                int first_block_num = inode->i_block[0];
                off_t start_group = locate_group(grp_idx);
                read_block(disk_fd, start_group, first_block_num, buffer);
                /* if it is a jpg, copy to the output dir */
                if (is_jpg(buffer)) {
                    /* get and store the inode number */
                    int inode_num = real_inode_num(grp_idx, ind_idx);
                    push(jpg_inodes, 0, &inode_num, sizeof(int));
                    /* create a new file in output-dir */
                    int jpg_fd = create_file_ino(inode_num, out_dir);
                    /* write to the new file */
                    copy_to_file(jpg_fd, super, disk_fd, inode, grp_idx);
                    close(jpg_fd);
                }
            }
        }
        free(inode);
        free(buffer);
    }
}


void copy_direct_blocks(int block_num, int disk_fd, int grp_idx, struct ext2_inode *inode, char *buffer, int jpg_fd, int block_offset) {
    for (unsigned int blk_idx = 0; blk_idx < (unsigned) block_num; ++blk_idx) {
        read_block(disk_fd, locate_group(grp_idx), inode->i_block[blk_idx], buffer);
        unsigned write_size = (blk_idx == (unsigned) block_num - 1 ? (unsigned) block_offset : block_size);
        if (write(jpg_fd, buffer, write_size) == -1) {
            printf("copy_direct_blocks: cannot write! errno=%d, jpg_fd=%d\n", errno, jpg_fd);
            exit(-1);
        }
    }
}


void copy_single_indirect(int single_offset, int disk_fd, int grp_idx, struct ext2_inode *inode, char *buffer, int jpg_fd, int block_offset) {
    int *single_dir = (int *) malloc(block_size);
    read_block(disk_fd, locate_group(grp_idx), inode->i_block[EXT2_NDIR_BLOCKS], (char *) single_dir);
    for (unsigned int blk_idx = 0; blk_idx < (unsigned) single_offset; ++blk_idx) {
        read_block(disk_fd, locate_group(grp_idx), single_dir[blk_idx], buffer);
        unsigned write_size = (blk_idx == (unsigned) single_offset - 1 ? (unsigned) block_offset : block_size);
        if (write(jpg_fd, buffer, write_size) == -1) {
            printf("copy_single_indirect: cannot write! errno=%d, jpg_fd=%d\n", errno, jpg_fd);
            exit(-1);
        }
    }
    free(single_dir);
}


void copy_double_indirect(int double_offset_1, int double_offset_2, int b_per_b, int disk_fd, int grp_idx, struct ext2_inode *inode, char *buffer, int jpg_fd, int block_offset) {
    int *double_dir = (int *) malloc(block_size);
    int *single_dir = (int *) malloc(block_size);
    read_block(disk_fd, locate_group(grp_idx), inode->i_block[EXT2_DIND_BLOCK], (char *) double_dir);
    for (unsigned int dir_idx = 0; dir_idx <= (unsigned) double_offset_1; ++dir_idx) {
        int block_num = (dir_idx == (unsigned) double_offset_1) ? double_offset_2 : b_per_b;
        read_block(disk_fd, locate_group(grp_idx), double_dir[dir_idx], (char *) single_dir);
        for (unsigned int blk_idx = 0; blk_idx < (unsigned) block_num; ++blk_idx) {
            read_block(disk_fd, locate_group(grp_idx), single_dir[blk_idx], buffer);
            unsigned write_size = (dir_idx == (unsigned) double_offset_1 && 
                                   blk_idx == (unsigned) block_num - 1 ? 
                                   (unsigned) block_offset : block_size);
            if (write(jpg_fd, buffer, write_size) == -1) {
                printf("copy_double_indirect: cannot write! errno=%d, jpg_fd=%d\n", errno, jpg_fd);
                exit(-1);
            }
        }
    }
    free(single_dir);
    free(double_dir);
}


void find_and_copy_jpg_filename(node_t **dir_locator, node_t **jpg_inodes, struct ext2_super_block *super, int disk_fd, char *out_dir) {
    /* scan through all the directories */
    node_t *dir_iterator = *dir_locator;
    inode_loc_t *dir_loc;
    struct ext2_group_desc *group = (struct ext2_group_desc *) malloc(sizeof(struct ext2_group_desc));
    struct ext2_inode *dir = (struct ext2_inode *) malloc(sizeof(struct ext2_inode));
    struct ext2_inode *inode = (struct ext2_inode *) malloc(sizeof(struct ext2_inode));
    struct ext2_dir_entry_2 *dir_entry;
    char dir_content[block_size];
    char *dir_content_curser;
    while (dir_iterator != NULL) {
        /* for each dir: read dir contents */
        dir_loc = (inode_loc_t *) dir_iterator->data;
        read_group_desc(disk_fd, dir_loc->ngroup, group);
        my_read_inode(disk_fd, dir_loc->offset, dir_loc->inode_no, dir);
        read_block(disk_fd, locate_group(dir_loc->ngroup), dir->i_block[0], dir_content);
        dir_entry = (struct ext2_dir_entry_2 *) dir_content;
        dir_content_curser = dir_content;
        while (dir_content_curser - dir_content < dir->i_size) {
            /* for each dir entry: check whether its inode is in jpg_inodes list */
            char file_name[256] = "";
            search_for_inode(dir_entry, jpg_inodes, file_name);
            /* if found a match: create a new file in output-dir and write to it */
            if (strlen(file_name) != 0) {
                int jpg_fd = create_file_filename(file_name, out_dir);
                my_read_inode(disk_fd, 
                            locate_inode_table(dir_loc->ngroup, group),
                            dir_entry->inode, 
                            inode);
                copy_to_file(jpg_fd, super, disk_fd, inode, dir_loc->ngroup);
                close(jpg_fd);
            }
            dir_content_curser += (8 + (dir_entry->name_len % 4 == 0 ? dir_entry->name_len : (dir_entry->name_len/4 + 1) * 4));
            dir_entry = (struct ext2_dir_entry_2 *) dir_content_curser;
        }
        dir_iterator = dir_iterator->next;
    }
    free(inode);
    free(dir);
    free(group);
}


int search_for_inode(struct ext2_dir_entry_2 *dir_entry, node_t **jpg_inodes, char *file_name) {
    int idx = 0; 
    node_t *iterator = *jpg_inodes;
    while (iterator != NULL) {
        unsigned inode_num = *(unsigned *) iterator->data;
        /* if found: return filename, and remove the inode_num from jpg_inodes list */
        if (inode_num == dir_entry->inode) {
            strcpy(file_name, dir_entry->name);
            erase(jpg_inodes, idx);
            return 1;
        }
        iterator = iterator->next;
        idx ++;
    }
    return 0;
}