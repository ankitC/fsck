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

#define S_IFDIR  0040000
#define S_IFMT  00170000
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define SECTOR_SIZE 512

void check_dir_inode(int inode_number, int parent);

int get_vistied_index(int index)
{
	return (index - 1);
}

void check_dir(struct ext2_inode *inode, unsigned char *buf, int inode_number, int parent) {
	int correct_error = 0;
	int n = 0;
	int offset = 0;
	int dir_size = inode->i_size;
	struct ext2_dir_entry_2 *dir;
	while (offset < dir_size) 
	{
		correct_error = 0;
		dir = (struct ext2_dir_entry_2 *)(buf + offset);
		++n;
		if (n == 1) 
		{ // .
			if (dir->name_len == 1 && strcmp(strndup(dir->name, dir->name_len), ".") == 0) 
			{
				if (dir->inode != (inode_number + 1)) 
				{
					printf("Error: Dir Inode %d: . has wrong dir->inode %d and name %s, change to itself\n", inode_number + 1, dir->inode, dir->name);
					dir->inode = inode_number + 1;
					correct_error = 1;
				}
			} 
			else 
			{
				printf("Error: Dir Inode %d: first dir is not ., change to .\n", inode_number + 1);
				dir->name_len = 1;
				dir->name[0] = '.';
				dir->inode = inode_number + 1;
				correct_error = 1;
			}
			//printf("Checking next dir with inode %d\n", dir->inode);
			check_dir_inode(get_vistied_index(dir->inode), inode_number + 1);
		} 
		else if (n == 2) 
		{ // ..
			if (dir->name_len == 2 && strcmp(strndup(dir->name, dir->name_len), "..") == 0) {
				if (dir->inode != parent) {
					printf("Error: Dir Inode %d: .. has wrong dir->inode %d, change to its parent\n", inode_number + 1, dir->inode);
					dir->inode = parent;
					correct_error = 1;
				}
			} else {
				printf("Error: Dir Inode %d: second dir is not .., change to ..\n", inode_number + 1);
				dir->name_len = 2;
				dir->name[0] = '.';
				dir->name[1] = '.';
				dir->inode = inode_number + 1;
				correct_error = 1;
			}
			check_dir_inode(get_vistied_index(dir->inode), inode_number + 1);
		} 
		else 
		{
			if (dir->inode != 0)
				check_dir_inode(get_vistied_index(dir->inode), inode_number + 1);
		}
		if (correct_error)
			printf("Error to be corrected found\n");
			write_bytes(partition_start_sector * SECTOR_SIZE + inode->i_block[0] * EXT2_BLOCK_SIZE(super_block) + offset, dir->rec_len, dir);
		offset += dir->rec_len;
		}
}

void check_dir_inode(int inode_number, int parent)
{
	struct ext2_inode *inode;
	
	inode_link[inode_number] = inode_link[inode_number] + 1;
	if(inode_number == 0)
		return;
	if (inode_link[inode_number] > 1)
		return;
	
	int inode_index = (inode_number) % EXT2_INODES_PER_GROUP(super_block);
	inode = inode_table[inode_number/EXT2_INODES_PER_GROUP(super_block)] + inode_index;
	if (!S_ISDIR(inode->i_mode))
		return;
	
	unsigned char *buf = calloc(inode->i_size, sizeof(char));
	get_data(partition_start_sector, inode, EXT2_BLOCK_SIZE(super_block), buf);
	check_dir(inode, buf, inode_number, parent);
	free(buf);
}

void pass1()
{
	check_dir_inode(get_vistied_index(EXT2_ROOT_INO), EXT2_ROOT_INO);
	inode_link[get_vistied_index(EXT2_ROOT_INO)]--;
}
