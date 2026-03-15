// Implements a dictionary's functionality

#define _GNU_SOURCE

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "dictionary.h"

unsigned int mmap_len;

unsigned int dictSize = 1;

// Represents number of buckets in a hash table
#define N 524289

// assumed words in dict
#define DICT_SIZE 143093

// Represents a hash table
// unsigned int table[N];

// buffer to hold dictionary
char *buf;

// node pool
unsigned int *pool;

// CHANGE HASH IN CHECK TOO
// Hashes word to a number
__attribute__((always_inline)) unsigned int hash(const char *s)
{
    unsigned int h = 2166136261;
    int c;

    while ((c = *s++))
        h = ((h << 5) + h) ^ c;

    // return a value between 1 and 524288 (inclusive)
    return (unsigned int)((h & 0x7FFFF) + 1);
}

// Loads dictionary into memory, returning true if successful else false
bool load(const char *dictionary)
{
    // Open dictionary
    int fd = open(dictionary, O_RDONLY);
    if (fd == -1)
    {
        return false;
    }

    // get file size
    struct stat st;
    fstat(fd, &st);
    unsigned int size = st.st_size;

    // alignment requirement
    mmap_len = size + 1 + (4 - (size + 1) % 4) + (N + 1) * sizeof(unsigned int);

    // initialise buffer
    buf = mmap(NULL, mmap_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);

    read(fd, buf, size);
    close(fd);
    pool = (unsigned int *)(buf + size + 1 + (4 - (size + 1) % 4));

    // gets one word at a time
    char *word = buf;

    while (*word)
    {
        char *tmp = memchr(word, '\n', LENGTH + 1);
        *tmp = '\0';
        unsigned int hash_val = hash(word);

        while (pool[hash_val] != 0)
        {
            hash_val++;
        }
        pool[hash_val] = word - buf + 1;
        dictSize++;

        word = tmp + 1;
    }

    return true;
}

// Returns number of words in dictionary if loaded else 0 if not yet loaded
unsigned int size(void)
{
    return dictSize - 1;
}

// Returns true if word is in dictionary else false
bool check(const char *word)
{
    char lower_word[LENGTH + 1];

    // lowercase word
    unsigned int i = 0;

    // inlined hash
    unsigned int hash_val = 2166136261;
    int c;

    while ((c = *word++))
    {
        hash_val = ((hash_val << 5) + hash_val) ^ (lower_word[i] = c | 0x20);
        i++;
    }
    lower_word[i] = '\0';
    unsigned int initial_hash_val;
    initial_hash_val = hash_val = (unsigned int)((hash_val & 0x7ffff) + 1);

    for (unsigned int trav = pool[hash_val]; trav != 0 && hash_val != N; trav = pool[hash_val])
    {
        if (!memcmp(buf + trav - 1, lower_word, i + 1))
        {
            if (initial_hash_val != hash_val)
            {
                // swap found element with first
                unsigned int tmp = pool[hash_val];
                pool[hash_val] = pool[initial_hash_val];
                pool[initial_hash_val] = tmp;
            }
            return true;
        }
        ++hash_val;
    }
    return false;
}

// Unloads dictionary from memory, returning true if successful else false
bool unload(void)
{
    munmap(buf, mmap_len);
    return true;
}
