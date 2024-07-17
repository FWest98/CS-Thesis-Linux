#ifndef __KVMISO_H__
#define __KVMISO_H__

#include <linux/kvm_host.h>

#ifdef CONFIG_KVMISO
void kvmiso_vm_init(struct kvm *);
void kvmiso_run(void);
void kvmiso_kernel_split(struct kvm_vcpu *);
#endif

#endif
