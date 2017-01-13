#ifndef H_VFS
#define H_VFS

#include <stdint.h>
#include <list.h>
#include <ata.h>

#define VFS_BLOCK_SIZE 512

#define VFS_NODE_NAME_LENGTH 100
#define MAX_PATH_DEPTH 512
#define MAX_PATH_LENGTH 2048
#define MAX_OPENED_FILES 1024
#define DEFAULT_DIR "/"

#define VFS_CURRENT_DIR "."
#define VFS_PARENT_DIR ".."

typedef struct vfs_node vfs_node_t;
typedef int32_t file_descriptor_t;
typedef struct vfs_file vfs_file_t;
typedef struct vfs_file_operations vfs_file_operations_t;
typedef struct vfs_node_operations vfs_node_operations_t;


typedef uint32_t gid_t;
typedef uint32_t uid_t;
typedef uint32_t dev_t;
typedef uint32_t ino_t;
typedef uint32_t mode_t;
typedef unsigned short nlink_t;
typedef long off_t;
typedef long time_t;
typedef int blksize_t;
typedef long blkcnt_t;

struct timespec {
  time_t  tv_sec;   /* Seconds */
  long    tv_nsec;  /* Nanoseconds */
};

#define S_IFMT 0170000	/* type of file */
#define S_IFDIR 0040000	/* directory */
#define S_IFCHR 0020000	/* character special */
#define S_IFBLK 0060000	/* block special */
#define S_IFREG 0100000	/* regular */
#define S_IFLNK 0120000	/* symbolic link */
#define S_IFSOCK 0140000	/* socket */
#define S_IFIFO 0010000	/* fifo */
#define S_MNT 0160000 /* mount point */

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

typedef struct dirent
{
    char name[VFS_NODE_NAME_LENGTH];
} dirent_t;

struct vfs_super {
    struct ata_device *dev;
    struct vfs_super_operations *ops;
    void *private;
};

typedef struct vfs_fs_operations
{
    struct vfs_super* (*read_super)(struct ata_device*);
} vfs_fs_operations_t;

struct vfs_super_operations
{
    struct vfs_node* (*spawn)(struct vfs_super*, char*, mode_t);
};

struct vfs_node_operations
{
    uint32_t (*read_dir)(vfs_node_t*, void*, uint32_t);
    vfs_node_t* (*lookup)(vfs_node_t*, char*);
    int (*create_node)(vfs_node_t*, char*, mode_t, struct vfs_file_operations*, void*, vfs_node_t**);
    int (*stat)(vfs_node_t *, char *, struct stat *);
};

struct vfs_file_operations
{
    int (*open)(vfs_file_t*, uint32_t);
    int (*read)(vfs_file_t*, void*, uint32_t, uint32_t*);
    int (*write)(vfs_file_t*, void*, uint32_t, uint32_t*);
    int (*close)(vfs_file_t*);
};

struct vfs_node
{
    list_node_t list;
    vfs_node_t *children;
    char name[VFS_NODE_NAME_LENGTH];
    uint32_t size;
    mode_t mode;
    void *obj;
    uint8_t is_loaded;
    struct vfs_super *super;
    vfs_node_operations_t *ops;
    vfs_file_operations_t *file_ops;
};

struct vfs_fs_type
{
    char name[100];
    vfs_fs_operations_t *ops;
};

#define FD_CLOEXEC 1
#define F_DUPFD_CLOEXEC 14

struct vfs_file
{
    list_node_t list;
    vfs_node_t *node;
    vfs_file_operations_t *ops;
    uint32_t pid;
    int flags;
    uint32_t pos;
};

int register_fs(struct vfs_fs_type *fs);
int mount_fs(char *path, char *fs_name, struct ata_device *dev);
int mkdir(char *path);
file_descriptor_t sys_open(char *path);
int sys_read(file_descriptor_t fd, void *buf, uint32_t size);
int sys_write(file_descriptor_t fd, void *buf, uint32_t size);
int sys_close(file_descriptor_t fd);
int create_vfs_node(char *path, mode_t mode, vfs_file_operations_t *file_ops, void *obj, vfs_node_t **out);
int stat_fs(char *path, struct stat *buf);
int fstat(file_descriptor_t fd, struct stat *buf);
int fcntl(int fd, int cmd, int arg);
int chdir(char *path);

void print_vfs_tree(struct vfs_node *node, uint32_t level);
#endif
