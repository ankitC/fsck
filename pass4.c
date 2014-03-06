#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include "utils.h"


unsigned char *new_bitmap;
int change;

void pass4() {
	int i;
	int bitmap;
	int change = 0;
	// check Inode Bitmap
	for (i = get_vistied_index(EXT2_ROOT_INO); i < super_block->s_inodes_count; ++i) 
	{
		if (i >= get_vistied_index(EXT2_ROOT_INO) && i <= get_vistied_index(EXT2_FIRST_INO(super_block))) 
				continue; 
		bitmap = is_inode_allocated(i);
		if (bitmap && !inode_link[i]) { // set to 0
			printf("Inode %d : bitmap is inconsistent with inode table: %d Vs %d\n", i + 1, bitmap, inode_link[i]);
			set_inode_bitmap(i, 0);
			change = 1;
		} else if (!bitmap && inode_link[i]) { // set to 1
			printf("Inode %d : bitmap is inconsistent with inode table: %d Vs %d\n", i + 1, bitmap, inode_link[i]);
			set_inode_bitmap(i, 1);
			change = 1;
		}
	}
	if (change) 
	{
		for (i = 0; i < total_number_of_groups; ++i)
			write_bytes(partition_start_sector * sector_size_bytes
			 + group_desc_table[i].bg_inode_bitmap * 
			 EXT2_BLOCK_SIZE(super_block), EXT2_BLOCK_SIZE(super_block), 
			 inode_bitmap_table[i]);
	}

	// check Block Bitmap
	new_bitmap = calloc(EXT2_BLOCK_SIZE(super_block) * total_number_of_groups, sizeof(char));
	
	check_block_bitmap();
	// replace blockBitmap with new_bitmap
#if 0
	int oldbit;
	int newbit;
	int group_no;
//	printf("%d\n",super_block.s_blocks_count);
	for (i = 0; i < super_block->s_blocks_count - 1; ++i)
	 {
		group_no = (i) / EXT2_BLOCKS_PER_GROUP(super_block);
		//printf("Checking old bitmap for inode: %d\n", i);
		oldbit = isBlockBitmapSet(i, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block);
		newbit = isBlockBitmapSet(i, new_bitmap, total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block);
		if (oldbit != newbit && oldbit == 0) {
			printf("block bitmap: block %d: %d Vs %d\n", i + 1, oldbit, newbit);
			setBlockBitmapSet(i, block_bitmap_table[group_no], total_number_of_groups, EXT2_BLOCK_SIZE(super_block), super_block, newbit);
		}
	}
#endif
	for (i = 0; i < total_number_of_groups; ++i)
		write_bytes(partition_start_sector * SECTOR_SIZE + group_desc_table[i].bg_block_bitmap * EXT2_BLOCK_SIZE(super_block), EXT2_BLOCK_SIZE(super_block), block_bitmap_table[i]);

}

