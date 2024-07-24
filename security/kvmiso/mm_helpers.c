#include "mm_helpers.h"
#include "asm/pgalloc.h"
#include "asm/pgtable.h"
#include "asm/pgtable_64_types.h"
#include "asm/pgtable_types.h"
#include "linux/hugetlb.h"
#include "linux/types.h"
#include <linux/mm.h>
#include <linux/mm_types.h>

static inline void pr_ptentry(unsigned long entry, unsigned long val, unsigned long vaddr, unsigned long flags)
{
	printk(KERN_CONT
		"*0x%lx -> %09lx: 0x%lx | %c %c %c %c %c\n",
		entry, val,
		(flags & _PAGE_PSE) ? __pa(vaddr) : vaddr,
		(flags & _PAGE_NX) ? '-' : 'X',
		(flags & _PAGE_PSE) ? 'H' : '-',
		(flags & _PAGE_USER) ? 'U' : 'S',
		(flags & _PAGE_RW) ? 'W' : 'R',
		(flags & _PAGE_PRESENT) ? 'P' : '-'
	);
}

static inline void pr_pgd(pgd_t *pgd)
{
	pr_ptentry(
		(unsigned long) pgd,
		pgd_val(*pgd),
		pgd_page_vaddr(*pgd),
		pgd_flags(*pgd)
	);
}

static inline void pr_p4d(p4d_t *p4d)
{
	pr_ptentry(
		(unsigned long) p4d,
		p4d_val(*p4d),
		(unsigned long) p4d_pgtable(*p4d),
		p4d_flags(*p4d)
	);
}

static inline void pr_pud(pud_t *pud)
{
	pr_ptentry(
		(unsigned long) pud,
		pud_val(*pud),
		(unsigned long) pud_pgtable(*pud),
		pud_flags(*pud)
	);
}

static inline void pr_pmd(pmd_t *pmd)
{
	pr_ptentry(
		(unsigned long) pmd,
		pmd_val(*pmd),
		pmd_page_vaddr(*pmd),
		pmd_flags(*pmd)
	);
}

static inline void pr_pte(pte_t *pte)
{
	pr_ptentry(
		(unsigned long) pte,
		pte_val(*pte),
		(unsigned long) (pte_val(*pte) & PTE_PFN_MASK),
		pte_flags(*pte)
	);
}

void dup_pgd_range(struct mm_struct *mm, unsigned long start, unsigned long end)
{
	p4d_t *src, *new;
	pgd_t *pgd;
	unsigned long cur;

	if (!pgtable_l5_enabled())
		return;

	// Align start and end pointers to pgd entries
	start &= PGDIR_MASK;
	end &= PGDIR_MASK; // end pointer is inclusive

	for(cur = start; cur < end; cur += PGDIR_SIZE) {
		pgd = pgd_offset(mm, cur);

		if(pgd_none(*pgd) || pgd_bad(*pgd))
			continue;

		src = (p4d_t*) pgd_page_vaddr(*pgd);
		new = p4d_alloc_one(mm, cur);
		memcpy(new, src, sizeof(p4d_t) * PTRS_PER_P4D);

		pgd_populate(mm, pgd, new);

		pr_info(
			"[DUP] Duplicated PGD[%i], from 0x%lx to 0x%lx\n",
			pgd_index(cur),
			src, new
		);
	}
}

void dup_p4d_range(struct mm_struct *mm, unsigned long start, unsigned long end)
{
	pud_t *src, *new;
	p4d_t *p4d;
	unsigned long cur;

	// Align start and end pointers to p4d entries
	start &= P4D_MASK;
	end &= P4D_MASK; // end pointer is inclusive

	for(cur = start; cur < end; cur += P4D_SIZE) {
		p4d = p4d_offset(pgd_offset(mm, cur), cur);

		if(p4d_none(*p4d) || p4d_bad(*p4d))
			continue;

		src = (pud_t*) p4d_pgtable(*p4d);
		new = pud_alloc_one(mm, cur);
		memcpy(new, src, sizeof(pud_t) * PTRS_PER_PUD);

		p4d_populate(mm, p4d, new);

		if (pgtable_l5_enabled()) {
			pr_info(
				"[DUP] Duplicated P4D[%i], from 0x%lx to 0x%lx\n",
				p4d_index(cur),
				src, new
			);
		} else {
			pr_info(
				"[DUP] Duplicated PGD=P4D[%i], from 0x%lx to 0x%lx\n",
				pgd_index(cur),
				src, new
			);
		}
	}
}

void dup_pud_range(struct mm_struct *mm, unsigned long start, unsigned long end)
{
	pmd_t *src, *new;
	pud_t *pud;
	unsigned long cur;

	// Align start and end pointers to pud entries
	start &= PUD_MASK;
	end &= PUD_MASK; // end pointer is inclusive

	for(cur = start; cur < end; cur += PUD_SIZE) {
		pud = pud_offset(p4d_offset(pgd_offset(mm, cur), cur), cur);

		if(pud_none(*pud) || pud_bad(*pud))
			continue;

		src = pud_pgtable(*pud);
		new = pmd_alloc_one(mm, cur);
		memcpy(new, src, sizeof(pmd_t) * PTRS_PER_PMD);

		pud_populate(mm, pud, new);

		pr_info(
			"[DUP] Duplicated PUD[%i], from 0x%lx to 0x%lx\n",
			pud_index(cur),
			src, new
		);
	}
}

void dup_pmd_range(struct mm_struct *mm, unsigned long start, unsigned long end)
{
	pte_t *src, *new;
	pmd_t *pmd;
	unsigned long cur;

	// Align start and end pointers to pmd entries
	start &= PMD_MASK;
	end &= PMD_MASK; // end pointer is inclusive

	for(cur = start; cur < end; cur += PMD_SIZE) {
		pmd = pmd_offset(pud_offset(p4d_offset(pgd_offset(mm, cur), cur), cur), cur);

		if(pmd_none(*pmd) || pmd_bad(*pmd))
			continue;

		src = (pte_t*) pmd_page_vaddr(*pmd);
		new = (pte_t*) page_address(pte_alloc_one(mm));
		memcpy(new, src, sizeof(pte_t) * PTRS_PER_PTE);

		pmd_populate_kernel(mm, pmd, new);

		pr_info(
			"[DUP] Duplicated PMD[%i], from 0x%lx to 0x%lx\n",
			pmd_index(cur),
			src, new
		);
	}
}

void dump_pt_walk_mm(unsigned long address, struct mm_struct *mm)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	int idx;

	pr_info("[WALK] Finding address 0x%lx in MM at 0x%lx\n", address, mm);

	idx = pgd_index(address);
	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd)) return;
	pr_info("[WALK] PGD[%03i]         = ", idx);
	pr_pgd(pgd);
	if (pgd_bad(*pgd)) return;

	idx = p4d_index(address);
	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d)) return;
	pr_info("[WALK]   P4D[%03i]       = ", idx);
	pr_p4d(p4d);
	if (p4d_bad(*p4d)) return;

	idx = pud_index(address);
	pud = pud_offset(p4d, address);
	if (pud_none(*pud)) return;
	pr_info("[WALK]     PUD[%03i]     = ", idx);
	pr_pud(pud);
	if (pud_bad(*pud)) return;

	idx = pmd_index(address);
	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd)) return;
	pr_info("[WALK]       PMD[%03i]   = ", idx);
	pr_pmd(pmd);
	if (pmd_bad(*pmd)) return;

	idx = pte_index(address);
	pte = pte_offset_kernel(pmd, address);
	if (pte_none(*pte))
		return;
	pr_info("[WALK]         PTE[%03i] = ", idx);
	pr_pte(pte);

	return;
}

void dump_pt_walk(unsigned long address)
{
	dump_pt_walk_mm(address, current->mm);
}

void dump_pgd_addr(struct mm_struct *mm, unsigned long addr)
{
	dump_pgd_rel(mm->pgd, 0);
}

inline void dump_pgd(pgd_t *pgd) { dump_pgd_rel(pgd, 0); }
void dump_pgd_rel(pgd_t *pgd, unsigned long base) {
	uint64_t i, previous = -1;
	pgdval_t val;
	uint64_t start, end;

	// PGD corresponds to P4D
	for(i = 0; i < PTRS_PER_PGD; i++, pgd++) {
		val = pgd_val(*pgd);
		if(val == 0) continue;

		if(i > previous + 1) {
			pr_info(
				"[DUMP] PGD[%03d - %03d]: unmapped\n",
				previous + 1, i - 1
			);
		}
		previous = i;

		start = (i << PGDIR_SHIFT) + base;
		end = ((i + 1) << PGDIR_SHIFT) - 1 + base;

		if(i >= KERNEL_PGD_BOUNDARY) {
			start |= 0xffff000000000000ull;
			end   |= 0xffff000000000000ull;
		}

		pr_info("[DUMP] PGD[%03d] maps [0x%lx]: ", i, start);
		pr_pgd(pgd);
	}

	if(i > previous + 1) {
		pr_info(
			"[DUMP] PGD[%03d - %03d]: unmapped\n",
			previous + 1, i - 1
		);
	}
}

void dump_p4d_addr(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd = pgd_offset(mm, addr);
	dump_p4d_rel((p4d_t *) pgd_page_vaddr(*pgd), addr & PGDIR_MASK);
}

inline void dump_p4d(p4d_t *p4d) { dump_p4d_rel(p4d, 0); }
void dump_p4d_rel(p4d_t *p4d, unsigned long base) {
	uint64_t i, previous = -1;
	p4dval_t val;
	uint64_t start, end;

	// PGD corresponds to P4D
	for(i = 0; i < PTRS_PER_P4D; i++, p4d++) {
		val = p4d_val(*p4d);
		if(val == 0) continue;

		if(i > previous + 1) {
			pr_info(
				"[DUMP] P4D[%03d - %03d]: unmapped\n",
				previous + 1, i - 1
			);
		}
		previous = i;

		start = (i << P4D_SHIFT) + base;
		end = ((i + 1) << P4D_SHIFT) - 1 + base;

		if(i >= KERNEL_PGD_BOUNDARY) {
			start |= 0xffff000000000000ull;
			end   |= 0xffff000000000000ull;
		}

		pr_info("[DUMP] P4D[%03d] maps [0x%lx]: ", i, start);
		pr_p4d(p4d);
	}

	if(i > previous + 1) {
		pr_info(
			"[DUMP] P4D[%03d - %03d]: unmapped\n",
			previous + 1, i - 1
		);
	}
}

void dump_pud_addr(struct mm_struct *mm, unsigned long addr)
{
	p4d_t *p4d = p4d_offset(pgd_offset(mm, addr), addr);
	dump_pud_rel(p4d_pgtable(*p4d), addr & P4D_MASK);
}

inline void dump_pud(pud_t *pud) { dump_pud_rel(pud, 0); }
void dump_pud_rel(pud_t *pud, unsigned long base) {
	uint64_t i, previous = -1;
	pudval_t val;
	uint64_t start, end;

	for(i = 0; i < PTRS_PER_PUD; i++, pud++) {
		val = pud_val(*pud);
		if(val == 0) continue;

		if(i > previous + 1) {
			pr_info(
				"[DUMP] PUD[%03d - %03d]: unmapped\n",
				previous + 1, i - 1
			);
		}
		previous = i;

		start = (i << PUD_SHIFT) + base;
		end = ((i + 1) << PUD_SHIFT) - 1 + base;

		if(i >= KERNEL_PGD_BOUNDARY) {
			start |= 0xffff000000000000ull;
			end   |= 0xffff000000000000ull;
		}

		pr_info("[DUMP] PUD[%03d] maps [0x%lx]: ", i, start);
		pr_pud(pud);
	}

	if(i > previous + 1) {
		pr_info(
			"[DUMP] PUD[%03d - %03d]: unmapped\n",
			previous + 1, i - 1
		);
	}
}

void dump_pmd_addr(struct mm_struct *mm, unsigned long addr)
{
	pud_t *pud = pud_offset(p4d_offset(pgd_offset(mm, addr), addr), addr);
	dump_pmd_rel(pud_pgtable(*pud), addr & PUD_MASK);
}

inline void dump_pmd(pmd_t *pmd) { dump_pmd_rel(pmd, 0); }
void dump_pmd_rel(pmd_t *pmd, unsigned long base) {
	uint64_t i, previous = -1;
	pmdval_t val;
	uint64_t start, end;

	for(i = 0; i < PTRS_PER_PMD; i++, pmd++) {
		val = pmd_val(*pmd);
		if(val == 0) continue;

		if(i > previous + 1) {
			pr_info(
				"[DUMP] PMD[%03d - %03d]: unmapped\n",
				previous + 1, i - 1
			);
		}
		previous = i;

		start = (i << PMD_SHIFT) + base;
		end = ((i + 1) << PMD_SHIFT) - 1 + base;

		if(i >= KERNEL_PGD_BOUNDARY) {
			start |= 0xffff000000000000ull;
			end   |= 0xffff000000000000ull;
		}

		pr_info("[DUMP] PMD[%03d] maps [0x%lx]: ", i, start);
		pr_pmd(pmd);
	}
	if(i > previous + 1) {
		pr_info(
			"[DUMP] PMD[%03d - %03d]: unmapped\n",
			previous + 1, i - 1
		);
	}
}

void dump_pte_addr(struct mm_struct *mm, unsigned long addr)
{
	pmd_t *pmd = pmd_offset(pud_offset(p4d_offset(pgd_offset(mm, addr), addr), addr), addr);
	dump_pte_rel((pte_t *) pmd_page_vaddr(*pmd), addr & PMD_MASK);
}

inline void dump_pte(pte_t *pte) { dump_pte_rel(pte, 0); }
void dump_pte_rel(pte_t *pte, unsigned long base) {
	uint64_t i, previous = -1;
	pteval_t val;
	uint64_t start, end;

	// PTE corresponds to P4D
	for(i = 0; i < PTRS_PER_PTE; i++, pte++) {
		val = pte_val(*pte);
		if(val == 0) continue;

		if(i > previous + 1) {
			pr_info(
				"[DUMP] PTE[%03d - %03d]: unmapped\n",
				previous + 1, i - 1
			);
		}
		previous = i;

		start = (i << PAGE_SHIFT) + base;
		end = ((i + 1) << PAGE_SHIFT) - 1 + base;

		if(i >= KERNEL_PGD_BOUNDARY) {
			start |= 0xffff000000000000ull;
			end   |= 0xffff000000000000ull;
		}

		pr_info("[DUMP] PTE[%03d] maps [0x%lx]: ", i, start);
		pr_pte(pte);
	}
	if(i > previous + 1) {
		pr_info(
			"[DUMP] PTE[%03d - %03d]: unmapped\n",
			previous + 1, i - 1
		);
	}
}

uint64_t pgd_contiguous_end(struct mm_struct *mm, uint64_t start)
{
	pgd_t *pgd;
	uint64_t i, end;

	i = pgd_index(start);

	for(pgd = (mm->pgd + i); i < PTRS_PER_PGD; i++, pgd++) {
		if(pgd_val(*pgd) == 0) {
			break;
		}
	}

	end = (i << PGDIR_SHIFT);

	if(i > KERNEL_PGD_BOUNDARY) { // not equals; since end is exclusive
		end |= 0xffff000000000000ull;
	}

	return end;
}
