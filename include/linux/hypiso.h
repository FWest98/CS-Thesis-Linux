#ifndef __HYPISO_H__
#define __HYPISO_H__

struct kvm_vcpu;
void hypiso_vcpu_run(struct kvm_vcpu *vcpu);
void hypiso_switch_fpu_return(struct task_struct *task);

#ifdef CONFIG_HYPISO
void hypiso_host_vmrun(struct kvm_vcpu *vcpu);
void hypiso_init(void);
void hypiso_init_vcpu(struct kvm_vcpu *vcpu);
#else /* CONFIG_HYPISO */
static inline void hypiso_host_vmrun(struct kvm_vcpu *vcpu) { hypiso_vcpu_run(vcpu); }
static inline void hypiso_init(void) { }
static inline void hypiso_init_vcpu(struct kvm_vcpu *vcpu) { }
#endif /* CONFIG_HYPISO */

#endif /* __HYPISO_H__ */
