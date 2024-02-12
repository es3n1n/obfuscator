#!/usr/bin/env sh
cmake -S . -B __clang_tidy_build -DCMAKE_C_COMPILER=clang-17 -DCMAKE_CXX_COMPILER=clang++-17 -DCMAKE_EXPORT_COMPILE_COMMANDS=1
python3 scripts/run_clang_tidy.py -clang-tidy-binary clang-tidy-17 -p __clang_tidy_build -j 25 -extra-arg="-std=c++23" -extra-arg="-stdlib=libc++" -q
