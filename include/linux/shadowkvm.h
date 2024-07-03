#ifndef __SHADOWKVM_H__
#define __SHADOWKVM_H__

#include <linux/kvm_host.h>

#ifdef CONFIG_SHADOWKVM
void shadowkvm_e820_block(void);
void shadowkvm_kernel_split(struct kvm_vcpu *);
void shadowkvm_vm_init(struct kvm *);
void shadowkvm_vcpu_init(struct kvm *kvm);
void shadowkvm_memory_intercept(struct kvm *kvm, struct kvm_userspace_memory_region *mem);
#endif

#endif
