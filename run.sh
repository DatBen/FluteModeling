#!/usr/bin/env fish
make clean
make
rm include/flute.h
python3 create_flute.py
python3 flute.py

cd vti
./../main
