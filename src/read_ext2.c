#include <stdio.h>
#include "read_ext2.h"

/* implementations credit to
 * Smith College
 * http://www.science.smith.edu/~nhowe/Teaching/csc262/oldlabs/ext2.html
 */

unsigned int block_size = 1024;         /* default 1kB block size */
unsigned int inodes_per_block = 0;                      /* number of inodes per block */
unsigned int itable_blocks = 0;                         /* size in blocks of the inode table */
unsigned int blocks_per_group = 0;
unsigned int num_groups = 0;


int debug = 0;          //turn on/off debug prints

/* read the first super block to initialize common variables */
void ext2_read_init( int                      fd)
{
	struct ext2_super_block super;
	lseek(fd, BASE_OFFSET, SEEK_SET);        /* position head above super-block */
        read(fd, &super, sizeof(struct ext2_super_block));              /* read super-block */

        if (super.s_magic != EXT2_SUPER_MAGIC) {
                fprintf(stderr, "read_super_block: Not a Ext2 filesystem\n");
                exit(1);
        }

        block_size = 1024 << super.s_log_block_size;
		inodes_per_block = block_size / sizeof(struct ext2_inode);		/* number of inodes per block */
		itable_blocks = super.s_inodes_per_group / inodes_per_block;		/* size in blocks of the inode table */
		blocks_per_group = super.s_blocks_per_group;
		num_groups = (super.s_blocks_count + blocks_per_group - 1) / blocks_per_group;

		
		if (debug)
		{
			printf("Reading first super-block from device: \n"
				   "Block size                    : %u\n"
				   "number of inodes in a block   : %u\n"
				   "Inode table size in blocks    : %u\n"
				   "Blocks per group              : %u\n"
				   "number of block groups        : %u\n"
				   ,
				   block_size,
				   inodes_per_block,
				   itable_blocks,
				   blocks_per_group,
				   num_groups);
		}
		
		
}

/* read the first super block; for this project, you will only deal with the first block group */
void read_super_block( int                      fd,        /* the disk image file descriptor */
					   int                      ngroup,        /* which block group to access */
					   struct ext2_super_block *super      /* where to put the super block */
						)
{
	    lseek(fd, BASE_OFFSET + BLOCK_OFFSET(blocks_per_group * ngroup), SEEK_SET);        /* position head above super-block */
        read(fd, super, sizeof(struct ext2_super_block));              /* read super-block */

        if (super->s_magic != EXT2_SUPER_MAGIC) {
                fprintf(stderr, "read_super_block: Not a Ext2 filesystem\n");
                exit(1);
        }

        block_size = 1024 << super->s_log_block_size;
		
		if (debug)
		{
			printf("Reading super-block from device: \n"
				   "Inodes count            : %u\n"
				   "Blocks count            : %u\n"
				   "First data block        : %u\n"
				   "Block size              : %u\n"
				   "log2(Block size)        : %u\n"
				   "Blocks per group        : %u\n"
				   "Inodes per group        : %u\n"
				   "First non-reserved inode: %u\n"
				   "Size of inode structure : %hu\n"
				   ,
				   super->s_inodes_count,
				   super->s_blocks_count,
				   super->s_first_data_block,
				   block_size,
				   super->s_log_block_size,
				   super->s_blocks_per_group,
				   super->s_inodes_per_group,
				   super->s_first_ino,          /* first non-reserved inode */
				   super->s_inode_size);
		}
		
		
		inodes_per_block = block_size / sizeof(struct ext2_inode);		/* number of inodes per block */
		itable_blocks = super->s_inodes_per_group / inodes_per_block;		/* size in blocks of the inode table */
}

/* Read the first group-descriptor in the first block group; you will not be tested with a disk image with more than one block group */
void read_group_desc( int                      fd,        /* the disk image file descriptor */
					  int                      ngroup,        /* which block group to access */
		      struct ext2_group_desc *group     /* where to put the group-descriptor */
					)
{
		lseek(fd, BASE_OFFSET + BLOCK_OFFSET(blocks_per_group * ngroup) + block_size, SEEK_SET);
        read(fd, group, sizeof(struct ext2_group_desc));

		if (debug)
		{
			printf("Reading first group-descriptor from device:\n"
				   "Blocks bitmap block: %u\n"
				   "Inodes bitmap block: %u\n"
				   "Inodes table block : %u\n"
				   "Free blocks count  : %u\n"
				   "Free inodes count  : %u\n"
				   "Directories count  : %u\n"
				   ,
				   group->bg_block_bitmap,
				   group->bg_inode_bitmap,
				   group->bg_inode_table,
				   group->bg_free_blocks_count,
				   group->bg_free_inodes_count,
				   group->bg_used_dirs_count);    /* directories count */
		}
}

/* calculate the relative start address of the inode table in a group */
off_t locate_inode_table( int ngroup, const struct ext2_group_desc *group      /* the first group-descriptor */
							    )
{
		return BLOCK_OFFSET(group->bg_inode_table + blocks_per_group * ngroup);
}

/* calculate the relative start address of the data blocks in a group */
off_t locate_data_blocks( int ngroup, const struct ext2_group_desc *group      /* the first group-descriptor */
							    )
{
		return BLOCK_OFFSET(group->bg_inode_table + itable_blocks + blocks_per_group * ngroup);
}

/* added by chenfei: calculate the relative start address of the data blocks in a group */
off_t locate_group( int ngroup      /* the first group-descriptor */
							    )
{
		return BLOCK_OFFSET(blocks_per_group * ngroup);
}

void read_inode(fd, ngroup, offset, inode_no, inode)
//	 int 							ngroup;
     int                            fd;        /* the floppy disk file descriptor */
     int 							ngroup;
     off_t 			   				offset;    /* offset to the start of the inode table */
     int                            inode_no;  /* the inode number to read  */
     struct ext2_inode             *inode;     /* where to put the inode */
{
        lseek(fd, BLOCK_OFFSET(blocks_per_group * ngroup) + offset + (inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);
        read(fd, inode, sizeof(struct ext2_inode));
}

// added by chenfei
void my_read_inode(fd, start_inode_table, inode_no, inode)
//	 int 							ngroup;
     int                            fd;        /* the floppy disk file descriptor */
     off_t 			   				start_inode_table;    /* offset to the start of the inode table */
     int                            inode_no;  /* the inode number to read  */
     struct ext2_inode             *inode;     /* where to put the inode */
{
        lseek(fd, start_inode_table + (inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);
        read(fd, inode, sizeof(struct ext2_inode));
}

// added by chenfei
void read_block( int    fd,             /* the disk image file descriptor */
                 off_t  start_group,    /* the relative offset of datablocks in the group */
                 __u32  block_num,      /* relative block number, start from 0 */
                 char * buffer          /* where to put the block data */
                 )
{
    lseek(fd, start_group + block_num * block_size, SEEK_SET);
    if (read(fd, buffer, block_size) == -1)
        printf("read_block: cannot read!\n");
}
