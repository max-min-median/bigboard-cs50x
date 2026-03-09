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

#include "dictionary50.h"
#include "dictionary.h"

// Undefine any definitions
#undef calculate

// If the submission uses the usual `table[N]`, we may need to null it out for them (to prevent load from blowing up)
typedef struct node {
    char word[45 + 1];
    struct node *next;
} node;

extern node *table[] __attribute__((weak));
extern const unsigned int N __attribute__((weak));

// This speller's name
#define SPELLER "speller4"

// Define code for load, check, size, unload
#define LOAD(suf) loaded = load##suf(dictionary);

#define CHECK(suf) word_ptr = allwords; \
                   tm##suf.misspellings = 0; \
                   for (int i = 0; i < tm##suf.words; i++) { \
                       tm##suf.misspellings += !check##suf(word_ptr); \
                       word_ptr += wordlen_plus1[i]; \
                   }

#define SIZE(suf) size##suf();

#define UNLOAD(suf) unloaded = unload##suf();


// Define useful macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define TIMER(str) do {clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_##str);} while(0)
#define TIME(process, suf, instructions) TIMER(start); instructions(suf) TIMER(end); CALCTIME(process, suf);
#define CALCTIME(str, suf) do {double t = calculate(&tp_start, &tp_end); tm##suf.str += t; tm##suf.str##_min = MIN(tm##suf.str##_min, t);} while(0)
#define USAGE "[ERROR] Usage: ./"SPELLER" [-i num] [-d dict] [-s signature] textpath1 [textpath2...]\n" 

// Default dictionary
#define DICTIONARY "dictionaries/large"

// Struct for timing data, per text
typedef struct {
    char *filename;
    int misspellings, dict_size, words;
    double load, check, size, unload;
    double load_min, check_min, size_min, unload_min;
} texttime;


// Prototypes
void check_text(char *dictionary, char *text, int iters, char *signature);
double calculate(const struct timespec *b, const struct timespec *a);
void print_texttime(texttime tm, char *signature);
void check_textpath(char *textpath, char *dictionary, int iters, char *signature);
void clear_table();


int main(int argc, char *argv[]) {

    if (table == NULL) {
        printf("table[] not found in submission.\n");
    } else {
        printf("Found table[%i] in submission.\n", N);
    }

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
                    fprintf(stderr, "[ERROR] -i: number of iterations must be a positive integer from 1 through 100,000\n");
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
        fprintf(stderr, "[ERROR] Invalid path: %s", textpath);
        exit(1);
    }

    if (S_ISREG(sb.st_mode)) {
        if (strcmp(textpath + strlen(textpath) - 4, ".txt") == 0) {
            check_text(textpath, dictionary, iters, signature);
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

void check_text(char *text, char *dictionary, int iters, char *signature) {
    // Structures for timing data
    struct timespec tm_start, tm_end, tp_start, tp_end;

    texttime tm = {text, .load_min = 999999.0, 999999.0, 999999.0, 999999.0};
    texttime tm_BENCH = tm;

    // In the event student uses a global accumulator for size(), repeated calls to load() may not first reset this to 0.
    // Thus, take note of the dictionary size immediately after the first load().
    int prev_dict_size = size(), prev_dict_size_BENCH = size_BENCH();

    // Load dictionary
    bool loaded;
    TIME(load,, LOAD)
    TIME(load, _BENCH, LOAD)

    // Calculate dict size here, immediately after 1 load().
    // Also give benefit of doubt if prev_dict_size == size()
    tm.dict_size = size() - prev_dict_size;
    if (tm.dict_size == 0) tm.dict_size = prev_dict_size;
    tm_BENCH.dict_size = size_BENCH() - prev_dict_size_BENCH;
    if (tm_BENCH.dict_size == 0) tm_BENCH.dict_size = prev_dict_size_BENCH;

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
    // Prepare to spell-check
    int index = 0, words = 0;
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
    tm.words = tm_BENCH.words = words;

    // Check whether there was an error
    if (ferror(file)) {
        fclose(file);
        fprintf(stderr, "[ERROR] Error reading %s.\n", text);
        unload();
        exit(1);
    }

    // Spellcheck entire text in one go (for greater timing accuracy)
    for (int k = 0; k < iters; k++) {
        TIME(check, , CHECK)
        TIME(check, _BENCH, CHECK)
    }

    // Close text
    fclose(file);

    // Unload dictionary
    bool unloaded;
    TIME(unload, , UNLOAD)
    clear_table();

    // Abort if dictionary not unloaded
    if (!unloaded) {
        fprintf(stderr, "[ERROR] Could not unload %s.\n", dictionary);
        exit(1);
    }
    TIME(unload, _BENCH, UNLOAD)

    // Time size once, because by this time we've not taken its timing
    TIME(size, , SIZE)
    TIME(size, _BENCH, SIZE)

    // Time rest of loads and unloads
    for (int i = 1; i < iters; i++) {
        TIME(load, , LOAD)
        TIME(load, _BENCH, LOAD)
        // TODO: add check to prevent cheating?
        TIME(size, , SIZE)
        TIME(size, _BENCH, SIZE)
        TIME(unload, , UNLOAD)
        clear_table();
        TIME(unload, _BENCH, UNLOAD)
    }

    // Success
    free(allwords);
    free(wordlen_plus1);
    tm.load /= iters;
    tm.check /= iters;
    tm.size /= iters;
    tm.unload /= iters;
    tm_BENCH.load /= iters;
    tm_BENCH.check /= iters;
    tm_BENCH.size /= iters;
    tm_BENCH.unload /= iters;
    print_texttime(tm, signature);
    print_texttime(tm_BENCH, signature);
}

// Clears student's `table[N]`, if it exists
void clear_table() {
    if (&table) memset(table, 0, N * sizeof *table);
}

// Returns number of seconds between b and a
double calculate(const struct timespec *b, const struct timespec *a) {
    if (b == NULL || a == NULL)
        return 0.0;
    else
        return (double) ((a->tv_sec * 1000000000LL + a->tv_nsec) - (b->tv_sec * 1000000000LL + b->tv_nsec)) / 1000000000.0;
}