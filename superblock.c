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

#define CEIL_FSCK(a,b) ((a+b-1)/b)
extern const unsigned int sector_size_bytes;

struct ext2_super_block* super_block = NULL;
struct ext2_group_desc* group_desc_table = NULL;
char** block_bitmap_table = NULL;
char** inode_bitmap_table = NULL;
struct ext2_inode** inode_table = NULL;
unsigned int partition_start_sector = 0;
int* inode_link = NULL;
int total_number_of_groups = 0;

static void make_block_bitmaps(int group_number)
{
	int num_sectors_block_bitmap = CEIL_FSCK(EXT2_BLOCKS_PER_GROUP(super_block), 8);
	num_sectors_block_bitmap = CEIL_FSCK(num_sectors_block_bitmap, SECTOR_SIZE);
	char block_buf[num_sectors_block_bitmap * sector_size_bytes];

	//printf("%d %d %d\n", group_desc_table[group_number].bg_block_bitmap, group_desc_table[group_number].bg_inode_bitmap, group_desc_table[group_number].bg_inode_table);
	//printf("BLOCKS_PER_SECTOR = %d\n", EXT2_BLOCK_SIZE(super_block));
	
	int block_bitmap_sector = group_desc_table[group_number].bg_block_bitmap \
								* (EXT2_BLOCK_SIZE(super_block)/sector_size_bytes) \
								 + partition_start_sector;
	//printf("Read Sector: %d\n",block_bitmap_sector );
	read_sectors(block_bitmap_sector, num_sectors_block_bitmap, block_buf);
	
	int block_bitmap_size = CEIL_FSCK(EXT2_BLOCKS_PER_GROUP(super_block), 8);

	block_bitmap_table[group_number] = calloc(block_bitmap_size, sizeof(char));

	memcpy(block_bitmap_table[group_number], block_buf, (size_t)block_bitmap_size);
	//printf("Copied %d bytes of block bitmap for group %d\n",block_bitmap_size, group_number);
}



static void make_inode_bitmaps(int group_number)
{
	int num_sectors_inode_bitmap = CEIL_FSCK(EXT2_INODES_PER_GROUP(super_block), 8);
	num_sectors_inode_bitmap = CEIL_FSCK(num_sectors_inode_bitmap, SECTOR_SIZE);
	char inode_buf[num_sectors_inode_bitmap * sector_size_bytes];
	int inode_bitmap_sector = group_desc_table[group_number].bg_inode_bitmap \
								* (EXT2_BLOCK_SIZE(super_block)/sector_size_bytes) \
								 + partition_start_sector;
	read_sectors(inode_bitmap_sector, num_sectors_inode_bitmap, inode_buf);

	//int inode_bitmap_size = 0;

	int inode_bitmap_size = CEIL_FSCK(EXT2_INODES_PER_GROUP(super_block), 8);

	inode_bitmap_table[group_number] = calloc(inode_bitmap_size, sizeof(char));
	memcpy(inode_bitmap_table[group_number], inode_buf, (size_t)inode_bitmap_size);
	//printf("Copied %d bytes of inode bitmap for group %d\n",inode_bitmap_size, group_number);
}


static void make_inode_table(int partition_start_sector, int group_number)
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
	//printf("Copied %d bytes of inode bitmap for group %d\n",inode_table_size, group_number);
}

#if 0
void check_inode(int inode_number)
{
	int index = (inode_number - 1) % EXT2_INODES_PER_GROUP(super_block);
	struct ext2_inode inode = inode_table[index];
	if(S_ISDIR(inode.i_mode))
		printf("Is a dir.\n");
	else
		printf("Not a dir\n");
}

#endif

/* Reading the super block from the partition */
static void read_super_block()
{
	char superblock_buf[SUPERBLOCK_SIZE];
	int superblock_start_sector = (partition_start_sector) + SUPERBLOCK_OFFSET/SECTOR_SIZE;
	//printf("Superblock Start sector: %d.\n", superblock_start_sector);
	read_sectors(superblock_start_sector, SUPERBLOCK_SIZE/SECTOR_SIZE, superblock_buf);
	super_block = calloc(1, sizeof(struct ext2_super_block));
	memcpy(super_block, superblock_buf, sizeof(struct ext2_super_block));
	//printf("Num blocks per group : %d\n",EXT2_BLOCKS_PER_GROUP(super_block));
	total_number_of_groups = CEIL_FSCK(super_block->s_blocks_count, EXT2_BLOCKS_PER_GROUP(super_block));
}

/* Reading the group descriptor table */
static void read_group_descriptor_table()
{
	int i = 0;
	int group_desc_table_size = CEIL_FSCK(total_number_of_groups *
			sizeof(struct ext2_group_desc),sector_size_bytes);
	int buf_size = CEIL_FSCK(group_desc_table_size, SECTOR_SIZE) * SECTOR_SIZE;
	char group_desc_table_buf[buf_size];

	int group_desc_table_sector = partition_start_sector + SUPERBLOCK_OFFSET
		/ SECTOR_SIZE + SUPERBLOCK_SIZE / SECTOR_SIZE;

	int group_desc_num_sectors = CEIL_FSCK(group_desc_table_size, SECTOR_SIZE);

	block_bitmap_table = calloc(total_number_of_groups, sizeof(char*));
	inode_bitmap_table = calloc(total_number_of_groups, sizeof(char*));
	inode_table = calloc(total_number_of_groups, sizeof(struct inode*));
	inode_link = calloc(super_block->s_inodes_count, sizeof(int));

#ifdef DEBUG
	printf("Total Number of Groups = %d \n", total_number_of_groups);
	printf("Group Desc Table Sector = %d \n", group_desc_table_sector);
	printf("Group Desc Table Size = %d \n", 	total_number_of_groups * sizeof(struct ext2_group_desc));
	printf("GD num_sectors = %d \n", group_desc_num_sectors);
	printf("bufsize = %d \n", buf_size);
#endif

	read_sectors(group_desc_table_sector,  group_desc_num_sectors
					,group_desc_table_buf);
	group_desc_table =
		calloc(total_number_of_groups, sizeof(struct ext2_group_desc));

	memcpy(group_desc_table, group_desc_table_buf, \
			total_number_of_groups * sizeof(struct ext2_group_desc));
#ifdef debug
	
	for(i = 0; i< total_number_of_groups; i++)
	{
		printf("%d %d %d\n", group_desc_table[i].bg_block_bitmap, group_desc_table[i].bg_inode_bitmap, group_desc_table[i].bg_inode_table);
	}

#endif

	for(i = 0; i< total_number_of_groups; i++)
	{
		make_block_bitmaps(i);
		make_inode_bitmaps(i);
		make_inode_table(partition_start_sector, i);
	}

}

void verify_partition(int partition_start_sector_address)
{
	int superblock_start_sector = (partition_start_sector) + SUPERBLOCK_OFFSET/SECTOR_SIZE;
	int current_group_number = 0;
	unsigned int inode_number = 0;
	partition_start_sector = partition_start_sector_address;
	printf("partition_start_sector = %d\n", partition_start_sector);
	/* Reading Super_block */
	read_super_block(partition_start_sector);

	/* Reading Group Descriptor table */
	read_group_descriptor_table(partition_start_sector);

	pass1();
	pass2();
	pass3();
	pass4();
}
