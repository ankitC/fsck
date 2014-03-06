#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include "utils.h"

/* Pass 4 checks the bitmaps to be consistent */
void pass4()
{
	int i;
	int bitmap;
	int change_req = 0;

	/* Check for discrepancies in the inode bitmap */
	for (i = get_vistied_index(EXT2_ROOT_INO); i < super_block->s_inodes_count; ++i) 
	{
		/* Skip the reserved inodes */
		if (i >= get_vistied_index(EXT2_ROOT_INO) && \
				i <= get_vistied_index(EXT2_FIRST_INO(super_block)))
			continue;

		bitmap = is_inode_allocated(i);
		if (bitmap && !inode_link[i])
		{
			/* Clear this inode from the inode bitmap */
			printf("Inode %d : bitmap is inconsistent with inode table:\
				   	%d Vs %d\n",i + 1, bitmap, inode_link[i]);
			set_inode_bitmap(i, 0);
			change_req = 1;
		}
		else if (!bitmap && inode_link[i])
		{
			/*Set the inode in the inode bitmap */
			printf("Inode %d : bitmap is inconsistent with inode table:\
				   	%d Vs %d\n", i + 1, bitmap, inode_link[i]);
			set_inode_bitmap(i, 1);
			change_req = 1;
		}
	}

	/* Write to the disk if any changes are required */
	if (change_req)
	{
		for (i = 0; i < total_number_of_groups; ++i)
		{
			write_bytes(partition_start_sector * sector_size_bytes
					+ group_desc_table[i].bg_inode_bitmap *
					EXT2_BLOCK_SIZE(super_block), EXT2_BLOCK_SIZE(super_block),
					inode_bitmap_table[i]);
		}
	}

	/* Find discrepancies in the block bitmaps */
	check_block_bitmap();

	/* Write changes to the block bitmap to the disk */
	for (i = 0; i < total_number_of_groups; ++i)
	{
		write_bytes(partition_start_sector * sector_size_bytes
				+ group_desc_table[i].bg_block_bitmap *
				EXT2_BLOCK_SIZE(super_block), EXT2_BLOCK_SIZE(super_block),
				block_bitmap_table[i]);
	}
}

