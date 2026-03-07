// Implements a spell-checker
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "dictionary.h"

// Undefine any definitions
#undef calculate

// filename
#define SPELLER "speller4"

// Define useful macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define TIMER(str) do {clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_##str);} while(0)
#define TIME(process, instructions) TIMER(start); instructions TIMER(end); CALCTIME(process);
#define CALCTIME(str) do {double tm = calculate(&tp_start, &tp_end); time_##str += tm; time_##str##_min = MIN(time_##str##_min, tm);} while(0)
#define USAGE "[ERROR] Usage: ./"SPELLER" [-i num] [-d dict] [-s signature] texts...\n" 

// Default dictionary
#define DICTIONARY "dictionaries/large"

// Prototype
double calculate(const struct timespec *b, const struct timespec *a);


int main(int argc, char *argv[]) {

    // Turn off error messages from getopt (using our own)
    opterr = 0;

    int opt;
    int iters = 1;  // default to running check once
    char *signature = "";
    char *dictionary = DICTIONARY;

    while ((opt = getopt(argc, argv, "i:d:s:")) != -1) {
        int num;
        switch (opt) {
            case 'i':
                num = atoi(optarg);
                if (num < 1 || num > 100000) {
                    fprintf(stderr, "-i: number of iterations must be a positive integer from 1 through 100,000\n");
                    return 1;
                }
                iters = num;
                break;

            case 'd':
                dictionary = optarg;
                break;

            case 's':
                signature = optarg;
                break;

            default:
                fprintf(stderr, USAGE);
                return 1;
        }
    }

    // Check for incorrect number of arguments
    if (argc - optind != 1) {
        fprintf(stderr, USAGE);
        return 1;
    }
    char *text = argv[optind];
    // Structures for timing data
    struct timespec tm_start, tm_end, tp_start, tp_end;

    // Benchmarks
    double time_load_min = 99999999.0, time_load = 0.0,
           time_check_min = 99999999.0, time_check = 0.0,
           time_size_min = 99999999.0, time_size = 0.0,
           time_unload_min = 99999999.0, time_unload = 0.0;

    // Load dictionary
    TIME(load,
    bool loaded = load(dictionary);
    );

    // Exit if dictionary not loaded
    if (!loaded) {
        fprintf(stderr, "[ERROR] Could not load %s.\n", dictionary);
        return 1;
    }

    // Try to open text
    FILE *file = fopen(text, "r");
    if (file == NULL) {
        fprintf(stderr, "[ERROR] Could not open %s.\n", text);
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
    while (fread(&c, sizeof(char), 1, file)) {
        // Allow only alphabetical characters and apostrophes
        if (isalpha(c) || (c == '\'' && index > 0)) {
            // Append character to word
            word[index] = c;
            index++;

            // Ignore alphabetical strings too long to be words
            if (index > LENGTH) {
                // Consume remainder of alphabetical string
                while (fread(&c, sizeof(char), 1, file) && isalpha(c));
                // Prepare for new word
                index = 0;
            }
        } else if (isdigit(c)) {
            // Ignore words with numbers (like MS Word can). Consume remainder of alphanumeric string
            while (fread(&c, sizeof(char), 1, file) && isalnum(c));
            // Prepare for new word
            index = 0;
        } else if (index > 0) {
            // We must have found a whole word. Terminate current word
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
    if (ferror(file)) {
        fclose(file);
        fprintf(stderr, "[ERROR] Error reading %s.\n", text);
        unload();
        return 1;
    }

    // Spellcheck entire text in one go (for greater timing accuracy)
    for (int k = 0; k < iters; k++) {
        word_ptr = allwords;
        misspellings = 0;
        TIME(check,
        for (int i = 0; i < words; i++) {
            misspellings += !check(word_ptr);
            word_ptr += wordlen_plus1[i];
        }
        )
    }

    // Close text
    fclose(file);

    // Determine dictionary's size
    TIME(size,
    unsigned int n = size();
    )
    
    // Unload dictionary
    TIME(unload,
    bool unloaded = unload();
    )

    // Abort if dictionary not unloaded
    if (!unloaded) {
        fprintf(stderr, "Could not unload %s.\n", dictionary);
        return 1;
    }

    // Time rest of loads and unloads
    for (int i = 1; i < iters; i++) {
        TIME(load, load(dictionary);)
        // TODO: add check to prevent cheating?
        TIME(unload, unload();)
    }

    // Report benchmarks
    // [OUT:signature]misspellings, n, words, l, c, s, u, total, l_min, c_min, s_min, u_min, total_min[signature]
    printf("[OUT%s%s]%i, %i, %i, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f\n",
        (*signature ? ":" : ""), signature,
        misspellings, n, words,
        time_load / iters,
        time_check / iters,
        time_size / iters,
        time_unload / iters,
        (time_load + time_check + time_size + time_unload) / iters,
        time_load_min,
        time_check_min,
        time_size_min,
        time_unload_min,
        time_load_min + time_check_min + time_size_min + time_unload_min
    );

    // Success
    free(allwords);
    free(wordlen_plus1);
    return 0;
}

// Returns number of seconds between b and a
double calculate(const struct timespec *b, const struct timespec *a) {
    if (b == NULL || a == NULL)
        return 0.0;
    else
        return (double) ((a->tv_sec * 1000000000LL + a->tv_nsec) - (b->tv_sec * 1000000000LL + b->tv_nsec)) / 1000000000.0;
}