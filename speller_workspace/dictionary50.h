// Declares a dictionary's functionality

#ifndef DICTIONARY50_H
#define DICTIONARY50_H

#include <stdbool.h>

// Maximum length for a word
// (e.g., pneumonoultramicroscopicsilicovolcanoconiosis)
#define LENGTH_B 45

// Prototypes
bool check_BENCH(const char *word);
unsigned int hash_BENCH(const char *word);
bool load_BENCH(const char *dictionary);
unsigned int size_BENCH(void);
bool unload_BENCH(void);
void clear_table_BENCH(void);

#endif // DICTIONARY50_H
