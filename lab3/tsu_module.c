#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define procfs_name "tsulab"
static struct proc_dir_entry *our_proc_file = NULL;

static ssize_t profile_read(struct file *file_pointer, char __user *buffer,
                            size_t buffer_length, loff_t *offset) {
    char s[] = "Tomsk State University\n";
    size_t len = sizeof(s) - 1;
    
    if (*offset >= len)
        return 0;
    
    size_t to_copy = len - *offset;
    if (buffer_length < to_copy)
        to_copy = buffer_length;
    
    if (copy_to_user(buffer, s + *offset, to_copy))
        return -EFAULT;
    
    *offset += to_copy;
    return to_copy;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = profile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = profile_read,
};
#endif

static int __init procfs1_init(void) {
    our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);
    
    if (our_proc_file == NULL) {
        pr_err("Error: Could not initialize /proc/%s\n", procfs_name);
        return -ENOMEM;
    }
    
    pr_info("Welcome to the Tomsk State University\n");
    pr_info("/proc/%s created\n", procfs_name);
    return 0;
}

static void __exit procfs1_exit(void) {
    proc_remove(our_proc_file);
    pr_info("/proc/%s removed\n", procfs_name);
    pr_info("Tomsk State University forever!\n");
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL");
