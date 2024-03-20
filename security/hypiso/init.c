#include "linux/sched/task.h"
#include <linux/topology.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#include "internal.h"

cpumask_var_t host_cpus;
cpumask_var_t guest_cpus;

int hypiso_on = 0;
int hypiso_nr_host_cpus = 1;
int hypiso_nr_guest_cpus = 1;

u64 hypiso_nr_vcpus = 0;
struct kvm_vcpu *hypiso_vcpus[MAX_NR_VCPUS];

/*
 * Set @target CPUs in @cpus using a small amount of cores, none of which have a
 * CPU in @taken.
 */
static int hypiso_set_cores(cpumask_var_t cpus, int target, cpumask_var_t taken)
{
	int cpu, sibling;
	struct cpumask forbidden;

	/*
	 * Mark all siblings of taken CPUs as forbidden.
	 */
	cpumask_clear(&forbidden);
	for_each_cpu(cpu, taken)
		cpumask_or(&forbidden, &forbidden, topology_sibling_cpumask(cpu));

	for_each_cpu(cpu, cpu_online_mask) {
		if (cpumask_test_cpu(cpu, &forbidden))
			continue;
		for_each_cpu(sibling, topology_sibling_cpumask(cpu)) {
			if (cpumask_weight(cpus) >= target)
				break;
			cpumask_set_cpu(sibling, cpus);
		}
	}

	if (cpumask_weight(cpus) != target)
		printk("HYPISO: there are only %d cpus available\n", cpumask_weight(cpus));

	return cpumask_weight(cpus);
}

static void hypiso_config_cores(void)
{
	cpumask_clear(host_cpus);
	cpumask_clear(guest_cpus);
	hypiso_nr_host_cpus = hypiso_set_cores(host_cpus, hypiso_nr_host_cpus, guest_cpus);
	hypiso_nr_guest_cpus = hypiso_set_cores(guest_cpus, hypiso_nr_guest_cpus, host_cpus);
}

void hypiso_init(void)
{
	zalloc_cpumask_var(&host_cpus, GFP_KERNEL);
	zalloc_cpumask_var(&guest_cpus, GFP_KERNEL);
	hypiso_config_cores();
	hypiso_init_sysfs();
	if (hypiso_on)
		hypiso_enable();
}

static struct task_struct *hypiso_spawn_runner(struct kvm_vcpu *vcpu)
{
	pid_t pid;
	struct task_struct *runner;
	char name[TASK_COMM_LEN];
	unsigned int old_flags = current->flags;
	//unsigned long clone_flags = SIGCHLD;

	current->flags |= PF_KTHREAD;
	//pid = kernel_thread(hypiso_runner, vcpu, clone_flags);

	struct kernel_clone_args args = {
		.flags = ((lower_32_bits(SIGCHLD) | CLONE_UNTRACED) & ~CSIGNAL),
		.exit_signal = (lower_32_bits(SIGCHLD) & CSIGNAL),
		.stack = (unsigned long) hypiso_runner,
		.stack_size = (unsigned long) vcpu,
	};

	pid = kernel_clone(&args);

	current->flags = old_flags;

	sched_setaffinity(pid, guest_cpus);
	runner = find_get_task_by_vpid(pid);

	snprintf(name, sizeof(name), "runner-%llu", hypiso_nr_vcpus);
	set_task_comm(runner, name);

	printk("HYPISO: %s/%d:%d@%px is creating vcpu %llu\n", current->comm,
		smp_processor_id(), current->pid, current, hypiso_nr_vcpus);

	printk("FLORIS: For vcpu %llu, owner PGD %px and runner PGD %px\n",
		hypiso_nr_vcpus, vcpu->owner->mm->pgd, runner->mm->pgd);

	return runner;
}

void hypiso_init_vcpu(struct kvm_vcpu *vcpu)
{
	hypiso_vcpus[hypiso_nr_vcpus] = vcpu;
	hypiso_nr_vcpus++;
	BUG_ON(hypiso_nr_vcpus > MAX_NR_VCPUS);

	get_task_struct(current);
	vcpu->owner = current;
	INIT_LIST_HEAD(&vcpu->node);
	vcpu->vmrunnable = false;
	vcpu->vmexit_pending = false;
	init_completion(&vcpu->runner_activated);

	vcpu->runner = hypiso_spawn_runner(vcpu);
}

void hypiso_set_nr_host_cpus(int new_nr_host_cpus)
{
	cpumask_clear(host_cpus);
	hypiso_nr_host_cpus = hypiso_set_cores(host_cpus, new_nr_host_cpus, guest_cpus);
}

void hypiso_set_nr_guest_cpus(int new_nr_guest_cpus)
{
	cpumask_clear(guest_cpus);
	hypiso_nr_guest_cpus = hypiso_set_cores(guest_cpus, new_nr_guest_cpus, host_cpus);
}
