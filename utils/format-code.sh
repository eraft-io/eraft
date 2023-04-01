#!/bin/bash

find src -type f \( -name '*.h' -or -name '*.hpp' -or -name '*.cpp' -or -name '*.c' -or -name '*.cc' \) -print | xargs clang-format -style=file --sort-includes -i
