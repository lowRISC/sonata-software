#!/bin/sh

program_filename=${1%.*}

cat pgrm.bin $1 > ${program_filename}_rp2040.bin

./uf2conv-ext.py ${program_filename}_rp2040.bin --base 0x10300000 --convert -f \
  0xe48bff56 --output ${program_filename}_rp2040.uf2 --force_bin
