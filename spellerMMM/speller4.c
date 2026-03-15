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
#define TIMEDIFF(b, a) ((double) ((a.tv_sec * 1000000000LL + a.tv_nsec) - (b.tv_sec * 1000000000LL + b.tv_nsec)) / 1000000000.0)
#define TIMER(start_or_end) do {clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_##start_or_end);} while(0)
#define TIME(process, suf, instructions) TIMER(start); instructions(suf) TIMER(end); CALCTIME(process, suf);
#define CALCTIME(str, suf) do {double t = TIMEDIFF(tp_start, tp_end); tm##suf.str += t; tm##suf.str##_min = MIN(tm##suf.str##_min, t);} while(0)
#define USAGE "[ERROR] Usage: ./"SPELLER" [-i num] [-d dict] [-v(erbose)] [-s signature] textpath1 [textpath2...]\n" 

#define FOR_LOAD_CHECK_SIZE_UNLOAD(_code_, bench) _code_(load, bench); _code_(check, bench); _code_(size, bench); _code_(unload, bench);

// Default dictionary
#define DICTIONARY "dictionaries/large"

// Structs and Variables
typedef struct {
    int misspellings, dict_size, words;
    double load, check, size, unload;
    double load_min, check_min, size_min, unload_min;
} texttime;

static char *signature, *dictionary;
static int iters, verbose;

// Prototypes
static void check_text(char *filename, texttime *total_tm, texttime *total_tm_BENCH);
static void print_texttime(char *filename, texttime tm);
static void check_textpath(char *textpath, texttime *total_tm, texttime *total_tm_BENCH);
static void clear_table();

static int prog_main(int argc, char *argv[]) {

    if (table == NULL) {
        printf("table[] not found in submission.\n");
    } else {
        printf("Found table[%i] in submission.\n", N);
    }

    // Turn off error messages from getopt (using our own)
    opterr = 0;

    int opt;
    iters = 1;  // default to running check once
    signature = "";
    dictionary = DICTIONARY;

    while ((opt = getopt(argc, argv, "vi:d:s:")) != -1) {
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
            case 'v':
                verbose = 1;
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

    texttime total_tm = {}, total_tm_BENCH = {};

    for (int i = 0; i < num_textpaths; i++)
        check_textpath(textpaths[i], &total_tm, &total_tm_BENCH);

    texttime final_ratio;
    #define CALCULATE_RATIOS(category, _unused_) final_ratio.category = total_tm_BENCH.category / total_tm.category; \
                                                 final_ratio.category##_min = total_tm_BENCH.category##_min / total_tm.category##_min
    FOR_LOAD_CHECK_SIZE_UNLOAD(CALCULATE_RATIOS, )

    print_texttime("TOTAL", total_tm);
    print_texttime("TOTAL BENCH", total_tm_BENCH);

    printf("[%s]%.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f\n",
        (*signature ? signature : "OUT"),
        final_ratio.load, final_ratio.check, final_ratio.size, final_ratio.unload,
        (total_tm_BENCH.load + total_tm_BENCH.check + total_tm_BENCH.size + total_tm_BENCH.unload) / (total_tm.load + total_tm.check + total_tm.size + total_tm.unload),
        final_ratio.load_min, final_ratio.check_min, final_ratio.size_min, final_ratio.unload_min,
        (total_tm_BENCH.load_min + total_tm_BENCH.check_min + total_tm_BENCH.size_min + total_tm_BENCH.unload_min) / (total_tm.load_min + total_tm.check_min + total_tm.size_min + total_tm.unload_min)
    );

    return 0;
}

void print_texttime(char *filename, texttime tm) {
    // [OUT:signature]misspellings, n, words, l, c, s, u, total, l_min, c_min, s_min, u_min, total_min[signature]
    printf("[%s%s%s]%i, %i, %i, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f\n",
        filename,
        (*signature ? ":" : ""), signature,
        tm.misspellings, tm.dict_size, tm.words,
        tm.load, tm.check, tm.size, tm.unload, tm.load + tm.check + tm.size + tm.unload,
        tm.load_min, tm.check_min, tm.size_min, tm.unload_min, tm.load_min + tm.check_min + tm.size_min + tm.unload_min
    );
}

void check_textpath(char *textpath, texttime *total_tm, texttime *total_tm_BENCH) {
    struct stat sb;

    if (stat(textpath, &sb) != 0) {
        fprintf(stderr, "[ERROR] Invalid path: %s", textpath);
        exit(1);
    }

    if (S_ISREG(sb.st_mode)) {
        if (strcmp(textpath + strlen(textpath) - 4, ".txt") == 0) {
            check_text(textpath, total_tm, total_tm_BENCH);
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

            check_textpath(newpath, total_tm, total_tm_BENCH);
        }
    }
}

void check_text(char *filename, texttime *total_tm, texttime *total_tm_BENCH) {
    // Structures for timing data
    struct timespec tm_start, tm_end, tp_start, tp_end;

    texttime tm = {.load_min = 999999.0, 999999.0, 999999.0, 999999.0};
    texttime tm_BENCH = tm;
    
    // In the event student uses a global accumulator for size(), repeated calls to load() may not first reset this to 0.
    // Thus, take note of the dictionary size immediately after the first load().
    int prev_dict_size = size(), prev_dict_size_BENCH = size_BENCH();
    
    // Load dictionary
    bool loaded;
    TIME(load,, LOAD)
    // Exit if dictionary not loaded
    if (!loaded) {
        fprintf(stderr, "[ERROR] Could not load %s.\n", dictionary);
        exit(1);
    }

    TIME(load, _BENCH, LOAD)
    
    // Calculate dict size here, immediately after 1 load().
    // Also give benefit of doubt if prev_dict_size == size()
    tm.dict_size = size() - prev_dict_size;
    if (tm.dict_size == 0) tm.dict_size = prev_dict_size;
    tm_BENCH.dict_size = size_BENCH() - prev_dict_size_BENCH;
    if (tm_BENCH.dict_size == 0) tm_BENCH.dict_size = prev_dict_size_BENCH;

    // Check that student dictionary contains same number of words as dictionary50
    if (tm.dict_size != tm_BENCH.dict_size) {
        fprintf(stderr, "[ERROR] Student's dictionary contains %i words instead of %i.\n", tm.dict_size, tm_BENCH.dict_size);
        exit(1);
    }

    // Try to open text
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "[ERROR] Could not open %s.\n", filename);
        unload();
        exit(1);
    }
    
    // Skips printing misspellings (for more accurate timing)
    // Prepare to spell-check
    int words = 0;
    char *allwords = malloc(1 << 23);  // 8388608 bytes, sufficient for all provided texts
    char *word_ptr = allwords, *letter_ptr = allwords;
    char *wordlen_plus1 = malloc(1 << 21);  // stores the length of each word (space for 2097152 words)
    
    // Load spell-checkable words from text into memory.
    while (fread(letter_ptr, sizeof(char), 1, file)) {
        // Only letters and apostrophes are allowed in words
        if (isalpha(*letter_ptr) || (*letter_ptr == '\'' && letter_ptr > word_ptr)) {
            // Ignore words longer than max length
            if (letter_ptr - word_ptr > LENGTH) {
                while (fread(letter_ptr, sizeof(char), 1, file) && isalpha(*letter_ptr));
                letter_ptr = word_ptr;
            } else {
                letter_ptr++;
            }
        } else if (isdigit(*letter_ptr)) {
            // Ignore words with numbers (like MS Word can). Consume remainder of alphanumeric string
            while (fread(letter_ptr, sizeof(char), 1, file) && isalnum(*letter_ptr));
            letter_ptr = word_ptr;
        } else if (letter_ptr > word_ptr) {
            // Reached the end of a word
            *letter_ptr = '\0';
            // Store word length in `wordlen` array
            letter_ptr++;
            wordlen_plus1[words++] = letter_ptr - word_ptr;
            word_ptr = letter_ptr;
        }
    }
    tm.words = tm_BENCH.words = words;
    
    // Check whether there was an error
    if (ferror(file)) {
        fclose(file);
        fprintf(stderr, "[ERROR] Error reading %s.\n", filename);
        unload();
        exit(1);
    }
    
    // Spellcheck entire text in one go (for greater timing accuracy)
    for (int k = 0; k < iters; k++) {
        TIME(check, , CHECK)
        TIME(check, _BENCH, CHECK)

        // Check that student dictionary contains same number of words as dictionary50
        if (tm.misspellings != tm_BENCH.misspellings) {
            fprintf(stderr, "[ERROR] Student reported %i misspellings in \"%s\" instead of %i.\n", tm.misspellings, filename, tm_BENCH.misspellings);
            exit(1);
        }
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
    
    #define DIVIDE_BY_ITERS(category, bench) tm##bench.category /= iters
    
    FOR_LOAD_CHECK_SIZE_UNLOAD(DIVIDE_BY_ITERS, )
    FOR_LOAD_CHECK_SIZE_UNLOAD(DIVIDE_BY_ITERS, _BENCH)

    if (verbose) {
        print_texttime(filename, tm);
        print_texttime(filename, tm_BENCH);
    }

    #define ADD_TO_TOTAL(category, bench) total_tm##bench->category += tm##bench.category; \
    total_tm##bench->category##_min += tm##bench.category##_min
    FOR_LOAD_CHECK_SIZE_UNLOAD(ADD_TO_TOTAL, )
    FOR_LOAD_CHECK_SIZE_UNLOAD(ADD_TO_TOTAL, _BENCH)

}

// Clears student's `table[N]`, if it exists
void clear_table() {
    if (&table) memset(table, 0, N * sizeof *table);
}

int main(int argc, char *argv[]) {
    return prog_main(argc, argv);
}