#!/bin/sh

clang -std=c++17 -Wno-everything -c lib.cpp -o vma.o -fPIC -lvulkan -lstdc++
ar rcs libvma.a vma.o
