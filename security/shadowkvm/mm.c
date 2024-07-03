
#include "asm/io.h"
#include "asm/pgalloc.h"
#include "asm/pgtable.h"
#include "asm/pgtable_types.h"
#include "linux/kvm.h"
#include "linux/memory_hotplug.h"
#include "linux/mm.h"
#include "linux/mm_types.h"
#include "linux/pgtable.h"
#include "linux/types.h"
#include <linux/kvm_host.h>
#include "linux/printk.h"

static phys_addr_t map_start = 0x0000000100000000;
static phys_addr_t map_size  = 0x0000000100000000;

static void dump_pagetable(unsigned long address)
{
	//pgd_t *base = __va(read_cr3_pa());
	//pgd_t *pgd = base + pgd_index(address);
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = pgd_offset(current->mm, address);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return;
	pr_info("pgd = %lx\n", pgd_val(*pgd));

	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d) || p4d_bad(*p4d))
		return;
	pr_info(" p4d = %lx\n", p4d_val(*p4d));

	pud = pud_offset(p4d, address);
	if (pud_none(*pud) || pud_bad(*pud))
		return;
	if (pud_trans_huge(*pud)) {
		pr_info("  pud = %lx (superpage)\n", pud_val(*pud));
		return;
	}
	pr_info("  pud = %lx\n", pud_val(*pud));

	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return;
	if (pmd_trans_huge(*pmd)) {
		pr_info("   pmd = %lx (hugepage)\n", pmd_val(*pmd));
		return;
	}
	pr_info("   pmd = %lx\n", pmd_val(*pmd));

	pte = pte_offset_kernel(pmd, address);
	if (pte_none(*pte))
		return;
	pr_info("    pte = %lx\n", pte_val(*pte));
	return;
}

void shadowkvm_kernel_split(struct kvm_vcpu *vcpu)
{
	printk("Kernel split\n");
	//uint64_t *start = phys_to_virt(map_start);

	//*start = 0x12345678ABCDEF;
	//printk("Try read: %ld\n", *start);
}

void shadowkvm_memory_map(unsigned long paddr_start,
			  unsigned long paddr_end,
			  unsigned long vaddr_start,
			  pgprot_t prot, struct mm_struct *mm)
{
	unsigned long paddr = paddr_start;
	unsigned long vaddr = vaddr_start;

	//spin_lock(&mm->page_table_lock);

	while(paddr < paddr_end) {
		pgd_t *pgd = pgd_offset(mm, vaddr);
		//printk("[SHADOWKVM] Point PGD %lx\n", pgd_val(*pgd));
		p4d_t *p4d = p4d_alloc(mm, pgd, vaddr);
		//printk("[SHADOWKVM] Point P4D %lx\n", p4d_val(*p4d));
		pud_t *pud = pud_alloc(mm, p4d, vaddr);
		//printk("[SHADOWKVM] Point PUD %lx\n", pud_val(*pud));
		pmd_t *pmd = pmd_alloc(mm, pud, vaddr);
		//printk("[SHADOWKVM] Point PMD %lx\n", pmd_val(*pmd));
		pte_t *pte = pte_alloc_map(mm, pmd, vaddr);
		//printk("[SHADOWKVM] Point PTE %lx\n", pte_val(*pte));

		//printk("[SHADOWKVM] Point P4D %lx; PUD %lx; PMD %lx; PTE %lx\n", p4d, pud, pmd, pte);
		//printk("[SHADOWKVM] Found P4D %lx; PUD %lx; PMD %lx; PTE %lx\n", p4d_val(*p4d), pud_val(*pud), pmd_val(*pmd), pte_val(*pte));

		pte_t val = mk_pte(pfn_to_page(__phys_to_pfn(paddr)), prot);
		val = pte_mkwrite(pte_mkdirty(pte_sw_mkyoung(val)));

		set_pte_at(mm, vaddr, pte, val);

		//printk("[SHADOWKVM] Wrote PTE %lx for VA %lx and PA %lx\n", val, vaddr, paddr);

		paddr += PAGE_SIZE;
		vaddr += PAGE_SIZE;
	}

	//spin_unlock(&mm->page_table_lock);
}

void shadowkvm_vm_init(struct kvm *kvm)
{
	// Map extra memory in process direct map :X
	// Add it to VM :X:X

	printk("VM Creation\n");

	unsigned long page_count = 0x1;
	shadowkvm_memory_map(map_start, map_start + map_size,
				phys_to_virt(map_start), PAGE_KERNEL, kvm->mm);

	shadowkvm_memory_map(map_start, map_start + 0x50000000,
				0x50000000, PAGE_SHARED_EXEC, kvm->mm);

	printk("[SHADOWKVM] Mapped %lx pages\n", page_count);

	dump_pagetable((unsigned long) phys_to_virt(map_start));

	printk("[SHADOWKVM] Dumped; kvm->mm: %lx; current->mm: %lx\n", kvm->mm, current->mm);

	uint64_t *start = phys_to_virt(map_start + 0x10000000);
	*start = 0x12345678ABCDEF;

	printk("Try read: %lx\n", *start);

	uint64_t *second = 0x50000000;
	*second = 0xABCDEF123456789;

	_printk("Try read second: %lx\n", *second);

	printk("WEOE?\n");
}

void shadowkvm_vcpu_init(struct kvm *kvm)
{
	/*struct kvm_userspace_memory_region mem0 = {
		.slot = 100 | (0 << 16),
		.flags = 0,
		.guest_phys_addr = 0x50000000,
		.memory_size = 0x50000000,
		.userspace_addr = 0x50000000
	};
	int ret0 = kvm_set_memory_region(kvm, &mem0);

	struct kvm_userspace_memory_region mem1 = {
		.slot = 100 | (1 << 16),
		.flags = 0,
		.guest_phys_addr = 0x50000000,
		.memory_size = 0x50000000,
		.userspace_addr = 0x50000000
	};
	int ret1 = kvm_set_memory_region(kvm, &mem1);

	printk("[SHADOWKVM] Set Region result %x - %x\n", ret0, ret1);*/

	u32 reserved = 0x100000;
	struct kvm_memslots *slots = __kvm_memslots(kvm, 0);
	struct kvm_memory_slot *tmp;

	kvm_for_each_memslot(tmp, slots) {
		if(gfn_to_gpa(tmp->base_gfn) != reserved) continue;
		tmp->userspace_addr = 0x50000000;
		printk("[SHADOWKVM] Memory adjusted for ASID %d\n", tmp->as_id);
	}

	slots = __kvm_memslots(kvm, 1);

	kvm_for_each_memslot(tmp, slots) {
		if(gfn_to_gpa(tmp->base_gfn) != reserved) continue;
		tmp->userspace_addr = 0x50000000;
		printk("[SHADOWKVM] Memory adjusted for ASID %d\n", tmp->as_id);
	}
}

void shadowkvm_memory_intercept(struct kvm *kvm, struct kvm_userspace_memory_region *mem)
{
	static u32 reserved_slot = 4;

	if(mem->memory_size < 0x100000)
		return;

	if(reserved_slot == -1)
		reserved_slot = mem->slot & 0xFFFF;

	if((mem->slot & 0xFFFF) != reserved_slot)
		return;

	mem->userspace_addr = 0x50000000;

	printk("[SHADOWKVM] Memory allocation intercepted for ASID %d\n", mem->slot);
}
