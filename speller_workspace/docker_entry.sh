#!/bin/sh

set -e

if [ "$1" = "--initialize" ]; then
#     # symbolic links to ephemeral student submission files in /speller_workspace
#     ln -sf "/$SPELLER_WS/_dictionary.c" "/$SPELLER/dictionary.c"
#     ln -sf "/$SPELLER_WS/_dictionary.h" "/$SPELLER/dictionary.h"
    
#     # symbolic link to version of speller.c we will be using
#     ln -sf "/$SPELLER_WS/$SPELLER_C_FILENAME" "/$SPELLER/speller.c"

#     # compile benchmark version of dictionary.c -- we only need to do this once at initialization
#     ln -sf "/$SPELLER_WS/$DICTIONARY_H_FILENAME" "/$SPELLER_WS/_dictionary.h"
#     ln -sf "/$SPELLER_WS/$DICTIONARY_C_BENCHMARK_FILENAME" "/$SPELLER/$DICTIONARY_C_BENCHMARK_FILENAME"

    echo "Compiling benchmark object file..."

    # compile benchmark version of dictionary.c -- we only need to do this once at initialization
    clang   -ggdb3 -gdwarf-4 -O0 -Qunused-arguments -std=c11 -Wall -Werror              \
            -Wextra -Wno-gnu-folding-constant -Wno-sign-compare -Wno-unused-parameter   \
            -Wno-unused-variable -Wshadow -c -o "/$SPELLER_WS/$BENCHMARK_BASENAME.o" "/$SPELLER_WS/$BENCHMARK_BASENAME.c"

    echo "Successfully compiled benchmark executable"
elif [ "$1" = "--compile-submission" ]; then

    cd /$SPELLER

    echo "Compiling speller..."

    # Compile submission dictionary.c
    clang   -ggdb3 -gdwarf-4 -O0 -Qunused-arguments -std=c11 -Wall -Werror              \
            -Wextra -Wno-gnu-folding-constant -Wno-sign-compare -Wno-unused-parameter   \
            -Wno-unused-variable -Wshadow -c -o dictionary.o dictionary.c

    # Compile speller
    clang   -ggdb3 -gdwarf-4 -O0 -Qunused-arguments -std=c11 -Wall -Werror              \
            -Wextra -Wno-gnu-folding-constant -Wno-sign-compare -Wno-unused-parameter   \
            -Wno-unused-variable -Wshadow -c -o $SPELLER_BASENAME.o $SPELLER_BASENAME.c

    # Link
    clang   -ggdb3 -gdwarf-4 -O0 -Qunused-arguments -std=c11 -Wall -Werror              \
            -Wextra -Wno-gnu-folding-constant -Wno-sign-compare -Wno-unused-parameter   \
            -Wno-unused-variable -Wshadow -o $SPELLER_BASENAME $SPELLER_BASENAME.o dictionary.o $BENCHMARK_BASENAME.o -lm

    rm *.c *.o *.h

fi