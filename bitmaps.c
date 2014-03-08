#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include "utils.h"

#define OPT_SYMLINK_LENGTH 60

static int* inode_visited_map;

/* Check if the inode is allocated */
int is_inode_allocated(int inode_number)
{
	int group_number = inode_number / EXT2_INODES_PER_GROUP(super_block);
	int inode_index = inode_number % EXT2_INODES_PER_GROUP(super_block);

	int index = inode_index / 8;
	int shift = inode_index % 8;
	char* inode_bitmap = inode_bitmap_table[group_number];

	int retval = (inode_bitmap[index] & ((0x01) << shift)) >> shift;
	return retval;
}

/* Set the bit in the inode bitmap */
void set_inode_bitmap(int inode_number, int value)
{

	int group_number = inode_number / EXT2_INODES_PER_GROUP(super_block);
	int inode_index = inode_number % EXT2_INODES_PER_GROUP(super_block);

	int index = inode_index / 8;
	int shift = inode_index % 8;
	if(value == 1)
		inode_bitmap_table[group_number][index] |= (1 << shift); 
	else
		inode_bitmap_table[group_number][index] &= (~(1<<shift));
}

/* Set the block bitmap for if the block is allocared */
static int block_bitmap_status(int block_number, unsigned char *block_bitmap, int group_number, int block_size, struct ext2_super_block *super_block)
{
	int group_no;
	int group_block;

	group_no = block_number / super_block->s_blocks_per_group;
	group_block = block_number % super_block->s_blocks_per_group;

	int index;
	int bit;

	index = group_block / 8;
	bit = group_block % 8;

	return (block_bitmap[index] & (1 << bit)) >> bit;
}

/* Set the block bitmap for the given block */
static void set_block_bitmap(int block_number, unsigned char *block_bitmap, int group_number, int block_size, struct ext2_super_block *super_block, int setbit)
{
	int group_no;
	int group_block;

	group_no = block_number / super_block->s_blocks_per_group;
	group_block = block_number % super_block->s_blocks_per_group;
	int index;
	int bit;

	index = group_block / 8;
	bit = group_block % 8;
	if (setbit) 
		block_bitmap[group_no * block_size + index] |= (1 << bit);
	else
		block_bitmap[group_no * block_size + index] &= (~(1 << bit));
}

/* Check if the block is allocated in the block bitmap */
void check_secondary(int block_number)
{
	int group_no = block_number / super_block->s_blocks_per_group;
	int oldbit = block_bitmap_status(block_number, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block);
	if(oldbit == 0)
	{
		printf("Pass 4: Inconsistency in block %d\n", block_number + 1);
		set_block_bitmap(block_number, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block, 1);
	}
}

/* Check all the blocks as you traverse through them. Same recursion as previously used */
void check_block(int localId)
{	
	if (inode_visited_map[localId]) 
		return;
	inode_visited_map[localId] = 1;

	int block_size = EXT2_BLOCK_SIZE(super_block);

	struct ext2_inode *inode;
	int inode_index = (localId) % EXT2_INODES_PER_GROUP(super_block);
	inode = inode_table[localId/EXT2_INODES_PER_GROUP(super_block)] + inode_index;

	int offset;
	int totalSize = inode->i_size;
	if (is_directory(inode))
	{
		unsigned char *buf = calloc(inode->i_size, sizeof(char));
		get_data(partition_start_sector, inode, EXT2_BLOCK_SIZE(super_block), buf);
		offset = 0;
		struct ext2_dir_entry_2 *dir;
		while (offset < totalSize)
		{
			dir = (struct ext2_dir_entry_2 *)(buf + offset);
			if (dir->inode != 0)
				check_block(get_vistied_index(dir->inode));
			offset += dir->rec_len;
		}
		free(buf);
	}
	if (is_symbolic_link(inode))
	{
		if(inode->i_size > OPT_SYMLINK_LENGTH)
			check_secondary(inode->i_block[0]);
		return;
	}
	
	offset = 0;
	int i, j, k;
	int done = 0;

	int len = block_size / sizeof(int);
	int* indblock = calloc(block_size, sizeof(char));
	int* dindblock = calloc(block_size, sizeof(char));
	int* tindblock = calloc(block_size, sizeof(char));

	if(!done)
	{
		/* Direct Blocks */
		for (i = 0; i < EXT2_NDIR_BLOCKS && !done; ++i)
		{
			check_secondary(get_vistied_index(inode->i_block[i]));
			offset += block_size;
			if (offset >= totalSize)
			{
				done = 1;
				break;
			}
		}
	}
	if(!done)
	{
		/* Checking singly indirect blocks */
		read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_IND_BLOCK], block_size, indblock);
		check_secondary(get_vistied_index(inode->i_block[EXT2_IND_BLOCK]));
		for (i = 0; i < len && !done;  ++i) {

			check_secondary(get_vistied_index(indblock[i]));
			offset += block_size;
			if (offset >= totalSize)
			{
				done = 1;
				break;
			}
		}
	}

	if(!done)
	{
		/* Checking doubly indirect blocks */
		read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_DIND_BLOCK], block_size, dindblock);
		check_secondary(get_vistied_index(inode->i_block[EXT2_DIND_BLOCK]));
		for (i = 0; i < len && !done; ++i) {
			read_bytes(partition_start_sector * SECTOR_SIZE + block_size * dindblock[i], block_size, indblock);

			check_secondary(get_vistied_index(dindblock[i]));
			for (j = 0; j < len && !done; ++j) {

				check_secondary(get_vistied_index(indblock[j]));
				offset += block_size;
				if (offset >= totalSize)
				{
					done = 1;
					break;
				}
			}
		}
	}

	if(!done)
	{
		/* Checking triply indirect blocks */
		read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_TIND_BLOCK], block_size, tindblock);
		check_secondary(get_vistied_index(inode->i_block[EXT2_TIND_BLOCK]));
		for (i = 0; i < len && !done; ++i) {
			read_bytes(partition_start_sector * SECTOR_SIZE + block_size * tindblock[i], block_size, dindblock);

			check_secondary(get_vistied_index(tindblock[i]));

			for (j = 0; j < len && !done; ++j) {
				read_bytes(partition_start_sector * SECTOR_SIZE + block_size * dindblock[j], block_size, indblock);

				check_secondary(get_vistied_index(dindblock[j]));

				for (k = 0; k < len && !done; ++k) {

					check_secondary(get_vistied_index(indblock[k]));
					offset += block_size;
					if (offset >= totalSize)
					{
						done = 1;
						break;
					}
				}
			}
		}
	}
	free(indblock);
	free(dindblock);
	free(tindblock);
}


void check_block_bitmap()
{
	int i, j, temp;
	int blocks_per_group = EXT2_BLOCKS_PER_GROUP(super_block);
	int bitmap;
	int block_size = EXT2_BLOCK_SIZE(super_block);
	int group_desc_blocks = CEIL_FSCK(sizeof(struct ext2_group_desc) * total_number_of_groups, block_size);
	int inode_tables_block = CEIL_FSCK(sizeof(struct ext2_inode) * EXT2_INODES_PER_GROUP(super_block), block_size);
	int group_no;
	int correction = 0;
	if(EXT2_BLOCK_SIZE(super_block) > EXT2_MIN_BLOCK_SIZE)
		correction = 1;
	for (i = 0; i < total_number_of_groups; ++i)
	{
		/* Checking the superblock */
		bitmap = block_bitmap_status(i * blocks_per_group + correction, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap) 
		{
			printf("Pass 4: block %d : contains superblock. bitmap wrong: %d Vs %d\n", i * blocks_per_group + 1, 0, 1);
			check_secondary(i * blocks_per_group + 0);
		}

		/* Checking the group descriptor blocks */
		for (j =1; j <= group_desc_blocks; ++j) 
		{
			bitmap = block_bitmap_status(i * blocks_per_group + j + correction, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
			if (!bitmap) {
				printf("Pass 4: block %d : contains group_descriptors. bitmap wrong: %d Vs %d\n", i * blocks_per_group + j + 1, 0, 1);
				check_secondary(i * blocks_per_group + j);
			}
		}

		/* Checking the block bitmap */
		bitmap = block_bitmap_status(get_vistied_index(group_desc_table[i].bg_block_bitmap), block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap) 
		{
			printf("Pass 4: block %d : bitmap wrong: %d Vs %d\n", group_desc_table[i].bg_block_bitmap, 0, 1);
			check_secondary(get_vistied_index(group_desc_table[i].bg_block_bitmap));
		}


		/* Checking the inode bitmap */
		bitmap = block_bitmap_status(get_vistied_index(group_desc_table[i].bg_inode_bitmap), block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap)
		{
			printf("Pass 4: block %d : bitmap wrong: %d Vs %d\n", group_desc_table[i].bg_inode_bitmap, 0, 1);
			check_secondary(get_vistied_index(group_desc_table[i].bg_inode_bitmap));
		}

		/* Checking the inode table */
		temp = group_desc_table[i].bg_inode_table;
		for (j = temp - 1; j < temp + inode_tables_block; ++j)
		{
			bitmap = block_bitmap_status(j, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
			if (!bitmap) 
			{
				printf("Pass 4: block %d : bitmap wrong for inode table: %d Vs %d\n", j + 1, 0, 1);
				check_secondary(j);
			}
		}
	}

	inode_visited_map = calloc(sizeof(int) * total_number_of_groups * EXT2_INODES_PER_GROUP(super_block), sizeof(int));
	check_block(get_vistied_index(EXT2_ROOT_INO));
	free(inode_visited_map);
}
