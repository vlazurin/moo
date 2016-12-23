#ifndef H_VFS
#define H_VFS

#include <stdint.h>
#include <list.h>

#define VFS_BLOCK_SIZE 512

#define VFS_NODE_NAME_LENGTH 100
#define MAX_OPENED_FILES 10

#define VFS_CURRENT_DIR "."
#define VFS_PARENT_DIR ".."

#define VFS_SUCCESS 0
#define VFS_BAD_PATH (-1)
#define VFS_NOT_FOUND (-2)
#define VFS_NODE_EXISTS (-3)
#define VFS_BAD_FS (-4)
#define VFS_NOT_INITIALIZED (-5)
#define VFS_FS_MAX_COUNT_REACHED (-6)
#define VFS_LOCKED (-7)
#define VFS_READ_NOT_POSSIBLE (-8)

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

#define ENOENT 2

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

typedef struct vfs_fs_operations
{
    vfs_node_t* (*create_root)();
} vfs_fs_operations_t;

struct vfs_node_operations
{
    uint32_t (*read_dir)(vfs_node_t*, void*, uint32_t);
    uint8_t (*exist)(vfs_node_t*, char*);
    vfs_node_t* (*get_node)(vfs_node_t*, char*);
    uint8_t (*create_node)(vfs_node_t*, char*, mode_t, vfs_file_operations_t*, void*, vfs_node_t**);
    int (*stat)(vfs_node_t *, char *, struct stat *);
};

struct vfs_file_operations
{
    uint32_t (*open)(vfs_file_t*, uint32_t);
    uint32_t (*read)(vfs_file_t*, void*, uint32_t);
    uint32_t (*write)(vfs_file_t*, void*, uint32_t);
    int32_t (*close)(vfs_file_t*);
};

struct vfs_node
{
    list_node_t list;
    vfs_node_t *children;
    char name[VFS_NODE_NAME_LENGTH];
    uint32_t size;
    mode_t mode;
    void *obj;
    vfs_node_operations_t *ops;
    vfs_file_operations_t *file_ops;
};

typedef struct vfs_type
{
    char name[100];
    vfs_fs_operations_t *ops;
} vfs_type_t;

#define FD_CLOEXEC 1

#define F_DUPFD_CLOEXEC 14

struct vfs_file
{
    list_node_t list;
    vfs_node_t *node;
    vfs_file_operations_t *ops;
    uint32_t pid;
    int flags;
};

int register_fs(vfs_type_t *fs);
int mount_fs(char *path, char *fs_name);
int mkdir(char *path);
// probably obj must be int, not a pointer
int create_vfs_device(char *path, vfs_file_operations_t *file_ops, void *obj);
file_descriptor_t sys_open(char *path);
int read_file(file_descriptor_t fd, void *buf, uint32_t size);
uint32_t sys_write(file_descriptor_t fd, void *buf, uint32_t size);
int32_t sys_close(file_descriptor_t fd);
int exist_vfs_node(char *path);
int create_vfs_node(char *path, mode_t mode, vfs_file_operations_t *file_ops, void *obj, vfs_node_t **out);
int stat_fs(char *path, struct stat *buf);
int fstat(file_descriptor_t fd, struct stat *buf);
// TODO: move me!!!
char *strdup(char *str);
int fcntl(int fd, int cmd, int arg);
#endif
