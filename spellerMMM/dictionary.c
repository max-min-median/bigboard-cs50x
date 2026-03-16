// Implements a dictionary's functionality

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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

// Fibonacci Hash
// #define FIB(h) (((h) * 11400714819323198485llu) & ((1 << POW) - 1))
#define FIB(h) (((h) * 11400714819323198485llu) >> (64 - POW))

char *dict, *_dict;  // dict points 1 to the left of _dict, since word_pos counts from 1.
int dict_size;
struct stat sb;

// Represents a node in a hash table
typedef struct __attribute__((aligned(32))) {
    int8_t position_to_compare;  // -1 if unable to find a position which differs in all possibilities
    char possibilities[7]; // possible letters at that position
    // (3 bytes padding)
    uint32_t offsets[6];
    // word offsets of each position
} hashinfo;

// Hash table
hashinfo hashtable[N];

// Returns true if word is in dictionary, else false
bool check(const char *word)
{
    unsigned int hsh = hash(word);
    uint8_t cmp_pos = hashtable[hsh].position_to_compare;
    if (cmp_pos >= 0) {
        char *word_no = memchr(hashtable[hsh].possibilities, word[cmp_pos] | 32, 5);
        if (!word_no) return 0;
        return strcasecmp(word, dict + hashtable[hsh].offsets[word_no - hashtable[hsh].possibilities]) == 0;
    } else {
        for (int i = 0; i < 5; i++) {
            if (hashtable[hsh].offsets[i] == 0) return 0;
            if (strcasecmp(word, dict + hashtable[hsh].offsets[i]) == 0) return 1;
        }
        return 0;
    }
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

    fstat(fd, &sb);
    _dict = mmap(NULL, sb.st_size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
    dict = _dict - 1;
    close(fd);

    uint64_t hsh = HASH_INIT;
    int word_pos = 1;

    for (char *i = _dict, *end = _dict + sb.st_size; i < end; i++) {
        if (*i == '\n') {
            *i = '\0';
            hsh = FIB(hsh);
            int poss_idx = 0;
            while (hashtable[hsh].offsets[poss_idx]) poss_idx++;  // go to new word position
            hashtable[hsh].offsets[poss_idx] = word_pos;  // insert word in hashinfo
            uint8_t found_position = 0;
            for (int pos = 0; pos < LENGTH + 1; pos++) {  // for each letter position from 0..44
                for (int w1 = 0; w1 < poss_idx; w1++) {
                    for (int w2 = w1 + 1; w2 <= poss_idx; w2++) {
                        char l1 = dict[hashtable[hsh].offsets[w1] + pos], l2 = dict[hashtable[hsh].offsets[w2] + pos];
                        if (l1 == 0 || l2 == 0) goto cannotFindPos;
                        if (l1 == l2) goto nextPos;
                    }
                }
                // Found suitable position where all words have a different character
                found_position = 1;
                hashtable[hsh].position_to_compare = pos;
                for (int w = 0; w <= poss_idx; w++) {
                    hashtable[hsh].possibilities[w] = dict[hashtable[hsh].offsets[w] + pos];
                }
                break;
                nextPos: ;
            }
            cannotFindPos:
            if (!found_position) hashtable[hsh].position_to_compare = -1;

            hsh = HASH_INIT;
            word_pos = i - dict + 1;
            dict_size++;
        } else {
            hsh = (hsh << 6) - hsh + (*i | 32);
        }
    }

    return 1;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size(void)
{
    return dict_size;
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload(void)
{
    munmap(dict, sb.st_size);
    memset(hashtable, 0, sizeof hashtable);
    return 1;
}