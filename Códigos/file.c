#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/vfs.h>
#include <linux/mm.h>
#include <asm/current.h>
#include <asm/uaccess.h>

#include "islenefs.h"

/* definitions for external objects */
int inode_number;

/* Apos passar pelo VFS, uma leitura chegara aqui. A unica
 * coisa que fazemos eh, achar o ponteiro para o conteudo do arquivo,
 * e retornar, de acordo com o tamanho solicitado */
ssize_t islenefs_read(struct file *file, char __user *buf,
        size_t count, loff_t *pos)
{
    void * conts;
    struct inode *inode = file->f_path.dentry->d_inode;
    int size = count;

    conts = inode->i_private;

    if (inode->i_size < count)
        size = inode->i_size;

    if ((*pos + size) >= inode->i_size)
        size = inode->i_size - *pos;

    /* As page tables do kernel estao sempre mapeadas (veremos o que
     * sao page tables mais pra frente do curso), mas o mesmo nao eh
     * verdade com as paginas de espaco de usuario. Dessa forma, uma
     * atribuicao de/para um ponteiro contendo um endedereco de espaco
     * de usuario pode falhar. Dessa forma, toda a comunicacao
     * de/para espaco de usuario eh feita com as funcoes copy_from_user()
     * e copy_to_user(). */
    if (copy_to_user(buf, conts + *pos, size))
        return -EFAULT;
    *pos += size;

    return size;
}

/* similar a leitura, mas vamos escrever no ponteiro do conteudo.
 * Por simplicidade, estamos escrevendo sempre no comeco do arquivo.
 * Obviamente, esse nao eh o comportamento esperado de um write 'normal'
 * Mas implementacoes de sistemas de arquivos sao flexiveis... */
ssize_t islenefs_write(struct file *file, const char __user *buf,
        size_t count, loff_t *pos)
{
    struct inode *inode = file->f_path.dentry->d_inode;
    struct page *page;


    /* if file conts not initialized */
    if (!inode->i_private) {
        page = alloc_pages(GFP_KERNEL, get_order(count));
        if (!page)
            return -ENOMEM;
        inode->i_private = page_address(page);
    }
/*G: isto esta errado. page_address(0) vai quebrar o kernel. E o resultado de alloc_pages vai ser
 justamente 0 se a alocacao falhar.
 -> o cÃ³digo acima resolve o problema */

    /* copy_from_user() : veja comentario na funcao de leitura */
    if (copy_from_user(inode->i_private + *pos, buf, count)) {
        return -EFAULT;
    }

    inode->i_size = count;
    return count;

}

static int islenefs_open(struct inode *inode, struct file *file)
{
    /* Todo arquivo tem uma estrutura privada associada a ele.
     * Em geral, estamos apenas copiando a do inode, se houver. Mas isso
     * eh tao flexivel quanto se queira, e podemos armazenar aqui
     * qualquer tipo de coisa que seja por-arquivo. Por exemplo: Poderiamos
     * associar um arquivo /mydir/4321 com o processo no 4321 e guardar aqui
     * a estrutura que descreve este processo */

    return 0;
}


const struct file_operations islenefs_file_operations = {
    .read   = islenefs_read,
    .write  = islenefs_write,
    .open   = islenefs_open,
};


const struct inode_operations islenefs_file_inode_operations = {
    .getattr    = simple_getattr,
};

