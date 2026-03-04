#!/bin/sh

set -e

# copy Makefile to /submission
cp /speller/Makefile /submission
cp /speller/speller2.c /submission
cp /speller/dictionary_speller50.c /submission

cd /submission
make speller2 -B
make speller50_recomp -B

cd /speller

for f in texts/*; do
    /submission/speller2 1 $f
    /submission/speller50_recomp 1 $f
done
