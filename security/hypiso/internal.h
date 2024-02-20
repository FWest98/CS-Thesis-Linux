#include <linux/topology.h>
#include <linux/slab.h>
#include <linux/kvm_host.h>

#ifndef __HYPISO_INTERNAL_H__
#define __HYPISO_INTERNAL_H__

#define MAX_NR_VCPUS 128
extern cpumask_var_t host_cpus;
extern cpumask_var_t guest_cpus;
extern int hypiso_on;
extern int hypiso_nr_host_cpus;
extern int hypiso_nr_guest_cpus;
extern u64 hypiso_nr_vcpus;
extern struct kvm_vcpu *hypiso_vcpus[MAX_NR_VCPUS];
void hypiso_set_nr_host_cpus(int new_nr_host_cpus);
void hypiso_set_nr_guest_cpus(int new_nr_guest_cpus);

void hypiso_enable(void);
void hypiso_disable(void);

void hypiso_host_cpu_init(int cpu);

void hypiso_guest_cpu_init(int cpu);
int hypiso_runner(void *data);

void hypiso_init_sysfs(void);

#endif /* __HYPISO_INTERNAL_H__ */
