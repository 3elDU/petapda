#ifndef OS_FS_H
#define OS_FS_H

#define MAX_OPEN_FILES 8

int _open(const char *, int);
int _read(int, char *, int);
int _close(int);

#endif