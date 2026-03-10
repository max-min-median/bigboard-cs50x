#!/bin/sh

set -e

if [ "$1" = "--initialize" ]; then
    
    # symbolic link to version of speller.c we will be using
    ln -sf -T "/$SPELLER_WS/$SPELLER_BASENAME.c" "/$SPELLER/$SPELLER_BASENAME.c"

    echo "Compiling benchmark object file..."

    # compile benchmark version of dictionary.c -- we only need to do this once at initialization
    clang   -ggdb3 -gdwarf-4 -O0 -Qunused-arguments -std=c11 -Wall -Werror              \
            -Wextra -Wno-gnu-folding-constant -Wno-sign-compare -Wno-unused-parameter   \
            -Wno-unused-variable -Wshadow -c -o "/$SPELLER_WS/$BENCHMARK_BASENAME.o" "/$SPELLER_WS/$BENCHMARK_BASENAME.c"

    # symbolic links to main speller file, as well as 'permanent' benchmark files
    ln -sf -T "/$SPELLER_WS/$SPELLER_BASENAME.c" "/$SPELLER/$SPELLER_BASENAME.c"
    ln -sf -T "/$SPELLER_WS/$BENCHMARK_BASENAME.o" "/$SPELLER/$BENCHMARK_BASENAME.o"
    ln -sf -T "/$SPELLER_WS/$BENCHMARK_BASENAME.h" "/$SPELLER/$BENCHMARK_BASENAME.h"

    echo "Successfully compiled benchmark object file"

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

    rm dictionary.* $BENCHMARK_BASENAME.o
fi