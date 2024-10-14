#include "asm/pgtable.h"
#include "asm/pgtable_types.h"
#include "asm/set_memory.h"
#include "asm/sev.h"
#include "asm/tlbflush.h"
#include "dmap.h"
#include "linux/gfp.h"
#include "linux/kvmiso.h"
#include "linux/memblock.h"
#include "linux/mm.h"
#include "linux/mmzone.h"
#include "linux/page_ref.h"
#include "mm_helpers.h"

void kvmiso_vm_init(struct kvm *kvm)
{
	printk("[KVMISO] KVM Init\n");

	dump_pt_walk((unsigned long) kvm->mm);

	// Determine direct map start and end - assuming contiguous map
	unsigned long map_start = __PAGE_OFFSET;
	unsigned long map_end = pgd_contiguous_end(current->mm, map_start);
	printk("[KVMISO] Direct map at 0x%lx until 0x%lx\n", map_start, map_end);

	// Clone the range for the direct map and make things explode
	dup_pgd_range(current->mm, map_start, map_end);
	dup_p4d_range(current->mm, map_start, map_end);
	dup_pud_range(current->mm, map_start, map_end);
	dup_pmd_range(current->mm, map_start, map_end);

	flush_tlb_all();

	// Iterate over all pages in the direct map
	dmap_iterate(&dmap_unmap_section);
	//dmap_iterate(&dmap_print_section);
	dmap_iterate(&dmap_statistics_section);

	struct direct_map_statistics statistics = dmap_statistics();
	printk("[KVMISO] Direct Map Statistics:");
	printk("             Free: 0x%lx pages", statistics.free);
	printk("             Invalid: 0x%lx pages", statistics.invalid);
	printk("             Used - User: 0x%lx pages", statistics.used_user);
	printk("             Used - Kernel: 0x%lx pages", statistics.used_kernel);
	printk("             Anon: 0x%lx pages", statistics.anon);
	printk("             SLAB: 0x%lx pages", statistics.slab);
	printk("             Unknown: 0x%lx pages", statistics.unknown);
}
EXPORT_SYMBOL(kvmiso_vm_init);

void kvmiso_kernel_split(struct kvm_vcpu *vcpu)
{
	printk("[KVMISO] Kernel Split");
}

void kvmiso_run(void)
{
	printk("[KVMISO] KVM Run");
	dump_pt_walk((unsigned long) current->mm);
}
EXPORT_SYMBOL(kvmiso_run);
