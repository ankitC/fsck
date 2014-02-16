#include <stdio.h>
#include <stdlib.h>
#include "genhd.h"
#include "utils.h"
#include "ext2_fs.h"
#include <linux/kernel.h>

#define SUPERBLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 1024
#define SECTOR_SIZE 512
#define SECTORS_PER_SUPERBLOCK SUPERBLOCK_OFFSET/SECTOR_SIZE
#define MAX_INODE_ENTRIES sizeof(struct ext2_inode) * EXT2_INODES_PER_GROUP(super_block)


extern const unsigned int sector_size_bytes;
unsigned int b0g0p = 63;
struct ext2_super_block* super_block;

struct group_descriptor_sector
{
	struct ext2_group_desc group_desc;
	char padding[SECTOR_SIZE - sizeof(struct ext2_group_desc)];
};


unsigned int inode_to_sector(unsigned int inode_num)
{
	int parition_number = 1;
	unsigned int num_blocks_per_group = EXT2_BLOCKS_PER_GROUP(super_block);
	unsigned int num_sectors_per_block = EXT2_BLOCK_SIZE(super_block) / sector_size_bytes;
	unsigned int group_number = (inode_num - 1)/EXT2_INODES_PER_GROUP(super_block);

	
	unsigned int group_start_sector = EXT2_BLOCKS_PER_GROUP(super_block) * \
										group_number * num_sectors_per_block \
										+ get_start_sector(parition_number);

	/* Reading the Group Descriptor */
	unsigned int group_descriptor_sector = group_start_sector + \
					+ SUPERBLOCK_OFFSET/SECTOR_SIZE \
					+ (EXT2_BLOCK_SIZE(super_block) / sector_size_bytes);
	
	char group_descriptor_buf[SECTOR_SIZE];
	read_sectors(group_descriptor_sector, 1, group_descriptor_buf);
	struct group_descriptor_sector* group_desciptor = (struct group_descriptor_sector*) group_descriptor_buf;
	
	/* Reading the inode table */
	unsigned int inode_table_sector_number =  group_desciptor->group_desc.bg_inode_table * num_sectors_per_block + group_start_sector;
	struct ext2_inode inode_table[EXT2_INODES_PER_GROUP(super_block)];
	int inode_table_size = (EXT2_INODES_PER_GROUP(super_block)*EXT2_INODE_SIZE(super_block)) / sector_size_bytes;


	read_sectors(inode_table_sector_number, inode_table_size, (char*) inode_table);

	int index = (inode_num - 1) % EXT2_INODES_PER_GROUP(super_block);
	struct ext2_inode inode = inode_table[index];
}

void verify_partition(int partition_number)
{
	char superblock_buf[SUPERBLOCK_SIZE];
	//unsigned int partition_number = 1;
	int partition_start_sector = get_start_sector(partition_number);
	int superblock_start_sector = (partition_start_sector) + SUPERBLOCK_OFFSET/SECTOR_SIZE;

	read_sectors(superblock_start_sector, SUPERBLOCK_SIZE/SECTOR_SIZE, superblock_buf);
	super_block = (struct ext2_super_block*) superblock_buf;

	printf("Magic Number:0x%x\n", super_block->s_magic);
	printf("Block Size: %d\n", EXT2_BLOCK_SIZE(super_block));
	printf("Inode size: %d\n",EXT2_INODE_SIZE(super_block));
	printf("Num_groups %d\n", super_block->s_blocks_count/super_block->s_blocks_per_group);
	inode_to_sector(2);

}