#ifndef ARCHIVE_H
#define ARCHIVE_H

#define ARCHIVE_MAGIC 0x4B415021
#define ARCHIVE_NAMELEN 56
/* size of entry on disk */
#define ARCHIVE_ENTRY_SIZE (ARCHIVE_NAMELEN + 4 + 4)

/* entry: name(56) | offset(4) | size(4) */
struct archive_entry {
    char name[ARCHIVE_NAMELEN];
    unsigned long offset;
    unsigned long size;
};

/* header: magic(4) | n(4) */
struct archive_header {
    unsigned long magic;
    /* the amount of files */
    unsigned long n;
};

/* data is raw file bytes. */

/* argv[0..n-1] are filenames to pack */
int archive_create(const char *name, char **argv, int n);
/* list contents of archive */
int archive_list(const char *name);
int archive_extract_alloc(const char *name);
/* returns a malloc'd buffer */
void *archive_read_alloc(const char *name, const char *file, unsigned long *len);

#endif /* ARCHIVE_H */