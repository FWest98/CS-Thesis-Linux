#include "asm/pgtable_types.h"
#include "asm/set_memory.h"
#include "asm/sev.h"
#include "asm/tlbflush.h"
#include "linux/gfp.h"
#include "linux/kvmiso.h"
#include "linux/memblock.h"
#include "mm_helpers.h"

void kvmiso_vm_init(struct kvm *kvm)
{
	printk("[KVMISO] KVM Init\n");

	memblock_dump_all();

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

	dump_pud_addr(current->mm, map_start);
	dump_pt_walk((unsigned long) kvm->mm);

	// Remove free pages from the direct map
	int order = 5;
	struct page *gigabyte = alloc_pages(GFP_KERNEL, order);
	unsigned long gigaddr = (unsigned long) page_address(gigabyte);

	for(int i = 0; i < (1 << order); i++) {
		*((int*) gigaddr + i * PAGE_SIZE) = i;
	}

	dump_pt_walk(gigaddr);
	//dump_pt_walk_mm(gigaddr, &init_mm);

	set_memory_np_noalias_pgd(current->mm->pgd, gigaddr, 1 << order);

	flush_tlb_all();
	dump_pt_walk(gigaddr);
	dump_pte_addr(current->mm, gigaddr);
	//dump_pt_walk_mm(gigaddr, &init_mm);

	printk("[KVMISO] Trying to read unmapped pages");

	for(int i = 0; i < (1 << order); i++) {
		if(*((int*) gigaddr + i * PAGE_SIZE) != i) {
			printk("[KVMISO] Panic different value");
		}
	}

	printk("[KVMISO] Read and checked all values");
	dump_pte_addr(current->mm, gigaddr);
}

void kvmiso_kernel_split(struct kvm_vcpu *vcpu)
{
	printk("[KVMISO] Kernel Split");
}

void kvmiso_run(void)
{
	printk("[KVMISO] KVM Run");
	dump_pt_walk((unsigned long) current->mm);
}
