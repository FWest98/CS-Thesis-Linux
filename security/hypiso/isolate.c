#include "internal.h"

static void hypiso_isolate_processes(const struct cpumask *cpus)
{
	struct task_struct *task;

	for_each_process(task) {
		if (!(task->flags & PF_KTHREAD))
			sched_setaffinity(task->pid, cpus);
	}
}

static void hypiso_reroute_irqs(const struct cpumask *cpus)
{
	int irq;
	for_each_active_irq(irq)
		irq_set_affinity(irq, cpus);
}

static void hypiso_start_runners(void)
{
	int i;
	struct kvm_vcpu *vcpu;

	for (i = 0; i < hypiso_nr_vcpus; i++) {
		vcpu = hypiso_vcpus[i];
		sched_setaffinity(vcpu->runner->pid, guest_cpus);
		complete(&vcpu->runner_activated);
	}
}

static void hypiso_stop_runners(void)
{
	int i;
	for (i = 0; i < hypiso_nr_vcpus; i++)
		reinit_completion(&hypiso_vcpus[i]->runner_activated);
}

void hypiso_enable(void)
{
	hypiso_isolate_processes(host_cpus);
	hypiso_reroute_irqs(host_cpus);
	hypiso_start_runners();
	hypiso_on = 1;
}

void hypiso_disable(void)
{
	hypiso_on = 0;
	hypiso_stop_runners();
	hypiso_reroute_irqs(cpu_online_mask);
	hypiso_isolate_processes(cpu_online_mask);
}
