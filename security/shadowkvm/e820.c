#include "linux/memblock.h"
#include "linux/printk.h"
#include "linux/types.h"

static phys_addr_t map_start = 0x0000000100000000;
static phys_addr_t map_size  = 0x0000000100000000;


void __init shadowkvm_e820_block(void)
{
	memblock_remove(map_start, map_size);
	memblock_reserve(map_start, map_size);
	//memblock_mark_nomap(map_start, map_size);

	printk("SHADOWKVM: Reserved memory range %llu to %llu as NOMAP\n",\
		map_start, map_start + map_size);
	memblock_dump_all();
}
