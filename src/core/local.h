// localization system, with every localization file being a basic key-value format
// comments are considered lines that start with #, and obviously are ignored by the parser
#ifndef LOCAL_H
#define LOCAL_H

#define LOCAL_KEY 256
#define LOCAL_VALUE 1024
#define LOCAL_LOCALE 32
#define LOCAL_ENTRIES 1024

struct local_entry {
    char key[LOCAL_KEY];
    char value[LOCAL_VALUE];
};

struct local_context {
    // the name of the current locale
    char locale[LOCAL_LOCALE];
    struct local_entry entries[LOCAL_ENTRIES];
    int len;
};

extern struct local_context local_context;

int local_load(const char *locale);

const char *local_get(const char *key);

const char *local_current_locale(void);

#endif // LOCAL_H
