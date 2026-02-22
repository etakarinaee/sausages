#include "archive.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put32(const uint32_t x, FILE *f) {
    fputc((int)(x & 0xffu), f);
    fputc((int)(x >> 8 & 0xffu), f);
    fputc((int)(x >> 16 & 0xffu), f);
    fputc((int)(x >> 24 & 0xffu), f);
}

static uint32_t get32(FILE *f) {
    uint32_t x = (uint32_t)(fgetc(f) & 0xff);
    x |= (uint32_t)(fgetc(f) & 0xff) << 8;
    x |= (uint32_t)(fgetc(f) & 0xff) << 16;
    x |= (uint32_t)(fgetc(f) & 0xff) << 24;

    return x;
}

static long fsize(FILE *f) {
    const long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    const long end = ftell(f);
    fseek(f, pos, SEEK_SET);

    return end;
}

static void fcopy(FILE *dst, FILE *src, size_t n) {
    char buf[4096];

    while (n > 0) {
        size_t r = n < sizeof buf ? n : sizeof buf;
        r = fread(buf, 1, r, src);

        if (r == 0) {
            break;
        }

        fwrite(buf, 1, r, dst);
        n -= r;
    }
}

int archive_create(const char *name, char **argv, const int n) {
    FILE *in;
    int i;

    FILE *out = fopen(name, "wb");
    if (!out) {
        perror(name);

        return -1;
    }

    struct archive_entry *dir = calloc(n, sizeof(struct archive_entry));
    if (!dir) {
        fprintf(stderr, "out of memory");
        fclose(out);

        return -1;
    }

    put32(ARCHIVE_MAGIC, out);
    put32((uint32_t)n, out);

    /* data starts after header and directory */
    uint32_t offset = 8 + (uint32_t)n * ARCHIVE_ENTRY_SIZE;

    /* measure each file and build dirs only in memory yet */
    for (i = 0; i < n; i++) {
        in = fopen(argv[i], "rb");
        if (!in) {
            perror(argv[i]);
            free(dir);
            fclose(out);

            return -1;
        }

        /* store only the filename, not the full path */
        const char *base = strrchr(argv[i], '/');
        base = base ? base + 1 : argv[i];

        strncpy(dir[i].name, base, ARCHIVE_NAMELEN - 1);
        dir[i].size = (uint32_t)fsize(in);
        dir[i].offset = offset;
        offset += dir[i].size;

        fclose(in);
    }

    /* write directory so reader can find files without scanning */
    for (i = 0; i < n; i++) {
        fwrite(dir[i].name, 1, ARCHIVE_NAMELEN, out);

        put32(dir[i].offset, out);
        put32(dir[i].size, out);
    }

    /* write data where it was promised to be written */
    for (i = 0; i < n; i++) {
        in = fopen(argv[i], "rb");
        if (!in) {
            perror(argv[i]);
            free(dir);
            fclose(out);

            return -1;
        }

        fcopy(out, in, dir[i].size);
        fclose(in);
    }

    free(dir);
    fclose(out);

    return 0;
}

/* list contents of archive */
int archive_list(const char *name) {
    struct archive_entry e;

    FILE *f = fopen(name, "rb");
    if (!f) {
        perror(name);

        return -1;
    }

    const uint32_t magic = get32(f);
    if (magic != ARCHIVE_MAGIC) {
        fprintf(stderr, "%s: not a valid archive\n", name);
        fclose(f);

        return -1;
    }

    const uint32_t n = get32(f);
    printf("%-56s %10s %10s\n", "name", "offset", "size");
    for (uint32_t i = 0; i < n; i++) {
        fread(e.name, 1, ARCHIVE_NAMELEN, f);
        e.offset = get32(f);
        e.size = get32(f);

        printf("%-56s %10" PRIu32 " %10" PRIu32 "\n", e.name, e.offset, e.size);
    }

    fclose(f);

    return 0;
}

int archive_extract_alloc(const char *name) {
    uint32_t i;

    FILE *f = fopen(name, "rb");
    if (!f) {
        perror(name);
        return -1;
    }

    const uint32_t magic = get32(f);
    if (magic != ARCHIVE_MAGIC) {
        fprintf(stderr, "%s: not a valid archive\n", name);
        fclose(f);

        return -1;
    }

    const uint32_t n = get32(f);
    struct archive_entry *dir = calloc(n, sizeof(struct archive_entry));
    if (!dir) {
        fprintf(stderr, "out of memory\n");
        fclose(f);

        return -1;
    }

    for (i = 0; i < n; i++) {
        fread(dir[i].name, 1, ARCHIVE_NAMELEN, f);
        dir[i].offset = get32(f);
        dir[i].size = get32(f);
    }

    for (i = 0; i < n; i++) {
        printf("%s: %" PRIu32 " bytes\n", dir[i].name, dir[i].size);
        FILE *out = fopen(dir[i].name, "wb");
        if (!out) {
            perror(dir[i].name);
            continue;
        }

        fseek(f, (long)dir[i].offset, SEEK_SET);
        fcopy(out, f, dir[i].size);

        fclose(out);
    }

    free(dir);
    fclose(f);

    return 0;
}

void *archive_read_alloc(const char *name, const char *file, uint32_t *len) {
    struct archive_entry e;

    FILE *f = fopen(name, "rb");
    if (!f) {
        perror(name);

        return NULL;
    }

    const uint32_t magic = get32(f);
    if (magic != ARCHIVE_MAGIC) {
        fprintf(stderr, "%s: not a valid archive\n", name);
        fclose(f);

        return NULL;
    }

    const uint32_t n = get32(f);
    for (uint32_t i = 0; i < n; i++) {
        fread(e.name, 1, ARCHIVE_NAMELEN, f);
        e.offset = get32(f);
        e.size = get32(f);

        if (strcmp(e.name, file) == 0) {
            void *buf = malloc(e.size + 1);
            if (!buf) {
                fprintf(stderr, "out of memory\n");
                fclose(f);

                return NULL;
            }

            fseek(f, (long)e.offset, SEEK_SET);
            fread(buf, 1, e.size, f);
            ((char *) buf)[e.size] = '\0';

            fclose(f);
            *len = e.size;

            return buf;
        }
    }

    fprintf(stderr, "%s: no entry '%s'\n", name, file);
    fclose(f);

    return NULL;
}