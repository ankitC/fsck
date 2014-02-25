#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "genhd.h"
#include "utils.h"

int main (int argc, char **argv)
{
	unsigned char buf[sector_size_bytes];        /* temporary buffer */
	int           the_sector;                     /* IN: sector to read */
	int mbr_entry_to_inspect;
	int retval;
	char disk_image[40];
	char do_check = 0;
	int i;
	int partition_to_fix = 255;

	while((retval = getopt(argc, argv, "p:i:f:"))!= -1){
		switch(retval)
		{
			case 'p':
				mbr_entry_to_inspect = atoi(optarg);
				do_check = 1;
				break;
			case 'i':
				strcpy(disk_image, optarg);
				break;
			case 'f':
				partition_to_fix = atoi(optarg);
				break;
		}
	}

	if ((device = open(disk_image, O_RDWR)) == -1) {
		perror("Could not open device file");
		exit(-1);
	}

	make_mbr(device);

	if(do_check == 1)
	{
		check_mbr(mbr_entry_to_inspect);
	}

	if(partition_to_fix != 0)
	{
			if(get_mbr_type(partition_to_fix) == LINUX_EXT2_PARTITION)
				verify_partition(get_start_sector(partition_to_fix));
	}
	else
	{
		for(i = 0; i < total_mbr_entries; i++)
			if(get_mbr_type(i + 1) == LINUX_EXT2_PARTITION)
				verify_partition(get_start_sector(i + 1));
	}
	
	close(device);
	free_mbr();
	return 0;
}