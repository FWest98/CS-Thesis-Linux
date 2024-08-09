#include "asm/set_memory.h"
#include "asm/tlbflush.h"
#include "linux/fs.h"
#include "linux/gfp.h"
#include "linux/mm.h"
#include "linux/page-flags.h"
#include "linux/panic.h"
#include "linux/static_call_types.h"
#include "mm_helpers.h"
#include <linux/mm_types.h>
#include "dmap.h"

#define DMAP_START __PAGE_OFFSET
static struct direct_map_statistics current_statistics;

struct direct_map_descriptor dmap_find()
{
	return (struct direct_map_descriptor) {
		.start = DMAP_START,
		.end = pgd_contiguous_end(current->mm, DMAP_START)
	};
}

struct direct_map_statistics dmap_statistics()
{
	return current_statistics;
}

// Iterates over the direct map, splitting it up in "sections"
// of equal type (free/used/invalid) that we can manipulate at once.
// Calls callback handler for each such section.
void dmap_iterate(direct_map_section_handler handler)
{
	// First, we find the direct map boundaries
	struct direct_map_descriptor dmap = dmap_find();

	// In the next iteration, we will use the current section
	// descriptor as iteration variable: we will continuously try
	// to "extend" the current section by moving end by PAGE_SIZE.
	// Since end is exclusive, it will always point to the first address
	// in the next page. If the end points to a page of a different type
	// than the current section, we have reached the end of the section,
	// call the handler, and create a fresh new section from the current
	// position.
	//
	// TL;DR: "section.end" is treated as the current address in the iteration
	struct direct_map_section section = {
		.start = dmap.start, .end = dmap.start,
		.type = DIRECT_MAP_UNKNOWN
	};

	for(; section.end < dmap.end; section.end += PAGE_SIZE) {
		unsigned long pfn = __pa(section.end) >> PAGE_SHIFT;

		enum direct_map_section_type type = DIRECT_MAP_UNKNOWN;

		if(!pfn_valid(pfn)) {
			type = DIRECT_MAP_INVALID;
			goto handle_section;
		}

		struct page *page = virt_to_page(section.end);
		if(page_count(page) == 0) {
			type = DIRECT_MAP_FREE;
			goto handle_section;
		}

		if(page_has_type(page)) {
			type = DIRECT_MAP_USED_KERNEL;
			goto handle_section;
		}

		if(PageSlab(page)) {
			type = DIRECT_MAP_SLAB;
			goto handle_section;
		}

		if(PageAnon(page)) {
			type = DIRECT_MAP_ANON;
			goto handle_section;
		}

		// Not YET in userspace, but also not guaranteed to be kernel
		// space. Maybe we check other things such as RESERVED?
		if(!folio_mapped(page_folio(page))) {
			type = DIRECT_MAP_UNKNOWN;
			goto handle_section;
		}

		type = DIRECT_MAP_USED_USER;

handle_section:
		if(type == section.type) continue;

		// If the detected type of the next page is different from
		// the current section, we call the handler and reset.
		handler(section);
		section.type = type;
		section.start = section.end;
	}

	// At the very end of the loop, we have hit the end of the direct map,
	// so we emit one more section as complete.
	handler(section);
}

void dmap_unmap_section_always(struct direct_map_section section)
{
	int pagecount = (section.end - section.start) / PAGE_SIZE;
	set_memory_np_noalias_pgd(current->mm->pgd, section.start, pagecount);
}

void dmap_unmap_section(struct direct_map_section section)
{
	// Currently, we unmap only free and USED_USER sections
	if(section.type != DIRECT_MAP_FREE && section.type != DIRECT_MAP_USED_USER)
		return;

	dmap_unmap_section_always(section);
}

void dmap_statistics_section(struct direct_map_section section)
{
	// If the first address of this section is the start of the direct map,
	// we are in a new run of statistics so we clear the previous result.
	if(section.start == DMAP_START) {
		current_statistics = (struct direct_map_statistics) {0};
	}

	unsigned long page_count = (section.end - section.start) / PAGE_SIZE;

	switch(section.type) {
		case DIRECT_MAP_UNKNOWN: current_statistics.unknown += page_count; break;
		case DIRECT_MAP_FREE: current_statistics.free += page_count; break;
		case DIRECT_MAP_USED_USER: current_statistics.used_user += page_count; break;
		case DIRECT_MAP_USED_KERNEL: current_statistics.used_kernel += page_count; break;
		case DIRECT_MAP_INVALID: current_statistics.invalid += page_count; break;
		case DIRECT_MAP_ANON: current_statistics.anon += page_count; break;
		case DIRECT_MAP_SLAB: current_statistics.slab += page_count; break;
		default: break;
	}
}

void dmap_print_section(struct direct_map_section section)
{
	static unsigned long previous_print = 0;
	if(section.type == DIRECT_MAP_UNKNOWN) return;

	// When we have consecutive section prints, we skip the start address
	if(previous_print != section.start) {
		pr_info("[DMAP] >> 0x%lx", section.start);
		previous_print = section.start;
	}

	// We skip printing single pages to declutter the result
	if(section.end - section.start == PAGE_SIZE) return;

	// Print depending on the type
	switch(section.type) {
		case DIRECT_MAP_USED_KERNEL: {
			pr_info("[DMAP] | Used KERNEL for 0x%lx pages",
				(section.end - section.start) / PAGE_SIZE);
			break;
		}
		case DIRECT_MAP_USED_USER: {
			pr_info("[DMAP] | Used USER for 0x%lx pages",
				(section.end - section.start) / PAGE_SIZE);
			break;
		}
		case DIRECT_MAP_FREE: {
			pr_info("[DMAP] | Free for 0x%lx pages",
				(section.end - section.start) / PAGE_SIZE);
			break;
		}
		case DIRECT_MAP_INVALID: {
			pr_info("[DMAP] | Invalid for 0x%lx pages",
				(section.end - section.start) / PAGE_SIZE);
			break;
		}
		default: {
			pr_info("[DMAP] | Unknown type");
			break;
		}
	}

	// Print end location
	pr_info("[DMAP] >> 0x%lx", section.end);
	previous_print = section.end;
}

/////////////////////// TESTS ///////////////////////

void dmap_unmap_page_test()
{
	pr_info("[KVMISO_DMAP] Start testing unmapping a single page");

	// For the test, we first allocate a single page to play with
	struct page *page = alloc_pages(GFP_KERNEL, 0);
	unsigned long addr = (unsigned long) page_address(page);

	// Print current PTE
	dump_pt_walk(addr);

	// Unmap
	set_memory_np_noalias_pgd(current->mm->pgd, addr, 1);

	// Print again
	dump_pt_walk(addr);
	flush_tlb_all();

	// Remap
	set_memory_p_noalias_pgd(current->mm->pgd, addr, 1);

	// Return page
	free_pages(addr, 0);

	pr_info("[KVMISO_DMAP] Finished testing unmapping a single page");
}

void dmap_unmap_page_test_and_fault()
{
	pr_info("[KVMISO_DMAP] Start testing unmapping and reading a single page");

	// For this test, we unmap a page and then try to read/write it
	struct page *page = alloc_pages(GFP_KERNEL, 0);
	unsigned long addr = (unsigned long) page_address(page);

	// Print current PTE
	dump_pt_walk(addr);

	// Write a value
	*((int*) addr) = 123;

	// Unmap
	set_memory_np_noalias_pgd(current->mm->pgd, addr, 1);
	flush_tlb_all();

	// Print again
	dump_pt_walk(addr);

	// Read the value and cause a page fault
	int read = *((int*) addr);

	if(read != 123) {
		panic("[KVMISO_DMAP] The read-after-page fault yielded %d instead of 123!", read);
	}

	// Return page
	free_pages(addr, 0);

	pr_info("[KVMISO_DMAP] Finished testing unmapping and reading a single page");
}
