#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ext2_fs.h"
#include "utils.h"

int get_vistied_index(int index)
{
	return (index - 1);
}

struct ext2_inode* find_inode_by_number(int inode_number)
{
	int group_number = inode_number / EXT2_INODES_PER_GROUP(super_block);
	int inode_index = inode_number % EXT2_INODES_PER_GROUP(super_block);
	return inode_table[group_number] + inode_index;

}
int get_file_type(struct ext2_inode* inode)
{
	if (is_directory(inode)) 
		return EXT2_FT_DIR;
	if (is_reg_file(inode)) 
		return EXT2_FT_REG_FILE;
	if (is_symbolic_link(inode))
		return EXT2_FT_SYMLINK;

	return EXT2_FT_UNKNOWN;
}

int is_directory(struct ext2_inode* inode)
{
	if((inode->i_mode & 0xF000) == EXT2_S_IFDIR)
		return 1;
	else
		return 0;
}

int is_reg_file(struct ext2_inode* inode)
{	
	if((inode->i_mode & 0xF000) == EXT2_S_IFREG)
		return 1;
	else
		return 0;
}

int is_symbolic_link(struct ext2_inode* inode)
{
	if((inode->i_mode & 0xF000) == EXT2_S_IFLNK)
		return 1;
	else
		return 0;
}

int find_dir_by_name(int inode_number, const char *dir_name)
{
	int dir_inode_number = 0;
	struct ext2_dir_entry_2 *dir = 0;
	struct ext2_inode *root = inode_table[0] + inode_number;
	if (!is_directory(root)) {
		printf("inode %d is not a directory\n", inode_number + 1);
		return dir_inode_number;
	}
	unsigned char *buf = calloc(root->i_size, sizeof(char));
	get_data(partition_start_sector, root, root->i_size, buf);
	int offset = 0;
	int total_size = root->i_size;
	while (offset < total_size) {
		dir = (struct ext2_dir_entry_2 *)(buf + offset);
		if (strcmp(strndup(dir->name, dir->name_len), dir_name) == 0) {
			dir_inode_number = dir->inode;
			break;
		}
		offset += dir->rec_len;
	}
	free(buf);
	return dir_inode_number;
}

void get_data(int start_sector, struct ext2_inode *inode, int num_bytes, unsigned char* buf)
{
	int i, j, k;
	int offset = 0;
	int total_size = inode->i_size;
	int len = num_bytes / sizeof(int);
	int* indblock = calloc(num_bytes, sizeof(char));
	int* dindblock = calloc(num_bytes, sizeof(char));
	int* tindblock = calloc(num_bytes, sizeof(char));
	char done = 0;
	
	/* Direct Blocks */
	if(!done)
		for (i = 0; i < EXT2_NDIR_BLOCKS && !done; ++i) 
		{ 
			check_secondary(get_vistied_index(inode->i_block[i]));
			read_bytes((start_sector * SECTOR_SIZE + num_bytes * inode->i_block[i]), num_bytes, buf + offset);
			offset += num_bytes;
			//printf("Offset = %d\n", offset);
			if (offset >= total_size) 
			{
				done = 1;
				break;
			}
		}

	/* Checking Indirect Blocks */
	if(!done)
	{
		check_secondary(inode->i_block[EXT2_IND_BLOCK]);
		read_bytes(start_sector * SECTOR_SIZE + num_bytes * inode->i_block[EXT2_IND_BLOCK], num_bytes, indblock);
		for (i = 0; i < len && !done;  ++i) 
		{
			check_secondary(get_vistied_index(indblock[i]));
			read_bytes(start_sector * SECTOR_SIZE + num_bytes * indblock[i], num_bytes, buf + offset);
			offset += num_bytes;
			if (offset >= total_size) 
			{
				done = 1;
				break;
			}
		}
	}
	
	/* Checking Doubly Indirect Blocks */
	if(!done)
	{
		check_secondary(inode->i_block[EXT2_DIND_BLOCK]);
		read_bytes(start_sector * SECTOR_SIZE + num_bytes * inode->i_block[EXT2_DIND_BLOCK], num_bytes, dindblock);
		for (i = 0; i < len && !done; ++i) 
		{
			check_secondary(get_vistied_index(dindblock[i]));
			read_bytes(start_sector * SECTOR_SIZE + num_bytes * dindblock[i], num_bytes, indblock);
			for (j = 0; j < len && !done; ++j) 
			{
				check_secondary(get_vistied_index(indblock[j]));
				read_bytes(start_sector * SECTOR_SIZE + num_bytes * indblock[j], num_bytes, buf + offset);
				offset += num_bytes;
				if (offset >= total_size)
				{ 
					done = 1;
					break;
				}
			}
		}
	}

	/* Checking triply indirect block */
	if(!done)
	{
		check_secondary(inode->i_block[EXT2_TIND_BLOCK]);
		read_bytes(start_sector * SECTOR_SIZE + num_bytes * inode->i_block[EXT2_TIND_BLOCK], num_bytes, tindblock);
		for (i = 0; i < len && !done; ++i) 
		{
			check_secondary(get_vistied_index(tindblock[i]));
			read_bytes(start_sector * SECTOR_SIZE + num_bytes * tindblock[i], num_bytes, dindblock);
			for (j = 0; j < len && !done; ++j) 
			{
				check_secondary(get_vistied_index(dindblock[j]));
				read_bytes(start_sector * SECTOR_SIZE + num_bytes * dindblock[j], num_bytes, indblock);
				for (k = 0; k < len && !done; ++k) 
				{
					check_secondary(get_vistied_index(indblock[k]));
					read_bytes(start_sector * SECTOR_SIZE + num_bytes * indblock[k], num_bytes, buf + offset);
					offset += num_bytes;
					if (offset >= total_size)
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
