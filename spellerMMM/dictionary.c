// Implements a dictionary's functionality

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "dictionary.h"

// Represents a node in a hash table
typedef struct node
{
    char word[LENGTH + 1];
    struct node *next;
}
node;

const unsigned int N = 80000;

unsigned int size_;

// Hash table
node *table[N];

// Returns true if word is in dictionary, else false
bool check(const char *word)
{
    unsigned int index = hash(word);
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
unsigned int hash(const char *word)
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
bool load(const char *dictionary)
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
        unsigned int index = hash(n->word);
        n->next = table[index];
        table[index] = n;
        size_++;
    }

    return false;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size(void)
{
    return size_;
}

void vacate_list(node *n)
{
    if (! n)
        return;
    vacate_list(n->next);
    free(n);
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload(void)
{
    for (unsigned int i = 0; i < N; ++i)
    {
        node *n = table[i];
        vacate_list(n);
    }
    return true;
}
