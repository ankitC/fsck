#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include "utils.h"

static int* inode_visited_map;

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

void setBlockBitmapSet(int block_number, unsigned char *block_bitmap, int group_number, int block_size, struct ext2_super_block *super_block, int setbit)
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


void check_secondary(int block_number)
{
	int group_no = block_number / super_block->s_blocks_per_group;
	int oldbit = block_bitmap_status(block_number, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block);
	if(oldbit == 0)
	{
		printf("Unallocated = %d\n", block_number);
		setBlockBitmapSet(block_number, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block, 1);
	}
}


void checkDataBlock(int localId)
{
	if (inode_visited_map[localId]) 
		return;
	inode_visited_map[localId] = 1;

	int block_size = EXT2_BLOCK_SIZE(super_block);

	struct ext2_inode *inode;
	int inode_index = (localId) % EXT2_INODES_PER_GROUP(super_block);
	inode = inode_table[localId/EXT2_INODES_PER_GROUP(super_block)] + inode_index;

	int tempSize;
	int totalSize = inode->i_size;
	if (is_directory(inode))
	{
		unsigned char *buf = calloc(inode->i_size, sizeof(char));
		get_data(partition_start_sector, inode, EXT2_BLOCK_SIZE(super_block), buf);
		tempSize = 0;
		struct ext2_dir_entry_2 *dir;
		while (tempSize < totalSize)
		{
			dir = (struct ext2_dir_entry_2 *)(buf + tempSize);
			if (dir->inode != 0)
				checkDataBlock(get_vistied_index(dir->inode));
			tempSize += dir->rec_len;
		}
		free(buf);
	}
	if (is_symbolic_link(inode))
	{
		if(inode->i_size > 60)
			check_secondary(inode->i_block[0]);
		return;
	}
	// set data block
	tempSize = 0;
	int i, j, k;
	int done = 0;

	int len = block_size / sizeof(int);
	int* indblock = calloc(block_size, sizeof(char));
	int* dindblock = calloc(block_size, sizeof(char));
	int* tindblock = calloc(block_size, sizeof(char));

	if(!done)
	{
		// Direct Blocks
		for (i = 0; i < EXT2_NDIR_BLOCKS && !done; ++i) {
			// inode->i_block[i]

			check_secondary(get_vistied_index(inode->i_block[i]));
			tempSize += block_size;
			if (tempSize >= totalSize)
			{
				done = 1;
				break;
			}
		}
	}
	if(!done)
	{
		// Indirect Blocks
		read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_IND_BLOCK], block_size, indblock);
		check_secondary(get_vistied_index(inode->i_block[EXT2_IND_BLOCK]));
		for (i = 0; i < len && !done;  ++i) {

			check_secondary(get_vistied_index(indblock[i]));
			tempSize += block_size;
			if (tempSize >= totalSize)
			{
				done = 1;
				break;
			}
		}
	}

	if(!done)
	{
		// Doubly-indirect Blocks
		read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_DIND_BLOCK], block_size, dindblock);
		check_secondary(get_vistied_index(inode->i_block[EXT2_DIND_BLOCK]));
		for (i = 0; i < len && !done; ++i) {
			read_bytes(partition_start_sector * SECTOR_SIZE + block_size * dindblock[i], block_size, indblock);

			check_secondary(get_vistied_index(dindblock[i]));
			for (j = 0; j < len && !done; ++j) {

				check_secondary(get_vistied_index(indblock[j]));
				tempSize += block_size;
				if (tempSize >= totalSize)
				{
					done = 1;
					break;
				}
			}
		}
	}

	if(!done)
	{
		// Triply-indirect Blocks
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
					tempSize += block_size;
					if (tempSize >= totalSize)
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
	int used = 0;
	int change = 0;
	int group_no;

	//check super_block & groupDescriptors & block bitmap & inode bitmap & inode table

	for (i = 0; i < total_number_of_groups; ++i)
	{
		//superBlock 0
		++used;
		//TODO: 0, 1, 3, 5, 7
		bitmap = block_bitmap_status(i * blocks_per_group + 0, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap) 
		{
			printf("block %d : contains superblock. bitmap wrong: %d Vs %d\n", i * blocks_per_group + 1, 0, 1);
			check_secondary(i * blocks_per_group + 0);
			change = 1;
		}

		//groupDescriptors 1 .. group_desc_blocks
		for (j = 1; j <= group_desc_blocks; ++j) 
		{
			++used;
			bitmap = block_bitmap_status(i * blocks_per_group + j, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
			if (!bitmap) {
				printf("block %d : contains group_descriptors. bitmap wrong: %d Vs %d\n", i * blocks_per_group + j + 1, 0, 1);
				change = 1;
				check_secondary(i * blocks_per_group + j);
			}
		}

		//block bitmap
		++used;
		bitmap = block_bitmap_status(get_vistied_index(group_desc_table[i].bg_block_bitmap), block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap) 
		{
			printf("block %d : bitmap wrong: %d Vs %d\n", group_desc_table[i].bg_block_bitmap, 0, 1);
			change = 1;
			check_secondary(get_vistied_index(group_desc_table[i].bg_block_bitmap));
		}


		//inode bitmap
		++used;
		bitmap = block_bitmap_status(get_vistied_index(group_desc_table[i].bg_inode_bitmap), block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap)
		{
			printf("block %d : bitmap wrong: %d Vs %d\n", group_desc_table[i].bg_inode_bitmap, 0, 1);
			change = 1;
			check_secondary(get_vistied_index(group_desc_table[i].bg_inode_bitmap));
		}

		//inode table
		temp = group_desc_table[i].bg_inode_table;
		for (j = temp - 1; j < temp + inode_tables_block; ++j)
		{
			++used;
			bitmap = block_bitmap_status(j, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
			if (!bitmap) 
			{
				printf("block %d : bitmap wrong for inode table: %d Vs %d\n", j + 1, 0, 1);
				change = 1;
				check_secondary(j);
			}
		}
	}
	// check data blstatic ock
	inode_visited_map = malloc(sizeof(int) * total_number_of_groups * EXT2_INODES_PER_GROUP(super_block));
	for (i = 0; i < total_number_of_groups * EXT2_INODES_PER_GROUP(super_block); ++i)
		inode_visited_map[i] = 0;
	checkDataBlock(get_vistied_index(EXT2_ROOT_INO));
	free(inode_visited_map);
}
