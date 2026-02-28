#include "local.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"

struct local_context local_context = {0};

static char *strip(char *s) {
    while (*s == ' ' || *s == '\t') {
        s++;
    }

    char *end = s + strlen(s) - 1;

    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end-- = '\0';
    }

    return s;
}

int local_load(const char *locale) {
    uint32_t len;
    char *data = archive_read_alloc(SAUSAGES_DATA, locale, &len);

    if (!data) {
        return -1;
    }

    local_context.len = 0;
    strncpy(local_context.locale, locale, LOCAL_LOCALE - 1);
    local_context.locale[LOCAL_LOCALE - 1] = '\0';

    char *save_ptr = NULL;
    char *line = strtok_r(data, "\n", &save_ptr);

    while (line) {
        char *trimmed = strip(line);

        if (*trimmed == '\0' || *trimmed == '#') {
            line = strtok_r(NULL, "\n", &save_ptr);
            continue;
        }

        char *delim = strchr(trimmed, '=');
        if (!delim) {
            line = strtok_r(NULL, "\n", &save_ptr);
            continue;
        }

        *delim = '\0';
        const char *key = strip(trimmed);
        const char *value = strip(delim + 1);

        if (*key == '\0') {
            line = strtok_r(NULL, "\n", &save_ptr);
            continue;
        }

        if (local_context.len >= LOCAL_ENTRIES) {
            break;
        }

        struct local_entry *entry = &local_context.entries[local_context.len];

        strncpy(entry->key, key, LOCAL_KEY - 1);
        entry->key[LOCAL_KEY - 1] = '\0';

        strncpy(entry->value, value, LOCAL_VALUE - 1);
        entry->value[LOCAL_VALUE - 1] = '\0';

        local_context.len++;
        line = strtok_r(NULL, "\n", &save_ptr);
    }

    free(data);

    return 0;
}

const char *local_get(const char *key) {
    for (int i = 0; i < local_context.len; i++) {
        if (strcmp(local_context.entries[i].key, key) == 0) {
            return local_context.entries[i].value;
        }
    }
    return key;
}

const char *local_current_locale(void) {
    return local_context.locale;
}
