#ifndef __KVMISO_DMAP_H__
#define __KVMISO_DMAP_H__

struct direct_map_descriptor {
	unsigned long start;
	unsigned long end; // exclusive
};

struct direct_map_statistics {
	unsigned long free;
	unsigned long used_kernel;
	unsigned long used_user;
	unsigned long invalid;
	unsigned long unknown;
};

enum direct_map_section_type {
	DIRECT_MAP_UNKNOWN,
	DIRECT_MAP_USED_KERNEL,
	DIRECT_MAP_USED_USER,
	DIRECT_MAP_FREE,
	DIRECT_MAP_INVALID,
};

struct direct_map_section {
	unsigned long start;
	unsigned long end; // exclusive
	enum direct_map_section_type type;
};

typedef void (*direct_map_section_handler) (struct direct_map_section);

// Methods

struct direct_map_descriptor dmap_find(void);
struct direct_map_statistics dmap_statistics(void);

void dmap_iterate(direct_map_section_handler);

void dmap_unmap_section(struct direct_map_section);
void dmap_unmap_section_always(struct direct_map_section);
void dmap_print_section(struct direct_map_section);
void dmap_statistics_section(struct direct_map_section);

void dmap_unmap_page_test(void);
void dmap_unmap_page_test_and_fault(void);

#endif
