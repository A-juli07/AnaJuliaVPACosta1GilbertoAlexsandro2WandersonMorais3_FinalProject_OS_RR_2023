#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/vfs.h>
#include <linux/sched.h>

#include <asm/current.h>

#include "islenefs.h"


extern int inode_number;

/* Lembre-se que nao temos um disco! (Isso so complicaria as coisas, pois teriamos
 * que lidar com o sub-sistema de I/O. Entao teremos uma representacao bastante
 * simples da estrutura de arquivos: Uma lista duplamente ligada circular (para
 * aplicacoes reais, um hash seria muito mais adequado) contem em cada elemento
 * um indice (inode) e uma pagina para conteudo (ou seja: o tamanho maximo de um
 * arquivo nessa versao do islene fs eh de 4Kb. Nao ha subdiretorios */

const struct super_operations islenefs_ops = {
    .statfs		= simple_statfs,
    .drop_inode	= generic_delete_inode,
};


/* criacao de um arquivo: sem misterio, sem segredo, apenas
 * alocar as estruturas, preencher, e retornar */
static int islenefs_create (struct inode *dir, struct dentry * dentry,
        int mode, struct nameidata *nd)
{
    struct inode *inode;
    int err= -ENOSPC;
    
    mode |= S_IFREG;
    
    inode = islenefs_get_inode(dir->i_sb, mode);

    if (inode) {
        d_instantiate(dentry, inode);
        dget(dentry);
        inode->i_private = NULL;
        err = 0;
    }
    return err;
}


static int islenefs_unlink(struct inode * dir, struct dentry *dentry ) {

    struct inode * inode = dentry->d_inode;
    free_pages((unsigned long)(inode->i_private), get_order(inode->i_size));

    inode->i_ctime = dir->i_ctime;
    inode_dec_link_count(inode);

    return 0;			
}

/* 
 * FunÃ§Ã£o islenefs_link experimental
 */

static int islenefs_link(struct dentry * old_dentry, struct inode * dir, 
        struct dentry * dentry) {

    struct inode * inode = old_dentry->d_inode;

    inode->i_ctime = CURRENT_TIME;
    inode_inc_link_count(inode);
    atomic_inc(&inode->i_count);

    d_instantiate(dentry,inode);
    dget(dentry);

    return 0;
}


static int islenefs_symlink(struct inode * dir, struct dentry *dentry,
        const char * symname) {

    struct inode * inode;
    int error = - ENOSPC;

    inode = islenefs_get_inode(dir->i_sb, S_IFLNK | S_IRWXUGO);

    if (inode) {
        int l = strlen(symname)+1;
        error = page_symlink(inode, symname, l);
        if (!error) {
            if (dir->i_mode & S_ISGID)
                inode->i_gid = dir->i_gid;
            d_instantiate(dentry, inode);
            dget(dentry);
            dir->i_mtime = dir->i_ctime = CURRENT_TIME;
        }
        else 
            iput(inode);
    }
    return error;

}


static int islenefs_mkdir(struct inode * dir, struct dentry *dentry, 
        int mode) {


    struct inode * inode;

    mode |= S_IFDIR;

    inode = islenefs_get_inode(dir->i_sb, mode);

    if (inode) {
        if (dir->i_mode & S_ISGID) {
            inode->i_gid = dir->i_gid;
            if (S_ISDIR(mode))
                inode->i_mode |= S_ISGID;
        }

        d_instantiate(dentry, inode);
        dget(dentry);
        dir->i_mtime = dir->i_ctime = CURRENT_TIME;
        inode->i_ino = ++inode_number;
        inc_nlink(dir);

        return 0;
    }
    return -ENOMEM;
/* G: nah. Sistemas de arquivos em memoria nao retornam 'ENOSPC' 
-> ENOMEM? */

}


const struct inode_operations islenefs_dir_inode_operations = {
    .create		= islenefs_create,
    .lookup		= simple_lookup,
    .unlink	  	= islenefs_unlink,
    .link		= islenefs_link,
    .symlink    = islenefs_symlink,
    .mkdir	    = islenefs_mkdir,
};


