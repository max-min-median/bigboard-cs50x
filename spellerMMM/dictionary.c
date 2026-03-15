// Implements a dictionary's functionality

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "dictionary.h"

// Hashing constants
#define HASH_INIT 182523
#define POW 19
#define N (1 << POW)  // 2^19

// Fibonacci Hash
#define FIB(h) (((h) * 11400714819323198485llu) & ((1 << POW) - 1))

char *dict;  // dict points 1 to the left of _dict, since word_pos counts from 1.

// Represents a node in a hash table
typedef struct
{
    int8_t position_to_compare;  // -1 if unable to find a position which differs in all possibilities
    char possibilities[5]; // possible letters at that position
    // (3 bytes padding)
    uint32_t offsets[5];
    // word offsets of each position
} hashinfo;

// Hash table
hashinfo hashtable[N];

// Returns true if word is in dictionary, else false
bool check(const char *word)
{
    // TODO
    return false;
}

// Hashes word to a number
unsigned int hash(const char *word) {
    uint64_t hsh = HASH_INIT;
    while (*word) {
        hsh = (hsh << 6) - hsh + (*word | 32);
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

    struct stat sb;

    fstat(fd, &sb);
    dict = mmap(NULL, sb.st_size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    uint64_t hsh = HASH_INIT;
    int word_pos = 0;
    for (char *i = dict, *end = dict + sb.st_size; i < end; i++) {
        if (*i == '\n') {
            *i = '\0';
            hsh = FIB(hsh);
            int poss_idx = 0;
            while (hashtable[hsh].possibilities[poss_idx]) poss_idx++;  // go to new word position
            hashtable[hsh].offsets[poss_idx] = word_pos;  // insert word in hashinfo
            uint8_t found_position = 0;
            for (int pos = 0; pos < poss_idx; pos++) {  // for each letter position from 0..44
                for (int w1 = 0; w1 < poss_idx; w1++) {
                    for (int w2 = w1 + 1; w2 <= poss_idx; w2++) {
                        if (dict[hashtable[hsh].offsets[w1] + pos] == dict[hashtable[hsh].offsets[w2] + pos])
                            goto nextPos;
                    }
                }
                // Found suitable position where all words have a different character
                found_position = 1;
                hashtable[hsh].position_to_compare = pos;
                for (int w = 0; w <= poss_idx; w++) {
                    hashtable[hsh].possibilities[w] = dict[hashtable[hsh].offsets[w] + pos];
                }
                printf("Found common letter position (%i) for hashtable[%lu]. Words here are: ", pos, hsh);
                for (int w = 0; w <= poss_idx; w++) printf("%s ", dict + hashtable[hsh].offsets[w]);
                break;
                nextPos: ;
            }
            if (!found_position) {
                hashtable[hsh].position_to_compare = -1;
                printf("hashtable[%lu]: Could not find a common letter position. Words are: ", hsh);
                for (int w = 0; w <= poss_idx; w++) printf("'%s' ", dict + hashtable[hsh].offsets[w]);
                puts("");
            }
            // printf("Placing %s (pos %i) -> hashtable[%lu]\n", dict + word_pos, word_pos, hsh);
            hsh = HASH_INIT;
            word_pos = i - dict + 1;
        } else {
            hsh = (hsh << 6) - hsh + (*i | 32);
        }
    }

    return 1;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size(void)
{
    // TODO
    return 0;
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload(void)
{
    // TODO
    return false;
}