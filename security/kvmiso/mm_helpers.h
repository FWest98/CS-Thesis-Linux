#ifndef __KVMISO_HELPERS_H__
#define __KVMISO_HELPERS_H__

#include "asm/pgtable.h"
#include "linux/mm_types.h"
#include "linux/types.h"
#include <linux/mm.h>

void dup_pgd_range(struct mm_struct *, unsigned long start, unsigned long end);
void dup_p4d_range(struct mm_struct *, unsigned long start, unsigned long end);
void dup_pud_range(struct mm_struct *, unsigned long start, unsigned long end);
void dup_pmd_range(struct mm_struct *, unsigned long start, unsigned long end);

void dump_pt_walk_mm(unsigned long address, struct mm_struct *mm);
void dump_pt_walk(unsigned long address);

void dump_pgd(pgd_t *);
void dump_pgd_rel(pgd_t *, unsigned long base);
void dump_pgd_addr(struct mm_struct *, unsigned long addr);
void dump_p4d(p4d_t *);
void dump_p4d_rel(p4d_t *, unsigned long base);
void dump_p4d_addr(struct mm_struct *, unsigned long addr);
void dump_pud(pud_t *);
void dump_pud_rel(pud_t *, unsigned long base);
void dump_pud_addr(struct mm_struct *, unsigned long addr);
void dump_pmd(pmd_t *);
void dump_pmd_rel(pmd_t *, unsigned long base);
void dump_pmd_addr(struct mm_struct *, unsigned long addr);
void dump_pte(pte_t *);
void dump_pte_rel(pte_t *, unsigned long base);
void dump_pte_addr(struct mm_struct *, unsigned long addr);

uint64_t pgd_contiguous_end(struct mm_struct *, uint64_t);

#endif
