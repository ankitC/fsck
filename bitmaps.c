#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include "utils.h"

int* inodeFlag;
extern unsigned char *new_bitmap;

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

int isBlockBitmapSet(int blockNo, unsigned char *blockBitmap, int groupNum, int blockSize, struct ext2_super_block *superBlock)
{
	int groupNo;
	int groupBlockNo;
	//getGroupBlock(blockNo, &groupNo, &groupBlockNo, superBlock);
	groupNo = blockNo / superBlock->s_blocks_per_group;
	groupBlockNo = blockNo % superBlock->s_blocks_per_group;
	int index;
	int bit;
	//getIndexBit(groupBlockNo, &index, &bit);
	if(blockNo >= 1921 && blockNo <= 1923)
		printf("BOLD::CAME TO %d \n", blockNo);
	if(blockNo >= 1349 && blockNo <= 1351)
		printf("BOLD::CAME TO %d \n", blockNo);
	if(blockNo >= 1793 && blockNo <= 1795)
		printf("BOLD::CAME TO %d \n", blockNo);
	if(blockNo >= 1944 && blockNo <= 1946)
		printf("BOLD::CAME TO %d \n", blockNo);

	index = groupBlockNo / 8;
	bit = groupBlockNo % 8;
	return (blockBitmap[index] & (1 << bit)) >> bit;
}

void setBlockBitmapSet(int blockNo, unsigned char *blockBitmap, int groupNum, int blockSize, struct ext2_super_block *superBlock, int setbit)
{
	int groupNo;
	int groupBlockNo;
	//getGroupBlock(blockNo, &groupNo, &groupBlockNo, superBlock);
	groupNo = blockNo / superBlock->s_blocks_per_group;
	groupBlockNo = blockNo % superBlock->s_blocks_per_group;
	int index;
	int bit;
	//getIndexBit(groupBlockNo, &index, &bit);
	index = groupBlockNo / 8;
	bit = groupBlockNo % 8;
	if (setbit) 
		blockBitmap[groupNo * blockSize + index] |= (1 << bit);
	else
		blockBitmap[groupNo * blockSize + index] &= (~(1 << bit));
	//printf("Returning from Set\n");
}


void check_secondary(int blockNo)
{
	int group_no = blockNo / super_block->s_blocks_per_group;
	int oldbit = isBlockBitmapSet(blockNo, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block);
	if(oldbit == 0)
	{
		printf("Unallocated = %d\n", blockNo);
		setBlockBitmapSet(blockNo, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block, 1);
	}

}


void checkDataBlock(int localId)
{
	//printf("Inode %d\n",localId);
	if (inodeFlag[localId]) 
		return;
	inodeFlag[localId] = 1;

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
		return;
	// set data block
	tempSize = 0;
	int i, j, k;
	
	int len = block_size / sizeof(int);
	int* indblock = calloc(block_size, sizeof(char));
	int* dindblock = calloc(block_size, sizeof(char));
	int* tindblock = calloc(block_size, sizeof(char));

	// Direct Blocks
	for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) {
		// inode->i_block[i]
		setBlockBitmapSet(get_vistied_index(inode->i_block[i]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
		check_secondary(get_vistied_index(inode->i_block[i]));
		tempSize += block_size;
		if (tempSize >= totalSize) goto FREE_MEMORY;
	}
	
	// Indirect Blocks
	read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_IND_BLOCK], block_size, indblock);
	setBlockBitmapSet(get_vistied_index(inode->i_block[EXT2_IND_BLOCK]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
	check_secondary(get_vistied_index(inode->i_block[EXT2_IND_BLOCK]));
	for (i = 0; i < len;  ++i) {
		setBlockBitmapSet(get_vistied_index(indblock[i]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
		check_secondary(get_vistied_index(indblock[i]));
		tempSize += block_size;
		if (tempSize >= totalSize) goto FREE_MEMORY;
	}
	
	// Doubly-indirect Blocks
	read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_DIND_BLOCK], block_size, dindblock);
	setBlockBitmapSet(get_vistied_index(inode->i_block[EXT2_DIND_BLOCK]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
	check_secondary(get_vistied_index(inode->i_block[EXT2_DIND_BLOCK]));

	for (i = 0; i < len; ++i) {
		read_bytes(partition_start_sector * SECTOR_SIZE + block_size * dindblock[i], block_size, indblock);
		setBlockBitmapSet(get_vistied_index(dindblock[i]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
		check_secondary(get_vistied_index(dindblock[i]));
		for (j = 0; j < len; ++j) {
			setBlockBitmapSet(get_vistied_index(indblock[j]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
			check_secondary(get_vistied_index(indblock[i]));
			tempSize += block_size;
			if (tempSize >= totalSize) goto FREE_MEMORY;
		}
	}

	// Triply-indirect Blocks
	read_bytes(partition_start_sector * SECTOR_SIZE + block_size * inode->i_block[EXT2_TIND_BLOCK], block_size, tindblock);
	setBlockBitmapSet(get_vistied_index(inode->i_block[EXT2_TIND_BLOCK]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
	check_secondary(get_vistied_index(inode->i_block[EXT2_TIND_BLOCK]));
	
	for (i = 0; i < len; ++i) {
		read_bytes(partition_start_sector * SECTOR_SIZE + block_size * tindblock[i], block_size, dindblock);
		setBlockBitmapSet(get_vistied_index(tindblock[i]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
		check_secondary(get_vistied_index(tindblock[i]));
		
		for (j = 0; j < len; ++j) {
			read_bytes(partition_start_sector * SECTOR_SIZE + block_size * dindblock[j], block_size, indblock);
			setBlockBitmapSet(get_vistied_index(dindblock[j]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
			check_secondary(get_vistied_index(dindblock[j]));
			
			for (k = 0; k < len; ++k) {
				setBlockBitmapSet(get_vistied_index(indblock[k]), new_bitmap, total_number_of_groups, block_size, super_block, 1);
				check_secondary(get_vistied_index(indblock[k]));
				tempSize += block_size;
				if (tempSize >= totalSize) goto FREE_MEMORY;
			}
		}
	}

	FREE_MEMORY:
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

	//check superBlock & groupDescriptors & block bitmap & inode bitmap & inode table

	for (i = 0; i < total_number_of_groups; ++i)
	{
		//superBlock 0
		++used;
		bitmap = isBlockBitmapSet(i * blocks_per_group + 0, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap) 
		{
			printf("block %d : contains superblock. bitmap wrong: %d Vs %d\n", i * blocks_per_group + 1, 0, 1);
			check_secondary(i * blocks_per_group + 0);
			change = 1;
		}
		setBlockBitmapSet(i * blocks_per_group + 0, new_bitmap, total_number_of_groups, block_size, super_block, 1);
		//groupDescriptors 1 .. group_desc_blocks
		for (j = 1; j <= group_desc_blocks; ++j) 
		{
			++used;
			bitmap = isBlockBitmapSet(i * blocks_per_group + j, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
			if (!bitmap) {
				printf("block %d : contains group_descriptors. bitmap wrong: %d Vs %d\n", i * blocks_per_group + j + 1, 0, 1);
				change = 1;
				check_secondary(i * blocks_per_group + j);
			}
			setBlockBitmapSet(i * blocks_per_group + j, new_bitmap, total_number_of_groups, block_size, super_block, 1);
		}

		//block bitmap
		++used;
		bitmap = isBlockBitmapSet(get_vistied_index(group_desc_table[i].bg_block_bitmap), block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap) 
		{
			printf("block %d : bitmap wrong: %d Vs %d\n", group_desc_table[i].bg_block_bitmap, 0, 1);
			change = 1;
			check_secondary(get_vistied_index(group_desc_table[i].bg_block_bitmap));
		}
		setBlockBitmapSet(get_vistied_index(group_desc_table[i].bg_block_bitmap), new_bitmap, total_number_of_groups, block_size, super_block, 1);

		//inode bitmap
		++used;
		bitmap = isBlockBitmapSet(get_vistied_index(group_desc_table[i].bg_inode_bitmap), block_bitmap_table[i], total_number_of_groups, block_size, super_block);
		if (!bitmap)
		{
			printf("block %d : bitmap wrong: %d Vs %d\n", group_desc_table[i].bg_inode_bitmap, 0, 1);
			change = 1;
			check_secondary(get_vistied_index(group_desc_table[i].bg_inode_bitmap));
		}
		setBlockBitmapSet(get_vistied_index(group_desc_table[i].bg_inode_bitmap), new_bitmap, total_number_of_groups, block_size, super_block, 1);
		
		//inode table
		temp = group_desc_table[i].bg_inode_table;
		for (j = temp - 1; j < temp + inode_tables_block; ++j)
		{
			++used;
			bitmap = isBlockBitmapSet(j, block_bitmap_table[i], total_number_of_groups, block_size, super_block);
			if (!bitmap) 
			{
				printf("block %d : bitmap wrong for inode table: %d Vs %d\n", j + 1, 0, 1);
				change = 1;
				check_secondary(j);
			}
			setBlockBitmapSet(j, new_bitmap, total_number_of_groups, block_size, super_block, 1);
		}
	}
	// check data block
	inodeFlag = malloc(sizeof(int) * total_number_of_groups * EXT2_INODES_PER_GROUP(super_block));
	for (i = 0; i < total_number_of_groups * EXT2_INODES_PER_GROUP(super_block); ++i)
		inodeFlag[i] = 0;
	checkDataBlock(get_vistied_index(EXT2_ROOT_INO));
	free(inodeFlag);
}
