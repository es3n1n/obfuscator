#!/usr/bin/env sh

objdump --disassemble --x86-asm-syntax=intel -j .s_code $1
