// Implements a spell-checker
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

// #include "benchmark_dictionary.h"
#include "dictionary.h"

// Undefine any definitions
#undef calculate

// This speller's name
#define SPELLER "speller4"

// Define useful macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define TIMER(str) do {clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_##str);} while(0)
#define TIME(process, instructions) TIMER(start); instructions TIMER(end); CALCTIME(process);
#define CALCTIME(str) do {double t = calculate(&tp_start, &tp_end); tm.str += t; tm.str##_min = MIN(tm.str##_min, t);} while(0)
#define USAGE "[ERROR] Usage: ./"SPELLER" [-i num] [-d dict] [-s signature] text\n" 

// Default dictionary
#define DICTIONARY "dictionaries/large"

// Struct for timing data, per text
typedef struct {
    char *filename;
    int misspellings;
    int dict_size;
    int words;
    double load;
    double check;
    double size;
    double unload;
    double load_min;
    double check_min;
    double size_min;
    double unload_min;
} texttime;


// Prototype
texttime check_text(char *dictionary, char *text, int iters);
double calculate(const struct timespec *b, const struct timespec *a);
void print_texttime(texttime tm, char *signature);
void check_textpath(char *textpath, char *dictionary, int iters, char *signature);


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

    char **textpaths = argv + optind;
    int num_textpaths = argc - optind;

    // Check if textpaths not provided
    if (num_textpaths == 0) {
        fprintf(stderr, USAGE);
        return 1;
    }

    for (int i = 0; i < num_textpaths; i++)
        check_textpath(textpaths[i], dictionary, iters, signature);
}

void check_textpath(char *textpath, char *dictionary, int iters, char *signature) {
    struct stat sb;

    if (stat(textpath, &sb) != 0) {
        fprintf(stderr, "Invalid path: %s", textpath);
        exit(1);
    }

    if (S_ISREG(sb.st_mode)) {
        if (strcmp(textpath + strlen(textpath) - 4, ".txt") == 0) {
            texttime tm = check_text(textpath, dictionary, iters);
            print_texttime(tm, signature);
        }
    } else if (S_ISDIR(sb.st_mode)) {
        DIR *dir = opendir(textpath);
        if (!dir) {
            fprintf(stderr, "[ERROR] Unable to open folder %s", textpath);
            exit(1);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char newpath[strlen(textpath) + strlen(entry->d_name) + 2];
            snprintf(newpath, sizeof(newpath), "%s/%s", textpath, entry->d_name);

            check_textpath(newpath, dictionary, iters, signature);
        }
    }
}

void print_texttime(texttime tm, char *signature) {
    // [OUT:signature]misspellings, n, words, l, c, s, u, total, l_min, c_min, s_min, u_min, total_min[signature]
    printf("[%s%s%s]%i, %i, %i, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, %.7f\n",
        tm.filename,
        (*signature ? ":" : ""), signature,
        tm.misspellings, tm.dict_size, tm.words,
        tm.load,
        tm.check,
        tm.size,
        tm.unload,
        tm.load + tm.check + tm.size + tm.unload,
        tm.load_min,
        tm.check_min,
        tm.size_min,
        tm.unload_min,
        tm.load_min + tm.check_min + tm.size_min + tm.unload_min
    );
}

texttime check_text(char *text, char *dictionary, int iters) {
    // Structures for timing data
    struct timespec tm_start, tm_end, tp_start, tp_end;

    texttime tm = {text, .load_min = 999999.0, 999999.0, 999999.0, 999999.0};

    // In the event student uses a global accumulator for size(), repeated calls to load() may not first reset this to 0.
    // Thus, take note of the current size first.
    unsigned int prev_size = size();

    // Load dictionary
    TIME(load,
    bool loaded = load(dictionary);
    );

    // Exit if dictionary not loaded
    if (!loaded) {
        fprintf(stderr, "[ERROR] Could not load %s.\n", dictionary);
        exit(1);
    }

    // Try to open text
    FILE *file = fopen(text, "r");
    if (file == NULL) {
        fprintf(stderr, "[ERROR] Could not open %s.\n", text);
        unload();
        exit(1);
    }

    // Skips printing misspellings (for more accurate timing)
    // printf("\nTo achieve more accurate timing, this version of 'speller' does not print misspellings\n");
    // Prepare to spell-check
    int index = 0;
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
            wordlen_plus1[tm.words] = index + 1;

            // copy word to 'text' list of words
            strcpy(word_ptr, word);

            // Prepare for next word
            word_ptr += wordlen_plus1[tm.words];
            tm.words++;
            index = 0;
        }
    }

    // Check whether there was an error
    if (ferror(file)) {
        fclose(file);
        fprintf(stderr, "[ERROR] Error reading %s.\n", text);
        unload();
        exit(1);
    }

    // Spellcheck entire text in one go (for greater timing accuracy)
    for (int k = 0; k < iters; k++) {
        word_ptr = allwords;
        tm.misspellings = 0;
        TIME(check,
        for (int i = 0; i < tm.words; i++) {
            tm.misspellings += !check(word_ptr);
            word_ptr += wordlen_plus1[i];
        }
        )
    }

    // Close text
    fclose(file);

    // Determine dictionary's size
    TIME(size,
    tm.dict_size = size() - prev_size;
    )

    // Unload dictionary
    TIME(unload,
    bool unloaded = unload();
    )

    // Abort if dictionary not unloaded
    if (!unloaded) {
        fprintf(stderr, "Could not unload %s.\n", dictionary);
        exit(1);
    }

    // Time rest of loads and unloads
    for (int i = 1; i < iters; i++) {
        TIME(load, load(dictionary);)
        TIME(size, size();)
        // TODO: add check to prevent cheating?
        TIME(unload, unload();)
    }

    // Success
    free(allwords);
    free(wordlen_plus1);
    tm.load /= iters;
    tm.check /= iters;
    tm.size /= iters;
    tm.unload /= iters;
    return tm;
}

// Returns number of seconds between b and a
double calculate(const struct timespec *b, const struct timespec *a) {
    if (b == NULL || a == NULL)
        return 0.0;
    else
        return (double) ((a->tv_sec * 1000000000LL + a->tv_nsec) - (b->tv_sec * 1000000000LL + b->tv_nsec)) / 1000000000.0;
}