// Declares a dictionary's functionality

#ifndef BENCHMARK_DICTIONARY_H
#define BENCHMARK_DICTIONARY_H

#include <stdbool.h>

// Maximum length for a word
// (e.g., pneumonoultramicroscopicsilicovolcanoconiosis)
#define LENGTH_B 45

// Prototypes
static bool check_BENCH(const char *word);
static unsigned int hash_BENCH(const char *word);
static bool load_BENCH(const char *dictionary);
static unsigned int size_BENCH(void);
static bool unload_BENCH(void);

#endif // BENCHMARK_DICTIONARY_H
