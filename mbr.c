#include <stdio.h>
#include <stdlib.h>
#include "genhd.h"
#include "utils.h"

extern int device;
struct mbr_entry* table_start = NULL;
struct mbr_entry* table_end = NULL;
extern const unsigned int sector_size_bytes;
int total_mbr_entries = 0;

/* Constructing the MBR and storing it for reference */
void make_mbr(int device)
{
	unsigned char buf[sector_size_bytes];  
	unsigned int i = 0;
	unsigned int entry_number = 1;
	unsigned int next_sector_to_read = 0;
	char* ebr_buf[sector_size_bytes];
	struct mbr_entry* m_entry = NULL;
	struct mbr_entry* e_entry = NULL;
	struct ebr_block* ebr_data = NULL;


	read_sectors(MBR_SECTOR, 1, buf);
	struct mbr_block* my_mbr =(struct mbr_block*) buf;

	if(my_mbr->signature != SIGNATURE)
		printf("Signature Mismatch! Abort!!\n");

	/* Filling in primary partitions */
	for(i = 0; i<MBR_ENTRIES; i++)
	{
		m_entry = (struct mbr_entry*)calloc(1, sizeof(struct mbr_entry));
		m_entry->id = entry_number++;
		m_entry->next_entry = NULL;
		m_entry->entry = my_mbr->entries[i];
		if(table_end != NULL)
		table_end->next_entry = m_entry;
		table_end = m_entry;
		if(i == 0)
			table_start = m_entry;
	}


#ifdef DEBUG
	dm_entry = table_start;
	while(m_entry != NULL)
	{
		printf("%02x\t%d\t%d\n",m_entry->entry.sys_ind,\
		m_entry->entry.start_sect, m_entry->entry.nr_sects);
		m_entry = m_entry->next_entry;
	}
#endif

	/* Checking the extended partitions */
	for(i=0; i<MBR_ENTRIES; i++)
	{
		if(my_mbr->entries[i].sys_ind == DOS_EXTENDED_PARTITION ||
				my_mbr->entries[i].sys_ind == LINUX_EXTENDED_PARTITION)
		{

			struct partition primary_ext_part = my_mbr->entries[i];
			unsigned int ebr_offset = primary_ext_part.start_sect;
			unsigned int next_ebr_offset = primary_ext_part.start_sect;
			next_sector_to_read = primary_ext_part.start_sect;
			while(1)
			{
				read_sectors(next_sector_to_read, 1, ebr_buf);
				ebr_data = (struct ebr_block*)ebr_buf;
				if(ebr_data->signature == SIGNATURE)
				{
					e_entry = (struct mbr_entry*)calloc(1, sizeof(struct mbr_entry));
					e_entry->entry = ebr_data->ebr_entry;
					e_entry->next_entry = NULL;
					e_entry->id = entry_number++;
					e_entry->entry.start_sect += ebr_offset;
					table_end->next_entry = e_entry;
					table_end = e_entry;
					if(ebr_data->next_ebr.nr_sects == 0)
						break;
					next_sector_to_read = next_ebr_offset+ebr_data->next_ebr.start_sect;
					ebr_offset = next_sector_to_read;
				}
				else
				{
					printf("Invalid MBR:Signature %d\n", ebr_data->signature);
					break;
				}
			}
		}
	}
	total_mbr_entries = entry_number;
}

/* Checks the MBR table for the requested entry 
	Prints the record if requested entry is found
	else prints -1.
*/
void check_mbr(int entry_id)
{
	struct mbr_entry* m_entry = table_start;
	while(m_entry != NULL)
	{
		if(m_entry->id == entry_id)
		{
			printf("0x%02X %d %d\n", m_entry->entry.sys_ind,\
				m_entry->entry.start_sect, m_entry->entry.nr_sects);
			return;
		}
		m_entry = m_entry->next_entry;		
	}
	printf("-1\n");
	return;
}


/* Get the type of the mbr entry given the id */
unsigned char get_mbr_type(int entry_id)
{
	struct mbr_entry* m_entry = table_start;
	while(m_entry != NULL)
	{
		if(m_entry->id == entry_id)
		{
			return m_entry->entry.sys_ind;
		}
		m_entry = m_entry->next_entry;		
	}
	
	return 255;
}

/* Prints the complete mbr for debug */
void print_mbr()
{
	struct mbr_entry* m_entry = table_start;

	while(m_entry != NULL)
	{
		printf("%02x\t%d\t%d\n",m_entry->entry.sys_ind,\
		m_entry->entry.start_sect, m_entry->entry.nr_sects);
		m_entry = m_entry->next_entry;
	}
}

/* Free the MBR linked list at the end */
void free_mbr()
{
	
	struct mbr_entry* m_entry = table_start;
	struct mbr_entry* to_free;

	while(m_entry != NULL)
	{
		to_free = m_entry;
		m_entry = m_entry->next_entry;
		free(to_free);
	}
	table_start = NULL;
	table_end = NULL;
}

/* Get the start sector of a particular partition to perform fsck */
int get_start_sector(unsigned int partition_number)
{
	struct mbr_entry* m_entry = table_start;
	unsigned int start_sector = -1;
	unsigned int partition_to_check = partition_number;
	while(partition_number && m_entry != NULL)
	{
		partition_number--;
		if(partition_to_check == m_entry->id)
		{
			start_sector = m_entry->entry.start_sect;
			return start_sector;
		}
		m_entry = m_entry->next_entry;
	}
	return -1;
}