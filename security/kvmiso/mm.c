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

	dump_pud_addr(current->mm, (unsigned long) kvm->mm);
	dump_pt_walk((unsigned long) kvm->mm);
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
