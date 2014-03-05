#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ext2_fs.h"
#include "utils.h"

int lost_found_dir_inode;

void move_to_lost_found(struct ext2_inode *lost, int inode_number) 
{
	int total_size = lost->i_size;
	unsigned char *buf = malloc(lost->i_size);
	get_data(partition_start_sector, lost, EXT2_BLOCK_SIZE(super_block), buf);
	int len = 2 + (int) floor(log10(inode_number));
	char *file_name = calloc(len, sizeof(char));
	sprintf(file_name, "#%d", inode_number);
	int offset = 0;
	int n = 0;
	struct ext2_dir_entry_2 *temp;
	struct ext2_dir_entry_2 *other;
	struct ext2_inode* inode;
	int group_number;
	int inode_index;
	while (offset < total_size) 
	{
		temp = (struct ext2_dir_entry_2 *)(buf + offset);
		++n;
		if (n > 2) 
		{ 
			// skip . and ..
			if (temp->inode == 0 && temp->rec_len >= EXT2_DIR_REC_LEN(len)) {
				temp->inode = inode_number;
				group_number = (inode_number - 1) / EXT2_INODES_PER_GROUP(super_block);
				inode_index = (inode_number - 1) % EXT2_INODES_PER_GROUP(super_block);
				inode = inode_table[group_number] + inode_index;
				temp->file_type = get_file_type(inode);
				temp->name_len = len;
				strcpy(temp->name, file_name);
				check_dir_inode(get_vistied_index(inode_number), lost_found_dir_inode);
				break;
			} 
			else if (temp->rec_len - EXT2_DIR_REC_LEN(temp->name_len) >= EXT2_DIR_REC_LEN(len)) 
			{
				other = (struct ext2_dir_entry_2 *)(buf + offset + EXT2_DIR_REC_LEN(temp->name_len));
				other->rec_len = temp->rec_len - EXT2_DIR_REC_LEN(temp->name_len);
				other->inode = inode_number;
				group_number = (inode_number - 1) / EXT2_INODES_PER_GROUP(super_block);
				inode_index = (inode_number - 1) % EXT2_INODES_PER_GROUP(super_block);
				inode = inode_table[group_number] + inode_index;
				other->file_type = get_file_type(inode);
				other->name_len = len;
				strcpy(other->name, file_name);
				temp->rec_len = EXT2_DIR_REC_LEN(temp->name_len);
				check_dir_inode(get_vistied_index(inode_number), lost_found_dir_inode);
				break;
			}
		}
		offset += temp->rec_len;
	}

	//Write Back
	//Assume only use first 12
	int i;
	offset = 0;
	for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) {
		
		write_bytes(partition_start_sector * sector_size_bytes + 
			EXT2_BLOCK_SIZE(super_block) * lost->i_block[i], 
			EXT2_BLOCK_SIZE(super_block), buf + offset);
		
		offset += EXT2_BLOCK_SIZE(super_block);
		if (offset >= total_size) 
			break;
	}

	free(buf);
	free(file_name);
}

void pass2()
{
	int i;
	struct ext2_inode *inode;
	struct ext2_inode *lost_found_inode;
	int group_number = 0;
	int inode_index = 0;
	/* Get the inode of lost+found directory */
	lost_found_dir_inode = find_dir_by_name(get_vistied_index(EXT2_ROOT_INO), "lost+found");
	if (lost_found_dir_inode == 0) {
		printf("lost+found directory is not found.\n");
		return;
	}

	/* Getting the lost+found inode */
//	group_number = lost_found_dir_inode / EXT2_INODES_PER_GROUP(super_block);
//	inode_index = lost_found_dir_inode % EXT2_INODES_PER_GROUP(super_block);
	lost_found_inode = find_inode_by_number(lost_found_dir_inode - 1);
	
	for (i = get_vistied_index(EXT2_ROOT_INO); i < super_block->s_inodes_count; ++i) 
	{
		if(i >=EXT2_GOOD_OLD_FIRST_INO)
		{
			inode_index = (i) % EXT2_INODES_PER_GROUP(super_block);
			group_number = i/EXT2_INODES_PER_GROUP(super_block);
			inode = inode_table[group_number] + inode_index;
			if (inode->i_links_count > 0 && inode_link[i] == 0) {
				printf("Inode %d should be put into lost+found\n", i + 1);
				move_to_lost_found(lost_found_inode, i + 1);
				// /update_parent(inode, lost_found_dir_inode, i);
			}
		}
	}
	pass1();
}
