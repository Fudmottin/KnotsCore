# KnotsCore

Compare Bitcoin Core and Bitcoin Knots to identify meaningful code differences.

## Goal
Focus on functional changes between the two repositories using Git tags, ignoring cosmetic changes like formatting or whitespace.

## What the code does
- Uses libgit2 to iterate over tagged commits.
- Compares files between tags to detect modifications.
- Filters out differences that do not affect functionality (e.g., line endings or whitespace).

## Dependencies
- C++20 compiler
- libgit2
- Git
- CMake

# Build KnotsCore
```mkdir build && cd build
cmake ..
make
```

# Compare specific tags
./build/src/KnotesCore v29.1 v29.1.knots20250903

I have core and knots as submodules to this project. You will need to fetch them
recursively if cloning this repository doesn't do it for you.

My git experience is fairly minimul. I didn't even know about libgit2 until
working on this project. It's possible that a simple bash script would be
a better tool for discovering the differences between Knots and Core. I
like C++, so there you go.

