#ifndef __UTILS_
#define __UTILS_

#include "genhd.h"

#define SIGNATURE 0xaa55
#define MBR_ENTRIES 4
#define EBR_ENTRIES 1
#define MBR_SECTOR 0
int device;

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

void make_mbr();
void print_mbr();
void free_mbr();

extern void verify_partition(int partition_number);

#endif