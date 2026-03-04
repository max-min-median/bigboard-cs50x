// Implements a spell-checker
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include "dictionary.h"

// Undefine any definitions
#undef calculate
#undef getrusage

// Define timing macros

// #define TIMER(str) do {clock_gettime(CLOCK_MONOTONIC, &tm_##str); clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_##str);} while(0)
// #define CALCTIME(str) do {time_##str##_m = calculate(&tm_start, &tm_end); time_##str##_p = calculate(&tp_start, &tp_end);} while(0)
#define TIMER(str) do {clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_##str);} while(0)
#define CALCTIME(str) do {time_##str##_p = calculate(&tp_start, &tp_end);} while(0)

// Default dictionary
#define DICTIONARY "dictionaries/large"

// Prototype
double calculate(const struct timespec *b, const struct timespec *a);

int main(int argc, char *argv[])
{
    // Check for correct number of args
    if (argc != 3 && argc != 4)
    {
        fprintf(stderr, "Usage: ./speller2 <NUM_ITERATIONS> [DICTIONARY] text\n");
        return 1;
    }

    // Structures for timing data
    struct timespec tm_start, tm_end, tp_start, tp_end;

    // Benchmarks
    double time_load_m = 0.0, time_load_p = 0.0,
           time_check_m = 0.0, time_check_p = 0.0,
           time_size_m = 0.0, time_size_p = 0.0,
           time_unload_m = 0.0, time_unload_p = 0.0;

    // No of times to repeat the spellcheck
    int iters = strtol(argv[1], NULL, 0);
    if (iters < 1 || iters > 100000) {
        fprintf(stderr, "`num_iters` must be from 1 to 100,000 inclusive");
        return 1;
    }

    // Determine dictionary to use
    char *dictionary = (argc == 4) ? argv[2] : DICTIONARY;

    // Load dictionary
    TIMER(start);
    bool loaded = load(dictionary);
    TIMER(end);

    // Exit if dictionary not loaded
    if (!loaded)
    {
        fprintf(stderr, "Could not load %s.\n", dictionary);
        return 1;
    }

    // Calculate time to load dictionary
    CALCTIME(load);

    // Try to open text
    char *text = (argc == 4) ? argv[3] : argv[2];
    FILE *file = fopen(text, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open %s.\n", text);
        unload();
        return 1;
    }

    // Skips printing misspellings (for more accurate timing)
    // printf("\nTo achieve more accurate timing, this version of 'speller' does not print misspellings\n");

    // Prepare to spell-check
    int index = 0, misspellings = 0, words = 0;
    char word[LENGTH + 1];
    char *allwords = malloc(1 << 23);  // 8388608 bytes, sufficient for all provided texts
    char *word_ptr = allwords;
    char *wordlen_plus1 = malloc(1 << 21);  // stores the length of each word (space for 2097152 words)

    // Load spell-checkable words from text into memory.
    char c;
    while (fread(&c, sizeof(char), 1, file))
    {
        // Allow only alphabetical characters and apostrophes
        if (isalpha(c) || (c == '\'' && index > 0))
        {
            // Append character to word
            word[index] = c;
            index++;

            // Ignore alphabetical strings too long to be words
            if (index > LENGTH)
            {
                // Consume remainder of alphabetical string
                while (fread(&c, sizeof(char), 1, file) && isalpha(c));

                // Prepare for new word
                index = 0;
            }
        }

        // Ignore words with numbers (like MS Word can)
        else if (isdigit(c))
        {
            // Consume remainder of alphanumeric string
            while (fread(&c, sizeof(char), 1, file) && isalnum(c));

            // Prepare for new word
            index = 0;
        }

        // We must have found a whole word
        else if (index > 0)
        {
            // Terminate current word
            word[index] = '\0';

            // Store word length in `wordlen` array
            wordlen_plus1[words] = index + 1;

            // copy word to 'text' list of words
            strcpy(word_ptr, word);

            // Prepare for next word
            word_ptr += wordlen_plus1[words];
            words++;
            index = 0;
        }
    }

    // Check whether there was an error
    if (ferror(file))
    {
        fclose(file);
        fprintf(stderr, "Error reading %s.\n", text);
        unload();
        return 1;
    }

    // Spellcheck entire text in one go (for greater timing accuracy)
    TIMER(start);
    for (int k = 0; k < iters; k++) {
        word_ptr = allwords;
        misspellings = 0;
        for (int i = 0; i < words; i++) {
            misspellings += !check(word_ptr);
            word_ptr += wordlen_plus1[i];
        }
    }
    TIMER(end);

    CALCTIME(check);

    // Close text
    fclose(file);

    // Determine dictionary's size
    TIMER(start);
    unsigned int n = size();
    TIMER(end);

    // Calculate time to determine dictionary's size
    CALCTIME(size);
    
    // Unload dictionary
    TIMER(start);
    bool unloaded = unload();
    TIMER(end);

    // Abort if dictionary not unloaded
    if (!unloaded)
    {
        fprintf(stderr, "Could not unload %s.\n", dictionary);
        return 1;
    }

    // Calculate time to unload dictionary
    CALCTIME(unload);

    // Report benchmarks
    // printf("\nWORDS MISSPELLED:     %d\n", misspellings);
    // printf("WORDS IN DICTIONARY:  %d\n", n);
    // printf("WORDS IN TEXT:        %d\n", words);
    // printf("TIME IN load:         %.4f\n", time_load);
    // printf("TIME IN check:        %.4f\n", time_check);
    // printf("TIME IN size:         %.4f\n", time_size);
    // printf("TIME IN unload:       %.4f\n", time_unload);
    // printf("TIME IN TOTAL:        %.4f\n\n", time_load + time_check + time_size + time_unload);
    printf("[OUTPUT]\n"
           "[%i, %i, %i]\n"
        //    "[%.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f]\n",
           "[%.7f, %.7f, %.7f, %.7f, %.7f]\n",
            misspellings, n, words,
            // time_load_m, time_load_p,
            // time_check_m, time_check_p,
            // time_size_m, time_size_p,
            // time_unload_m, time_unload_p,
            // time_load_m + time_check_m + time_size_m + time_unload_m,
            // time_load_p + time_check_p + time_size_p + time_unload_p
            time_load_p,
            time_check_p,
            time_size_p,
            time_unload_p,
            time_check_p + time_size_p + time_unload_p
    );

    // Success
    return 0;
}

// Returns number of seconds between b and a
double calculate(const struct timespec *b, const struct timespec *a)
{
    if (b == NULL || a == NULL)
    {
        return 0.0;
    }
    else
    {
        return (double) ((a->tv_sec * 100000000LL + a->tv_nsec) - (b->tv_sec * 100000000LL + b->tv_nsec)) / 1000000000.0;
    }
}