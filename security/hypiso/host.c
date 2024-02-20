#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/stddef.h>
#include <linux/hypiso.h>
#include "internal.h"

static void hypiso_sleep_until_vmexit(struct kvm_vcpu *vcpu)
{
	for (;;) {
		if (smp_load_acquire(&vcpu->vmexit_pending))
			break;
		schedule();
	}
}

void hypiso_host_vmrun(struct kvm_vcpu *vcpu)
{
	if (!hypiso_on) {
		hypiso_vcpu_run(vcpu);
		return;
	}

	vcpu_put(vcpu);
	vcpu->vmexit_pending = false;
	smp_wmb();
	smp_store_release(&vcpu->vmrunnable, true);
	hypiso_sleep_until_vmexit(vcpu);
	vcpu_load(vcpu);
}
