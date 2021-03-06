#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "genhd.h"
#include "utils.h"
#include "ext2_fs.h"

#define SUPERBLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 1024
//#define SECTOR_SIZE sector_size_bytes
#define SECTORS_PER_SUPERBLOCK SUPERBLOCK_OFFSET/SECTOR_SIZE
#define MAX_INODE_ENTRIES sizeof(struct ext2_inode) * EXT2_INODES_PER_GROUP(super_block)

extern const unsigned int sector_size_bytes;

struct ext2_super_block* super_block = NULL;
struct ext2_group_desc* group_desc_table = NULL;

/* Per group information */
char** block_bitmap_table = NULL;
char** inode_bitmap_table = NULL;
struct ext2_inode** inode_table = NULL;

unsigned int partition_start_sector = 0;
int* inode_link = NULL;
int total_number_of_groups = 0;

/* Create the block bitmap */
static void make_block_bitmaps(int group_number)
{
	int num_sectors_block_bitmap = CEIL_FSCK(EXT2_BLOCKS_PER_GROUP(super_block), 8);
	num_sectors_block_bitmap = CEIL_FSCK(num_sectors_block_bitmap, SECTOR_SIZE);
	char block_buf[num_sectors_block_bitmap * sector_size_bytes];

	int block_bitmap_sector = group_desc_table[group_number].bg_block_bitmap \
							  * (EXT2_BLOCK_SIZE(super_block)/sector_size_bytes) \
							  + partition_start_sector;
	read_sectors(block_bitmap_sector, num_sectors_block_bitmap, block_buf);

	int block_bitmap_size = CEIL_FSCK(EXT2_BLOCKS_PER_GROUP(super_block), 8);

	block_bitmap_table[group_number] = calloc(block_bitmap_size, sizeof(char));
	memcpy(block_bitmap_table[group_number], block_buf, (size_t)block_bitmap_size);
}

/* Create the inode bitmaps */
static void make_inode_bitmaps(int group_number)
{
	int num_sectors_inode_bitmap = CEIL_FSCK(EXT2_INODES_PER_GROUP(super_block), 8);
	num_sectors_inode_bitmap = CEIL_FSCK(num_sectors_inode_bitmap, SECTOR_SIZE);
	char inode_buf[num_sectors_inode_bitmap * sector_size_bytes];
	int inode_bitmap_sector = group_desc_table[group_number].bg_inode_bitmap \
							  * (EXT2_BLOCK_SIZE(super_block)/sector_size_bytes) \
							  + partition_start_sector;
	read_sectors(inode_bitmap_sector, num_sectors_inode_bitmap, inode_buf);

	int inode_bitmap_size = CEIL_FSCK(EXT2_INODES_PER_GROUP(super_block), 8);

	inode_bitmap_table[group_number] = calloc(inode_bitmap_size, sizeof(char));
	memcpy(inode_bitmap_table[group_number], inode_buf, (size_t)inode_bitmap_size);
}

/* Create the inode table */
static void make_inode_table(int group_number)
{
	/* Total Number of inodes in the group */
	int inodes_per_group = EXT2_INODES_PER_GROUP(super_block);
	int inode_table_size = inodes_per_group * EXT2_INODE_SIZE(super_block);

	/* Make space for the inodes */
	inode_table[group_number] = calloc(inodes_per_group, EXT2_INODE_SIZE(super_block));


	unsigned int inode_table_sector_number =  group_desc_table[group_number].bg_inode_table \
											  * (EXT2_BLOCK_SIZE(super_block)/sector_size_bytes) \
											  + partition_start_sector;

	/* Calculate the number of sectors to read the inode table */
	int num_sectors_inode_table = CEIL_FSCK(inode_table_size, 512);
	char inode_table_buf[inode_table_size];

	read_sectors(inode_table_sector_number, num_sectors_inode_table , inode_table_buf);
	memcpy(inode_table[group_number], inode_table_buf, inode_table_size);
}

/* Reading the super block from the partition */
static void read_super_block()
{
	char superblock_buf[SUPERBLOCK_SIZE];
	int superblock_start_sector = (partition_start_sector) + SUPERBLOCK_OFFSET/SECTOR_SIZE;

	read_sectors(superblock_start_sector, SUPERBLOCK_SIZE/SECTOR_SIZE, superblock_buf);
	super_block = calloc(1, sizeof(struct ext2_super_block));
	memcpy(super_block, superblock_buf, sizeof(struct ext2_super_block));

	total_number_of_groups = CEIL_FSCK(super_block->s_blocks_count, EXT2_BLOCKS_PER_GROUP(super_block));
}

/* Reading the group descriptor table and per group information */
static void read_group_descriptor_table()
{
	int i = 0;

	int group_desc_table_size = CEIL_FSCK(total_number_of_groups *
			sizeof(struct ext2_group_desc), sector_size_bytes);

	int buf_size = CEIL_FSCK(group_desc_table_size, SECTOR_SIZE) * SECTOR_SIZE;

	char group_desc_table_buf[buf_size];

	int group_desc_table_sector = partition_start_sector + SUPERBLOCK_OFFSET
		/ SECTOR_SIZE + SUPERBLOCK_SIZE / SECTOR_SIZE;

	int group_desc_num_sectors = CEIL_FSCK(group_desc_table_size, SECTOR_SIZE);

#ifdef DEBUG
	printf("Total Number of Groups = %d \n", total_number_of_groups);
	printf("Group Desc Table Sector = %d \n", group_desc_table_sector);
	printf("Group Desc Table Size = %d \n", 	total_number_of_groups * sizeof(struct ext2_group_desc));
	printf("GD num_sectors = %d \n", group_desc_num_sectors);
	printf("bufsize = %d \n", buf_size);
#endif

	/* Read group descriptor table and save it in memory */
	read_sectors(group_desc_table_sector, group_desc_num_sectors ,group_desc_table_buf);
	group_desc_table =	calloc(total_number_of_groups, sizeof(struct ext2_group_desc));
	memcpy(group_desc_table, group_desc_table_buf, \
			total_number_of_groups * sizeof(struct ext2_group_desc));

#ifdef DEBUG
	for(i = 0; i< total_number_of_groups; i++)
		printf("%d %d %d\n", group_desc_table[i].bg_block_bitmap, group_desc_table[i].bg_inode_bitmap, group_desc_table[i].bg_inode_table);
#endif

	/* Allocate space for per group information */
	block_bitmap_table = calloc(total_number_of_groups, sizeof(char*));
	inode_bitmap_table = calloc(total_number_of_groups, sizeof(char*));
	inode_table = calloc(total_number_of_groups, sizeof(struct inode*));
	inode_link = calloc(super_block->s_inodes_count, sizeof(int));

	/* Read per group information */
	for(i = 0; i< total_number_of_groups; i++)
	{
		make_block_bitmaps(i);
		make_inode_bitmaps(i);
		make_inode_table(i);
	}
}

static void free_structures()
{
	int i;

	for(i = 0 ; i< total_number_of_groups; i++)
	{
		free(inode_table[i]);
		free(inode_bitmap_table[i]);
		free(block_bitmap_table[i]);
	}

	free(inode_table);
	free(block_bitmap_table);
	free(inode_bitmap_table);
	free(group_desc_table);
	free(super_block);
	free(inode_link);
}
void verify_partition(int partition_start_sector_address)
{
	int superblock_start_sector = (partition_start_sector) + SUPERBLOCK_OFFSET/SECTOR_SIZE;
	int current_group_number = 0;
	unsigned int inode_number = 0;
	partition_start_sector = partition_start_sector_address;

#ifdef DEBUG
	printf("partition_start_sector = %d\n", partition_start_sector);
#endif

	/* Reading Super_block */
	read_super_block(partition_start_sector);

	/* Reading Group Descriptor table */
	read_group_descriptor_table(partition_start_sector);

	/* Starting passes for FSCK */
	pass1();
	pass2();
	pass3();
	pass4();

	free_structures();
}
