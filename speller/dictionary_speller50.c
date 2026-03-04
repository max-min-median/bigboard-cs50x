// Implements a dictionary's functionality

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "dictionary.h"

bool strcustomcmp(char* word, char* dict_word);
unsigned int hash_djb(const char *str);
void printDictionary();

// TODO: Choose number of buckets in hash table
#define N 1048576  // pow of 2
#define NEXT_HASH hsh++
#define MAX_COLLISIONS 20
// #define NEXT_HASH hsh = (hsh * 5 + 1) & (N - 1)

// Hash table
int table[N];  // 0 is a sentinel value indicating no word
char *dict_map;
struct stat sb;
int bucketCounter[N];
int collisionCount[MAX_COLLISIONS + 1];
int dictionaryWordCount = 0;
int pos_multiplier[LENGTH + 1];
int letter_value[LENGTH + 1][123];
int hashmiss = 0;

// Returns true if word is in dictionary, else false
bool check(const char *word)
{
    int hsh = hash(word);
    if (strcasecmp(word, dict_map + table[hsh] - 1) == 0) return 1;
    int firsthash = hsh;
    while (1) {
        hashmiss++;
        NEXT_HASH;
        if (table[hsh] == 0) return 0;
        if (strcasecmp(word, dict_map + table[hsh] - 1) == 0) {
            int tmp = table[firsthash];
            table[firsthash] = table[hsh];
            table[hsh] = tmp;
            return 1;
        }
    }
}

// Hashes word to a number
unsigned int hash(const char *word) // Word can be in any case.
{
    int result = 0;
    int *letter_val = (int *) letter_value;
    while (*word) {
        result += letter_val[(int) *word++];
        letter_val += 123;
    }
    return result & (N - 1);
}

// unsigned int hash_djb2(const char *str) {
//     unsigned long h = 5381;
//     int c;
//     while ((c = *str++)) h = ((h << 5) + h) + c; /* hash * 33 + c */
//     return h % N;
// }

// Loads dictionary into memory, returning true if successful, else false
bool load(const char *dictionary)
{
    const int mult = 31;
    pos_multiplier[0] = 1;
    for (int i = 1; i <= LENGTH; i++) pos_multiplier[i] = (pos_multiplier[i - 1] * mult) % N;

    int *letter_val = (int *) letter_value;
    for (int i = 0; i <= LENGTH; i++, letter_val += 123) {
        for (int j = 0; j < 26; j++)
            letter_val[j + 'A'] = letter_val[j + 'a'] = (j * pos_multiplier[i]) % N;
        letter_val['\''] = 0;
    }

    int fd = open(dictionary, O_RDONLY);
    if (fd == -1)
    {
        printf("Could not open file.\n");
        return 0;
    }

    fstat(fd, &sb);
    dict_map = mmap(NULL, sb.st_size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    // *(dict_map + sb.st_size) = '\n';  // Terminate the file with '\n'

    int wordPos = 1;  // One-indexed! NOT zero-indexed!
    for (int i = 0, len = 0; i < sb.st_size; i++)
    {
        if (dict_map[i] != '\n') { len++; continue; }
        dict_map[i] = '\0';
        dictionaryWordCount++;
        int hsh = hash(dict_map + wordPos - 1);
        bucketCounter[hsh]++;
        if (table[hsh] == 0) {
            table[hsh] = wordPos;
        } else {
            for (int wordPosCpy = wordPos, otherlen; ; ) {
                NEXT_HASH;
                if (table[hsh] == 0) {
                    table[hsh] = wordPosCpy;
                    break;
                } else if (len < (otherlen = strlen(dict_map + table[hsh] - 1))) {
                    int tmp = table[hsh];
                    table[hsh] = wordPosCpy;
                    wordPosCpy = tmp;
                    len = otherlen;
                }
            }
        }
        wordPos = i + 2;  // move to start of next word
        len = 0;
    }

/*
    int sumsq = 0;
    for (int i = 0; i < N; i++) collisionCount[bucketCounter[i]]++;
    for (int i = 0; i <= MAX_COLLISIONS; i++) {
        printf("Buckets with %i nodes: %i\n", i, collisionCount[i]);
        sumsq += i * i * collisionCount[i];
    }
    printf("\nSum of squares: %i\n", sumsq);
    // printDictionary();
*/
    return 1;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size(void)
{
    return dictionaryWordCount;
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload(void)
{
    munmap(dict_map, sb.st_size + 1);
    return true;
}

bool strcustomcmp(char* w1, char* w2) {
    for ( ; ; w1++, w2++) {
        if (*w1 == *w2) {
            if (*w1 == '\0') return 1;
            continue;
        } else if (*w2 - *w1 == 32) {
            return 0;
        }
    }
}

void printDictionary()
{
    int count = 0;
    printf("Dictionary Words (as positioned in hash table)\n\n");
    for (int i = 0; i < N; i++) {
        if (table[i] == 0) continue;
        printf("%i: %s\n", i, dict_map + table[i] - 1);
    }
    printf("\nTotal words: %i\n", count);
    exit(EXIT_SUCCESS);
}