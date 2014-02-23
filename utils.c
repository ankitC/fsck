#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define SECTOR_SIZE 512

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

//	printf("Start Sector %d\n", start_sector);
//	printf("Inode Size = %d\n", inode->i_size );
	// Direct Blocks
	if(!done)
	for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) 
	{ 
		read_bytes((start_sector + 2* inode->i_block[i]) * SECTOR_SIZE, num_bytes, buf + offset);
		offset += num_bytes;
		//printf("Offset = %d\n", offset);
		if (offset >= total_size) 
		{
			done = 1;
			break;
		}
	}
	
	// Indirect Blocks
	if(!done)
	{
	read_bytes(start_sector * SECTOR_SIZE + num_bytes * inode->i_block[EXT2_IND_BLOCK], num_bytes, indblock);
	for (i = 0; i < len;  ++i) 
	{
		read_bytes(start_sector * SECTOR_SIZE + num_bytes * indblock[i], num_bytes, buf + offset);
		offset += num_bytes;
		if (offset >= total_size) 
		{
			done = 1;
			break;
		}
	}
	}
	// Doubly-indirect Blocks
	if(!done)
	{
	read_bytes(start_sector * SECTOR_SIZE + num_bytes * inode->i_block[EXT2_DIND_BLOCK], num_bytes, dindblock);
	for (i = 0; i < len; ++i) 
	{
		read_bytes(start_sector * SECTOR_SIZE + num_bytes * dindblock[i], num_bytes, indblock);
		for (j = 0; j < len; ++j) 
		{
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
	// Thriply-indirect Blocks
	if(!done)
	{
	read_bytes(start_sector * SECTOR_SIZE + num_bytes * inode->i_block[EXT2_TIND_BLOCK], num_bytes, tindblock);
	for (i = 0; i < len; ++i) 
	{
		read_bytes(start_sector * SECTOR_SIZE + num_bytes * tindblock[i], num_bytes, dindblock);
		for (j = 0; j < len; ++j) 
		{
			read_bytes(start_sector * SECTOR_SIZE + num_bytes * dindblock[j], num_bytes, indblock);
			for (k = 0; k < len; ++k) 
			{
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