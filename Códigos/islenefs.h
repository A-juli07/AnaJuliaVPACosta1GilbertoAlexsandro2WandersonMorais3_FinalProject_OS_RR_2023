#ifndef _ISLENEFS_H_
#define _ISLENEFS_H_

extern struct inode * islenefs_get_inode(struct super_block *sb, int mode);

extern const struct super_operations islenefs_ops;
extern const struct file_operations islenefs_file_operations;
extern const struct inode_operations islenefs_file_inode_operations;
extern const struct inode_operations islenefs_dir_inode_operations;

#endif

