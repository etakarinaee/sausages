#include "archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put32(unsigned long x, FILE *f) {
    fputc(x & 0xff, f);
    fputc((x >> 8) & 0xff, f);
    fputc((x >> 16) & 0xff, f);
    fputc((x >> 24) & 0xff, f);
}

static unsigned long get32(FILE *f) {
    unsigned long x;

    x = fgetc(f) & 0xff;
    x |= (fgetc(f) & 0xff) << 8;
    x |= (fgetc(f) & 0xff) << 16;
    x |= (fgetc(f) & 0xff) << 24;

    return x;
}

static long fsize(FILE *f) {
    long pos, end;

    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);

    return end;
}

static void fcopy(FILE *dst, FILE *src, unsigned long n) {
    char buf[4096];
    unsigned long r;

    while (n > 0) {
        r = n < sizeof buf ? n : sizeof buf;
        r = fread(buf, 1, r, src);

        if (r == 0) {
            break;
        }

        fwrite(buf, 1, r, dst);
        n -= r;
    }
}

int archive_create(const char *name, char **argv, const int n) {
    FILE *out, *in;
    struct archive_entry *dir;
    unsigned long offset;
    int i;

    out = fopen(name, "wb");
    if (!out) {
        perror(name);

        return -1;
    }

    dir = calloc(n, sizeof(struct archive_entry));
    if (!dir) {
        fprintf(stderr, "out of memory");
        fclose(out);

        return -1;
    }

    put32(ARCHIVE_MAGIC, out);
    put32(n, out);

    /* data starts after header and directory */
    offset = 8 + n * ARCHIVE_ENTRY_SIZE;

    /* measure each file and build dirs only in memory yet */
    for (i = 0; i < n; i++) {
        in = fopen(argv[i], "rb");
        if (!in) {
            perror(argv[i]);
            free(dir);
            fclose(out);

            return -1;
        }

        strncpy(dir[i].name, argv[i], ARCHIVE_NAMELEN - 1);
        dir[i].size = fsize(in);
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
    FILE *f;
    unsigned long magic, n, i;
    struct archive_entry e;

    f = fopen(name, "rb");
    if (!f) {
        perror(name);

        return -1;
    }

    magic = get32(f);
    if (magic != ARCHIVE_MAGIC) {
        fprintf(stderr, "%s: not a valid archive\n", name);
        fclose(f);

        return -1;
    }

    n = get32(f);
    printf("%-56s %10s %10s\n", "name", "offset", "size");
    for (i = 0; i < n; i++) {
        fread(e.name, 1, ARCHIVE_NAMELEN, f);
        e.offset = get32(f);
        e.size = get32(f);

        printf("%-56s %10lu %10lu\n", e.name, e.offset, e.size);
    }

    fclose(f);

    return 0;
}

int archive_extract_alloc(const char *name) {
    FILE *f, *out;
    unsigned long magic, n, i;
    struct archive_entry *dir;

    f = fopen(name, "rb");
    if (!f) {
        perror(name);
        return -1;
    }

    magic = get32(f);
    if (magic != ARCHIVE_MAGIC) {
        fprintf(stderr, "%s: not a valid archive\n", name);
        fclose(f);

        return -1;
    }

    n = get32(f);
    dir = calloc(n, sizeof(struct archive_entry));
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
        printf("%s: %lu bytes\n", dir[i].name, dir[i].size);
        out = fopen(dir[i].name, "wb");
        if (!out) {
            perror(dir[i].name);
            continue;
        }

        fseek(f, dir[i].offset, SEEK_SET);
        fcopy(out, f, dir[i].size);

        fclose(out);
    }

    free(dir);
    fclose(f);

    return 0;
}

void *archive_read_alloc(const char *name, const char *file, unsigned long *len) {
    FILE *f;
    unsigned long magic, n, i;
    struct archive_entry e;
    void *buf;

    f = fopen(name, "rb");
    if (!f) {
        perror(name);

        return NULL;
    }

    magic = get32(f);
    if (magic != ARCHIVE_MAGIC) {
        fprintf(stderr, "%s: not a valid archive\n", name);
        fclose(f);

        return NULL;
    }

    n = get32(f);
    for (i = 0; i < n; i++) {
        fread(e.name, 1, ARCHIVE_NAMELEN, f);
        e.offset = get32(f);
        e.size = get32(f);

        if (strcmp(e.name, file) == 0) {
            buf = malloc(e.size);
            if (!buf) {
                fprintf(stderr, "out of memory\n");
                fclose(f);

                return NULL;
            }

            fseek(f, e.offset, SEEK_SET);
            fread(buf, 1, e.size, f);
            ((char *)buf)[e.size] = '\0';

            fclose(f);
            *len = e.size;

            return buf;
        }
    }

    fprintf(stderr, "%s: no entry '%s'", name, file);
    fclose(f);

    return NULL;
}
