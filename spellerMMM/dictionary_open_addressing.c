// Implements a dictionary's functionality

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "dictionary.h"


// Hashing constants
#define HASH_INIT 182523
#define POW 19
#define N (1 << POW)  // 2^19

// Hash table, dictionary, size, stat struct
int hashtable[N];
char *_dict, *dict;  // dict points 1 to the left of _dict, since word_pos counts from 1.
struct stat sb;
int dict_size;

// Fib hash
// #define FIB(h) (((h) * 11400714819323198485llu) >> (64 - POW))
// #define FIB(h) (((h) * 11400714819323198485llu) & ((1 << POW) - 1))
#define FIB(h) ((h) & ((1 << POW) - 1))

// Returns true if word is in dictionary, else false
bool check(const char *word) {
    unsigned int hsh = hash(word), hsh_orig = hsh;
    while (hashtable[hsh]) {
        if (strcasecmp(dict + hashtable[hsh], word) == 0) {
            int tmp = hashtable[hsh];
            hashtable[hsh] = hashtable[hsh_orig];
            hashtable[hsh_orig] = tmp;
            return 1;
        }
        hsh = (hsh + 1) & (N - 1);
    }
    return 0;
}

// Hashes word to a number
unsigned int hash(const char *word) {
    uint64_t hsh = HASH_INIT;
    while (*word) {
        hsh = (hsh * 31) + (*word | 32);
        word++;
    }
    return FIB(hsh);
}

// Loads dictionary into memory, returning true if successful, else false
bool load(const char *dictionary) {

    int fd = open(dictionary, O_RDONLY);

    if (fd == -1)
    {
        printf("Could not open file.\n");
        return 0;
    }

    fstat(fd, &sb);
    _dict = mmap(NULL, sb.st_size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
    dict = _dict - 1;
    close(fd);

    uint64_t hsh = HASH_INIT;
    int word_pos = 1;

    for (char *i = _dict, *end = _dict + sb.st_size; i < end; i++) {
        if (*i == '\n') {
            hsh = FIB(hsh);
            while (hashtable[hsh] != 0) hsh = (hsh + 1) & (N - 1);
            hashtable[hsh] = word_pos;
            *i = '\0';
            // printf("Placing %s (pos %i) -> hashtable[%lu]\n", dict + word_pos, word_pos, hsh);
            word_pos = i - dict + 1;
            dict_size++;
            hsh = HASH_INIT;
        } else {
            hsh = (hsh * 31) + (*i | 32);
        }
    }

    // printf("Dictionary loaded with %i collisions\n", collisions);
    return 1;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size(void) {
    return dict_size;
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload(void)
{
    munmap(dict, sb.st_size);
    memset(hashtable, 0, sizeof hashtable);
    return 1;
}