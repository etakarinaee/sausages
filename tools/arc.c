/*
* usage:  arc c archive.arc file1 file2 ...
 *        arc l archive.arc
 *        arc x archive.arc
 */

#include <stdio.h>

#include "../archive.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: arc [c|l|x] archive.arc [files...]\n");
        return 1;
    }

    switch (argv[1][0]) {
        case 'c': {
            if (argc < 4) {
                fprintf(stderr, "arc c: need files to pack\n");

                return 1;
            }

            return archive_create(argv[2], argv + 3, argc - 3) < 0;
        }

        case 'l': {
            return archive_list(argv[2]) < 0;
        }

        case 'x': {
            return archive_extract(argv[2]) < 0;
        }

        default: {
            fprintf(stderr, "unknown command '%c'\n", argv[1][0]);
            return 1;
        }
    }
}
