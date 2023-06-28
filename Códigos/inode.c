
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/sched.h>

#include <asm/current.h>
#include <asm/uaccess.h>

#include "islenefs.h"


/* O islenefs eh um fs muito simples. Os inodes sao atribuidos em ordem crescente, sem
 * reaproveitamento */
extern int inode_number;

struct inode * islenefs_get_inode(struct super_block *sb, int mode)
{
/* G: deveria estar em inode.c 
 * -> Here I am! */

    struct inode * inode = new_inode(sb);

    if (inode) {
        inode->i_mode = mode;
        inode->i_uid = current->cred->fsuid;
        inode->i_gid = current->cred->fsgid;
        inode->i_blocks = 0;
        inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
        inode->i_ino = ++inode_number;
      /* G: Voce esta usando o inode number que foi retornado nessa funcao. Apesar de nao ter nada de errado nisso,
            isso ignora completamente o inode_number global do islenefs. Entao voce poderia te-lo apagado. 
           -> na verdade eu esqueci de acrescentar a linha acima, agora tem sentido a variÃ¡vel global */
        switch (mode & S_IFMT) {
            case S_IFREG:
                inode->i_op = &islenefs_file_inode_operations;
                inode->i_fop = &islenefs_file_operations;
                break;
            case S_IFDIR:
                inode->i_op = &islenefs_dir_inode_operations;
                inode->i_fop = &simple_dir_operations;
                /* directory inodes start off with i_nlink == 2 (for "." entry) */
                inc_nlink(inode);
                break;
        }
    }
    return inode;
}


static int islenefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode * inode;
    struct dentry * root;

    sb->s_maxbytes = 4096;
    /* Cafe eh uma bebida saudavel, mas nao exagere. */
    sb->s_magic = 0xBEBACAFE;
    sb->s_blocksize = 1024;
    sb->s_blocksize_bits = 10;

    sb->s_op = &islenefs_ops;
    sb->s_time_gran = 1;

    inode = new_inode(sb);

    if (!inode)
        return -ENOMEM;

    inode->i_ino = ++inode_number;
    inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
    inode->i_blocks = 0;
    inode->i_uid = inode->i_gid = 0;
    inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;
    inode->i_op = &islenefs_dir_inode_operations;
    inode->i_fop = &simple_dir_operations;
    set_nlink(inode, 2);

    root = d_make_root(inode);
    if (!root) {
        iput(inode);
        return -ENOMEM;
    }
    sb->s_root = root;
    return 0;

}


static struct dentry *islenefs_get_sb(struct file_system_type *fs_type, int flags, const char *dev_name,
		   void *data)
{
  return mount_bdev(fs_type, flags, dev_name, data, islenefs_fill_super);
}

static struct file_system_type islenefs_fs_type = {
    .owner		= THIS_MODULE,
    .name		= "islenefs",
    .mount		= islenefs_get_sb,
    .kill_sb	= kill_litter_super,

};

static int __init init_islenefs_fs(void)
{
    int err;
    inode_number = 0;
    err=register_filesystem(&islenefs_fs_type);
    if (!err)
        goto out;
    printk(KERN_INFO "islenefs: support added\n");
out:
    return err;
}

static void __exit exit_islenefs_fs(void)
{
    unregister_filesystem(&islenefs_fs_type);
}

module_init(init_islenefs_fs)
module_exit(exit_islenefs_fs)

MODULE_LICENSE("GPL");
