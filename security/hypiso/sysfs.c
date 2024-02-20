#include <linux/kobject.h>
#include <linux/string.h>
#include "internal.h"

static ssize_t hypiso_sysfs_hypiso_on_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	if (*buf == '0')
		hypiso_disable();
	else if (*buf == '1')
		hypiso_enable();

	return count;
}

static ssize_t hypiso_sysfs_hypiso_on_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hypiso_on);
}

static ssize_t hypiso_sysfs_nr_host_cpus_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	long nr;

	if (kstrtol(buf, 0, &nr)) {
		printk("HYPISO: parsing of '%s' as a number failed\n", buf);
		return count;
	}

	if (nr < 1) {
		printk("HYPISO: need at least 1 host cpu\n");
		return count;
	}

	hypiso_set_nr_host_cpus(nr);

	return count;
}

static ssize_t hypiso_sysfs_nr_host_cpus_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hypiso_nr_host_cpus);
}

static ssize_t hypiso_sysfs_nr_guest_cpus_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	long nr;

	if (kstrtol(buf, 0, &nr)) {
		printk("HYPISO: parsing of '%s' as a number failed\n", buf);
		return count;
	}

	if (nr < 1) {
		printk("HYPISO: need at least 1 guest cpu\n");
		return count;
	}

	hypiso_set_nr_guest_cpus(nr);

	return count;
}

static ssize_t hypiso_sysfs_nr_guest_cpus_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hypiso_nr_guest_cpus);
}

static ssize_t hypiso_sysfs_core_config_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "host_cpus  %*pbl\nguest_cpus %*pbl\n",
		cpumask_pr_args(host_cpus), cpumask_pr_args(guest_cpus));
}

static struct kobj_attribute hypiso_sysfs_hypiso_on = {
	.attr = {
		.name = "hypiso_on",
		.mode = S_IWUSR | S_IRUSR,
	},
	.store = hypiso_sysfs_hypiso_on_store,
	.show = hypiso_sysfs_hypiso_on_show,
};

static struct kobj_attribute hypiso_sysfs_nr_host_cpus = {
	.attr = {
		.name = "nr_host_cpus",
		.mode = S_IWUSR | S_IRUSR,
	},
	.store = hypiso_sysfs_nr_host_cpus_store,
	.show = hypiso_sysfs_nr_host_cpus_show,
};

static struct kobj_attribute hypiso_sysfs_nr_guest_cpus = {
	.attr = {
		.name = "nr_guest_cpus",
		.mode = S_IWUSR | S_IRUSR,
	},
	.store = hypiso_sysfs_nr_guest_cpus_store,
	.show = hypiso_sysfs_nr_guest_cpus_show,
};

static struct kobj_attribute hypiso_sysfs_core_config = {
	.attr = {
		.name = "core_config",
		.mode = S_IRUSR,
	},
	.show = hypiso_sysfs_core_config_show,
};

static struct attribute *hysiso_attrs[] = {
	&hypiso_sysfs_hypiso_on.attr,
	&hypiso_sysfs_nr_host_cpus.attr,
	&hypiso_sysfs_nr_guest_cpus.attr,
	&hypiso_sysfs_core_config.attr,
	NULL,
};

static struct attribute_group hysiso_attr_group = {
	.attrs = hysiso_attrs,
};

void hypiso_init_sysfs(void)
{
	int ret;
	struct kobject *hysiso_kobj = kobject_create_and_add("hypiso", kernel_kobj);

	ret = sysfs_create_group(hysiso_kobj, &hysiso_attr_group);
	if (ret)
		kobject_put(hysiso_kobj);
}
