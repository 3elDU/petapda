#include "fs.h"

#include <fs/ff.h>
#include <pico/printf.h>
#include <string.h>

typedef struct
{
    int fd;
    FIL fp;
    bool eof;
} openfile_t;

openfile_t open_files[MAX_OPEN_FILES];

/** Finds the next available file entry and returns a pointer to it */
openfile_t *next_avail_file()
{
    int fd_max = 3;

    for (uint i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (open_files[i].fd > fd_max)
            fd_max = open_files[i].fd;
    }

    for (uint i = 0; i < MAX_OPEN_FILES; i++)
    {
        openfile_t *f = &open_files[i];
        if (f->fd == 0)
        {
            f->fd = fd_max;
            return f;
        }
    }

    return NULL;
}
/** Finds the open file by file descriptor */
openfile_t *find_open_file(int fd)
{
    for (uint i = 0; i < MAX_OPEN_FILES; i++)
    {
        openfile_t *f = &open_files[i];
        if (f->fd == fd)
            return f;
    }

    return NULL;
}

int _open(const char *path, int flag)
{
    printf("_open(%s, 0x%X)\n", path, flag);

    FRESULT fr;
    openfile_t *f = next_avail_file();

    if (f == NULL)
        return -1;

    fr = f_open(&f->fp, path, FA_READ); // support readonly for now
    if (fr != FR_OK)
    {
        printf("_open(%s) error - %d\n", path, fr);
        // mark file entry as free
        f->fd = 0;
        return -1;
    }

    return f->fd;
}

int _read(int handle, char *buffer, int length)
{
    printf("_read(fd=%d, len=%d)\n", handle, length);

    openfile_t *f = find_open_file(handle);
    if (f == NULL)
        return 0;

    if (f->eof)
    {
        return 0;
    }

    UINT br;
    FRESULT fr;

    fr = f_read(&f->fp, buffer, length, &br);
    if (fr != FR_OK)
    {
        printf("_read() error - %d\n", fr);
        return -1;
    }

    if (br < length)
        f->eof = true;

    return (int)br;
}

int _close(int handle)
{
    printf("_close(fp=%d)\n", handle);

    openfile_t *f = find_open_file(handle);

    if (f == NULL)
        return -1;

    FRESULT fr = f_close(&f->fp);
    if (fr != FR_OK)
    {
        printf("_close() error, fr=%d\n", fr);
        return -1;
    }

    memset(f, 0, sizeof(openfile_t));

    return 0;
}