#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "ext2_fs.h"

/* Pass 3 checks the reference count of each directory set in its inode
 * to be equal to the observed reference count during traversal */
void pass3()
{
	int i;
	struct ext2_inode *inode;
	int inode_index, group_number;

	for (i = get_vistied_index(EXT2_ROOT_INO); i< super_block->s_inodes_count; ++i) 
	{
		inode_index = (i) % EXT2_INODES_PER_GROUP(super_block);
		group_number = i/EXT2_INODES_PER_GROUP(super_block);
		inode = find_inode_by_number(i);
		if (inode->i_links_count != inode_link[i])
		{
			printf("Pass 3: Inode %d inconsistent link count: %d Vs %d\n", i + 1, inode->i_links_count, inode_link[i]);
			inode->i_links_count = inode_link[i];
			write_bytes(partition_start_sector * sector_size_bytes +
					group_desc_table[group_number].bg_inode_table * EXT2_BLOCK_SIZE(super_block)
					+ EXT2_INODE_SIZE(super_block) * inode_index,
					EXT2_INODE_SIZE(super_block),
					inode_table[group_number] + inode_index);
		}
	}
}
