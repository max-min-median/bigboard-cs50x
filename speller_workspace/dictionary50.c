// Implements a dictionary's functionality

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "dictionary50.h"

// Represents a node in a hash table
typedef struct node
{
    char word[LENGTH_B + 1];
    struct node *next;
}
node;

static const unsigned int N = 80000;

static unsigned int size_;

// Hash table
static node *table[N];

// Returns true if word is in dictionary, else false
bool check_BENCH(const char *word)
{
    unsigned int index = hash_BENCH(word);
    node *n = table[index];
    while (n)
    {
        if (strcasecmp(n->word, word) == 0)
            return true;
        n = n->next;
    }
    return false;
}

// Hashes word to a number
unsigned int hash_BENCH(const char *word)
{
    unsigned int hash = 5381;
    while (*word)
    {
        char c = tolower(*word);
        word++;
        hash = ((hash << 5) + hash) + c;
    }
    return hash % N;
}

// Loads dictionary into memory, returning true if successful, else false
bool load_BENCH(const char *dictionary)
{
    size_ = 0;

    FILE *d = fopen(dictionary, "r");
    if (! d)
        return false;

    while (1)
    {
        node *n = malloc(sizeof(node));
        if (! n)
            break;
        if (fscanf(d, "%s", n->word) == EOF)
        {
            free(n);
            fclose(d);
            return true;
        }
        unsigned int index = hash_BENCH(n->word);
        n->next = table[index];
        table[index] = n;
        size_++;
    }

    return false;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size_BENCH(void)
{
    return size_;
}

void vacate_list_BENCH(node *n)
{
    if (! n)
        return;
    vacate_list_BENCH(n->next);
    free(n);
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload_BENCH(void)
{
    for (unsigned int i = 0; i < N; ++i)
    {
        node *n = table[i];
        vacate_list_BENCH(n);
    }
    return true;
}

void clear_table_BENCH(void)
{
    // null out table[]
    memset(table, 0, N * sizeof *table);
}