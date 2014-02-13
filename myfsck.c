#include <stdio.h>
#include <stdlib.h>
#include "genhd.h"

#define SIGNATURE 0xaa55
#define MBR_ENTRIES 4
#define EBR_ENTRIES 1
#define SECTOR_SIZE 512

struct mbr_block
{
	char   mbr_code[446];
	struct partition entries[MBR_ENTRIES];
	unsigned short  signature;   /* Always equal to 0xaa55.  */
};

struct ebr_block
{
	char   ebr_code[446];
	struct partition ebr_entry;
	struct partition next_ebr;
	char   code[32];
	unsigned short  signature;   /* Always equal to 0xaa55.  */
};

struct mbr_entry
{
	int id;
	struct partition entry;
	struct mbr_entry* next_entry;
};

void check(char buf[])
{
	struct mbr_block* my_mbr =(struct mbr_block*) buf;
	unsigned int i = 0;
	unsigned int entry_number = 1;
	unsigned int next_sector_to_read = 0;
	char* ebr_buf[SECTOR_SIZE];

	struct mbr_entry* m_entry = NULL;
	struct mbr_entry* e_entry = NULL;

	struct mbr_entry* table_start = NULL;
	struct mbr_entry* table_end = NULL;

	if(my_mbr->signature != SIGNATURE)
		printf("Signature Mismatch! Abort!!\n");

	printf("Making primary partitions\n");

	/* Filling in primary partitions */
	for(i = 0; i<MBR_ENTRIES; i++)
	{
		printf("%d\n",i);
		fflush(stdout);
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

	printf("Finished primary partitions\n");
	m_entry = table_start;
	while(m_entry != NULL)
	{
		printf("%02x\t%d\t%d\n",m_entry->entry.sys_ind,\
		m_entry->entry.start_sect, m_entry->entry.nr_sects);
		m_entry = m_entry->next_entry;
	}

	printf("Making Extended Partitions\n");
	for(i=0; i<MBR_ENTRIES; i++)
	{
		//	printf("%d\t%02x\t%d\t%d\n",i,my_mbr->entries[i].sys_ind,\
		my_mbr->entries[i].start_sect, my_mbr->entries[i].nr_sects);

		if(my_mbr->entries[i].sys_ind == DOS_EXTENDED_PARTITION ||
				my_mbr->entries[i].sys_ind == LINUX_EXTENDED_PARTITION)
		{

			struct partition primary_ext_part = my_mbr->entries[i];
			unsigned int offset = primary_ext_part.start_sect;
			read_sectors(primary_ext_part.start_sect, 1, ebr_buf);
			while(1)
			{
				struct ebr_block* ebr_data = (struct ebr_block*)ebr_buf;
				if(ebr_data->signature == SIGNATURE)
				{
					e_entry = (struct mbr_entry*)calloc(1, sizeof(struct mbr_entry));
					e_entry->entry = ebr_data->ebr_entry;
					e_entry->next_entry = NULL;
					e_entry->id = entry_number++;
					e_entry->entry.start_sect += offset;
					table_end->next_entry = e_entry;
					table_end = e_entry;
					if(ebr_data->next_ebr.start_sect == 0)
						break;
					next_sector_to_read = offset+ebr_data->next_ebr.start_sect;
					read_sectors(next_sector_to_read, 1, ebr_buf);
					offset = next_sector_to_read;
				}
			}
		}
	}

	m_entry = table_start;
	while(m_entry != NULL)
	{
		printf("%02x\t%d\t%d\n",m_entry->entry.sys_ind,\
		m_entry->entry.start_sect, m_entry->entry.nr_sects);
		m_entry = m_entry->next_entry;
	}

}
