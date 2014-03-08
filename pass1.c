#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "ext2_fs.h"

extern unsigned int partition_start_sector;
extern struct ext2_super_block* super_block;
extern struct ext2_inode** inode_table;
extern int* inode_link;
extern struct ext2_super_block* super_block;

void check_dir_inode(int inode_number, int parent);
void check_dir(struct ext2_inode *inode, unsigned char *buf, int inode_number, int parent);

/* Pass 1 checks for . and .. to be set correctly for all directories */
void pass1()
{
	check_dir_inode(get_vistied_index(EXT2_ROOT_INO), EXT2_ROOT_INO);
	inode_link[get_vistied_index(EXT2_ROOT_INO)]--;
}


/* Check the directory inode and read its contents to make sure all is OK */
void check_dir_inode(int inode_number, int parent)
{
	struct ext2_inode *inode;

	/* Increment the link count to the inode as it is visited */
	inode_link[inode_number] = inode_link[inode_number] + 1;

	/* If already visited inode, then return */
	if (inode_link[inode_number] > 1)
		return;
	/* Get the inode and read it later */
	inode = find_inode_by_number(inode_number);

	if (!is_directory(inode))
		return;

	unsigned char *buf = calloc(inode->i_size, sizeof(char));
	/* Get the data in the inode in buf for analysis */
	get_data(partition_start_sector, inode, EXT2_BLOCK_SIZE(super_block), buf);

	/* Check the dir from its data now */
	check_dir(inode, buf, inode_number, parent);
	free(buf);
}


/* Traverse the directory and perform consistency checks specified by pass 1 */
void check_dir(struct ext2_inode *inode, unsigned char *buf, int inode_number, int parent)
{
	int correct_error = 0;
	int n = 0;
	struct ext2_dir_entry_2 *dir;

	/* 'offset' is the distance traversed in the dir and
	 * we traverse till offset == directory size*/
	int offset = 0;
	int dir_size = inode->i_size;


	while (offset < dir_size)
	{
		correct_error = 0;
		dir = (struct ext2_dir_entry_2 *)(buf + offset);

#ifdef DEBUG
		printf("Directory Name: %s Type: %d\n",dir->name, dir->file_type);
#endif

		++n;
		/* Check for . */
		if (n == 1)
		{
			if (dir->name_len == 1 && strcmp(strndup(dir->name, dir->name_len), ".") == 0) 
			{
				if (dir->inode != (inode_number + 1))
				{
					printf("Pass1: Dir Inode %d: . has wrong dir->inode %d and name %s, change to itself\n", inode_number + 1, dir->inode, dir->name);
					dir->inode = inode_number + 1;
					correct_error = 1;
				}
			}
			else
			{
				printf("Pass1: Dir Inode %d: first dir is not ., change to .\n", inode_number + 1);
				dir->name_len = 1;
				dir->name[0] = '.';
				dir->inode = inode_number + 1;
				correct_error = 1;
			}

			check_dir_inode(get_vistied_index(dir->inode), inode_number + 1);
		}

		/* Check for .. to point to the parent */
		else if (n == 2)
		{
			if (dir->name_len == 2 && strcmp(strndup(dir->name, dir->name_len), "..") == 0)
			{
				if (dir->inode != parent)
				{
					printf("Pass1: Dir Inode %d: .. has wrong dir->inode %d, change to its parent\n", inode_number + 1, dir->inode);
					dir->inode = parent;
					correct_error = 1;
				}
			}
			else
			{
				printf("Pass1: Dir Inode %d: second dir is not .., change to ..\n", inode_number + 1);
				dir->name_len = 2;
				dir->name[0] = '.';
				dir->name[1] = '.';
				dir->inode = parent;
				correct_error = 1;
			}
			check_dir_inode(get_vistied_index(dir->inode), inode_number + 1);
		}
		else
		{
			if (dir->inode != 0)
				check_dir_inode(get_vistied_index(dir->inode), inode_number + 1);
		}

		/* Write to disk and correct the errors if required */
		if (correct_error)
			write_bytes(partition_start_sector * SECTOR_SIZE + inode->i_block[0] * EXT2_BLOCK_SIZE(super_block) + offset, dir->rec_len, dir);

		/* Move ahead in the traversal */
		offset += dir->rec_len;
	}
}

