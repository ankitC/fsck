#ifndef __UTILS_
#define __UTILS_

#include "genhd.h"
#include "ext2_fs.h"

#define SIGNATURE 0xaa55
#define MBR_ENTRIES 4
#define EBR_ENTRIES 1
#define MBR_SECTOR 0
#define SECTOR_SIZE sector_size_bytes


int device;
extern const unsigned int sector_size_bytes;
extern int total_mbr_entries;

/* Superblock.c */
extern unsigned int partition_start_sector;
extern struct ext2_super_block* super_block;
extern struct ext2_inode** inode_table;
extern int* inode_link;
extern struct ext2_super_block* super_block;
extern struct ext2_group_desc* group_desc_table;
extern int lost_found_dir_inode;

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

/* MBR functions API */
void make_mbr();
void print_mbr();
void free_mbr();
void check_mbr(int entry_id);

void verify_partition(int partition_number);

/* Util functions API */
void get_data(int start_sector, struct ext2_inode* inode, int num_bytes, unsigned char* buf);
int get_vistied_index(int index);
int is_directory(struct ext2_inode* inode);
int find_dir_by_name(int inode_number, const char *dir_name);
int get_file_type(struct ext2_inode* inode);
struct ext2_inode* find_inode_by_number(int inode_number);

void pass1();
void pass2();
void pass3();

void check_dir_inode(int inode_number, int parent);

#endif
