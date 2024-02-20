#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/stddef.h>
#include <linux/hypiso.h>
#include <asm/mmu_context.h>
#include <asm-generic/mmu_context.h>
#include "internal.h"

static void hypiso_guest_vmrun(struct kvm_vcpu *vcpu)
{
	vcpu_load(vcpu);
	hypiso_vcpu_run(vcpu);
	vcpu_put(vcpu);

	vcpu->vmrunnable = false;
	smp_wmb();
	smp_store_release(&vcpu->vmexit_pending, true);
}

/*
 * Kernel thread running on guest cores responsible for running a specific vCPU.
 */
int hypiso_runner(void *data)
{
	struct kvm_vcpu *vcpu = data;

	printk("HYPISO: spawned %s/%d:%d@%px\n", current->comm,
		smp_processor_id(), current->pid, current);

	for (;;) {
		if (!hypiso_on)
			wait_for_completion(&vcpu->runner_activated);
		if (smp_load_acquire(&vcpu->vmrunnable))
			hypiso_guest_vmrun(vcpu);
		schedule();
	}

	return 0;
}
