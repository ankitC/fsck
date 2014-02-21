#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "genhd.h"
#include "utils.h"
#include "ext2_fs.h"
#include <linux/kernel.h>

#define SUPERBLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 1024
#define BLOCKSIZE 1024
#define SECTOR_SIZE sector_size_bytes
#define SECTORS_PER_SUPERBLOCK SUPERBLOCK_OFFSET/SECTOR_SIZE
#define MAX_INODE_ENTRIES sizeof(struct ext2_inode) * EXT2_INODES_PER_GROUP(super_block)
#define SECTORS_PER_BLOCK(m) EXT2_BLOCK_SIZE(m)/sector_size_bytes
#define S_IFDIR  0040000
#define S_IFMT  00170000
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define CEIL(a,b) ((a+b-1)/b)


extern const unsigned int sector_size_bytes;

static struct ext2_super_block* super_block = NULL;
static struct ext2_group_desc* group_desc_table = NULL;
static char** block_bitmap_table = NULL;
static char** inode_bitmap_table = NULL;
static struct ext2_inode** inode_table = NULL;

static volatile int group_start_sector = 0;

static void make_block_bitmaps(int partition_start_sector, int group_number)
{
	int num_sectors_block_bitmap = CEIL(EXT2_BLOCKS_PER_GROUP(super_block), 8);

	char block_buf[num_sectors_block_bitmap * sector_size_bytes];

	printf("%d %d %d\n", group_desc_table[group_number].bg_block_bitmap, group_desc_table[group_number].bg_inode_bitmap, group_desc_table[group_number].bg_inode_table);
	printf("BLOCKS_PER_SECTOR = %d\n", EXT2_BLOCK_SIZE(super_block));
	
	int block_bitmap_sector = group_desc_table[group_number].bg_block_bitmap \
								* (EXT2_BLOCK_SIZE(super_block)/sector_size_bytes) \
								 + partition_start_sector;
	printf("Read Sector: %d\n",block_bitmap_sector );
	read_sectors(block_bitmap_sector, num_sectors_block_bitmap, block_buf);
	print_sector(block_buf);
	int block_bitmap_size = CEIL(EXT2_BLOCKS_PER_GROUP(super_block), 8);

	if((EXT2_BLOCKS_PER_GROUP(super_block) % 8) == 0)
		block_bitmap_size = EXT2_BLOCKS_PER_GROUP(super_block) / 8;
	else
		block_bitmap_size = EXT2_BLOCKS_PER_GROUP(super_block) / 8 + 1;

	//memcpy(block_bitmap_i, block_buf, (size_t)block_bitmap_size);
}


#if 0
static void make_inode_bitmaps(struct group_descriptor_t group_descriptor)
{

	int num_sectors_inode_bitmap = EXT2_INODES_PER_GROUP(super_block) \
							/ (8 * SECTOR_SIZE) + 1;
	char inode_buf[num_sectors_inode_bitmap * sector_size_bytes];
	int inode_bitmap_sector = group_descriptor.group_desc.bg_inode_bitmap \
								* SECTORS_PER_BLOCK + group_start_sector;
	read_sectors(inode_bitmap_sector, num_sectors_inode_bitmap, inode_buf);

	int inode_bitmap_size = 0;
	if(EXT2_INODES_PER_GROUP(super_block) % 8 == 0)
		inode_bitmap_size = EXT2_INODES_PER_GROUP(super_block) / 8;
	else
		inode_bitmap_size = EXT2_INODES_PER_GROUP(super_block) / 8 + 1;	
	memcpy(inode_bitmap, inode_buf, (size_t)inode_bitmap_size);
}


static void make_inode_table(struct group_descriptor_t group_descriptor)
{
	unsigned int num_blocks_per_group = EXT2_BLOCKS_PER_GROUP(super_block);
	unsigned int num_sectors_per_block = EXT2_BLOCK_SIZE(super_block) / sector_size_bytes;

	unsigned int inode_table_sector_number =  group_descriptor.group_desc.bg_inode_table \
												* SECTORS_PER_BLOCK + group_start_sector; 
	int inode_table_size = (EXT2_INODES_PER_GROUP(super_block)*EXT2_INODE_SIZE(super_block)) / sector_size_bytes;

	read_sectors(inode_table_sector_number, inode_table_size, (char*) inode_table);
}

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
void read_super_block(int partition_start_sector)
{
	char superblock_buf[SUPERBLOCK_SIZE];
	int superblock_start_sector = (partition_start_sector) + SUPERBLOCK_OFFSET/SECTOR_SIZE;
	printf("Superblock Start sector: %d.\n", superblock_start_sector);
	read_sectors(superblock_start_sector, SUPERBLOCK_SIZE/SECTOR_SIZE, superblock_buf);
	super_block = calloc(1, sizeof(struct ext2_super_block));
	//super_block = (struct ext2_super_block) superblock_buf;
	memcpy(super_block, superblock_buf, sizeof(struct ext2_super_block));
	printf("Num blocks per group : %d\n",EXT2_BLOCKS_PER_GROUP(super_block));
}

/* Reading the group descriptor table */
void read_group_descriptor_table(int partition_start_sector)
{
	int total_number_of_groups = CEIL(super_block->s_blocks_count, EXT2_BLOCKS_PER_GROUP(super_block));

	int group_desc_table_size = CEIL(total_number_of_groups *
			sizeof(struct ext2_group_desc),sector_size_bytes);
	int buf_size = CEIL(group_desc_table_size, SECTOR_SIZE) * SECTOR_SIZE;
	char group_desc_table_buf[buf_size];

	int group_desc_table_sector = partition_start_sector + SUPERBLOCK_OFFSET
		/ SECTOR_SIZE + SUPERBLOCK_SIZE / SECTOR_SIZE;

	int group_desc_num_sectors = CEIL(group_desc_table_size, SECTOR_SIZE);

	block_bitmap_table = calloc(total_number_of_groups, sizeof(char*));
	inode_bitmap_table = calloc(total_number_of_groups, sizeof(char*));
	inode_table = calloc(total_number_of_groups, sizeof(struct inode*));

//#ifdef DEBUG
	printf("Total Number of Groups = %d \n", total_number_of_groups);
	printf("Group Desc Table Sector = %d \n", group_desc_table_sector);
	printf("Group Desc Table Size = %d \n", 	total_number_of_groups * sizeof(struct ext2_group_desc));
	printf("GD num_sectors = %d \n", group_desc_num_sectors);
	printf("bufsize = %d \n", buf_size);
//#endif

	read_sectors(group_desc_table_sector,  group_desc_num_sectors
					,group_desc_table_buf);
	group_desc_table =
		calloc(total_number_of_groups, sizeof(struct ext2_group_desc));

	memcpy(group_desc_table, group_desc_table_buf, \
			total_number_of_groups * sizeof(struct ext2_group_desc));
#ifdef debug
	int i = 0;
	for(i = 0; i< total_number_of_groups; i++)
	{
		printf("%d %d %d\n", group_desc_table[i].bg_block_bitmap, group_desc_table[i].bg_inode_bitmap, group_desc_table[i].bg_inode_table);
	}
#endif
}

void verify_partition(int partition_start_sector)
{
	int superblock_start_sector = (partition_start_sector) + SUPERBLOCK_OFFSET/SECTOR_SIZE;
	int current_group_number = 0;
	unsigned int inode_number = 0;


	/* Reading Super_block */
	read_super_block(partition_start_sector);

	/* Reading Group Descriptor table */
	read_group_descriptor_table(partition_start_sector);
	make_block_bitmaps(partition_start_sector, 0);


#if 0
	group_start_sector = partition_start_sector;
	unsigned int num_sectors_per_block = EXT2_BLOCK_SIZE(super_block) / sector_size_bytes;
	int block_bitmap_size = 0;

	if((EXT2_BLOCKS_PER_GROUP(super_block) % 8) == 0)
		block_bitmap_size = EXT2_BLOCKS_PER_GROUP(super_block) / 8;
	else
		block_bitmap_size = EXT2_BLOCKS_PER_GROUP(super_block) / 8 + 1;


	int inode_bitmap_size = 0;
	if(EXT2_INODES_PER_GROUP(super_block) % 8 == 0)
		inode_bitmap_size = EXT2_INODES_PER_GROUP(super_block) / 8;
	else
		inode_bitmap_size = EXT2_INODES_PER_GROUP(super_block) / 8 + 1;


	block_bitmap = calloc(block_bitmap_size, sizeof(char));
	inode_bitmap = calloc(inode_bitmap_size, sizeof(char));
	inode_table = calloc(EXT2_INODES_PER_GROUP(super_block), sizeof(struct ext2_inode));

	printf("Magic Number:0x%x\n", super_block->s_magic);
	printf("Block Size: %d\n", EXT2_BLOCK_SIZE(super_block));
	printf("Inode size: %d\n",EXT2_INODE_SIZE(super_block));
	int num_groups = super_block->s_blocks_count/super_block->s_blocks_per_group;
	int group_descriptor_sector;
	group_descriptor_sector = partition_start_sector + 
				(SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE) / SECTOR_SIZE;

	if(group_descriptor_sector < partition_start_sector + SECTORS_PER_SUPERBLOCK)
		group_descriptor_sector = partition_start_sector + SECTORS_PER_SUPERBLOCK;

	for(current_group_number = 0; current_group_number<num_groups; current_group_number++)
	{
		/* Reading the Group Descriptor */
		if(current_group_number != 0){
			group_start_sector += EXT2_BLOCKS_PER_GROUP(super_block) * SECTORS_PER_BLOCK;
			group_descriptor_sector += EXT2_BLOCKS_PER_GROUP(super_block) * SECTORS_PER_BLOCK;
		}

		printf("Group Start sector:%d\n", group_start_sector);
		read_sectors(group_descriptor_sector, 1, (char*)&group_descriptor);
		printf("Block Bitmap : %d\n", group_descriptor.group_desc.bg_block_bitmap);
		printf("Inode Bitmap : %d\n", group_descriptor.group_desc.bg_inode_bitmap);
		printf("inode_table  : %d\n", group_descriptor.group_desc.bg_inode_table);

		make_block_bitmaps(group_descriptor);
		make_inode_bitmaps(group_descriptor);
		make_inode_table(group_descriptor);
		for(inode_number = 1; inode_number <= EXT2_INODES_PER_GROUP(super_block); inode_number ++)
		{
			//check_inode(inode_number);
		}
		if (current_group_number == 0)
		{
			check_inode(2);
		}

	}

	free(inode_bitmap);
	free(block_bitmap);
	free(inode_table);
#endif
}
