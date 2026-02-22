#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdint.h>

#define ARCHIVE_MAGIC 0x4B415021
#define ARCHIVE_NAMELEN 56
/* size of entry on disk */
#define ARCHIVE_ENTRY_SIZE (ARCHIVE_NAMELEN + 4 + 4)

/* where the game data is stored */
#define SAUSAGES_DATA "sausages.arc"
/* entrypoint in lua */
#define SAUSAGES_ENTRY "game.lua"

/* entry: name(56) | offset(4) | size(4) */
struct archive_entry {
    char name[ARCHIVE_NAMELEN];
    uint32_t offset;
    uint32_t size;
};

/* header: magic(4) | n(4) */
struct archive_header {
    uint32_t magic;
    /* the amount of files */
    uint32_t n;
};

/* data is raw file bytes. */

/* argv[0..n-1] are filenames to pack */
int archive_create(const char *name, char **argv, int n);

/* list contents of archive */
int archive_list(const char *name);

int archive_extract_alloc(const char *name);

/* returns a malloc 'd buffer */
void *archive_read_alloc(const char *name, const char *file, uint32_t *len);

#endif /* ARCHIVE_H */