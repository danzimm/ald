#!/usr/bin/env bash
FILES=$(find . -name '*.cpp' -o -name '*.h')
clang-format -i --verbose $FILES
