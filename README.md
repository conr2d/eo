# Eo

Eo provides syntactic sugars for C++ developers to write Go-like code.

Eo is not recommended for peformance-critical use case. This aims at migrating existing codebase from Go to C++ with minimum changes.

Eo is under active development and its APIs are not stable.

[Eo](https://en.wiktionary.org/wiki/eo#Latin) means "I **go**" in Latin and its pronunciation is ["Ay-Oh"](https://youtu.be/lkbP5OPQhdQ) like what Freddie Mercury shouted at Live Aid.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

## Requirements

Build dependencies will be installed by [Conan](https://github.com/conan-io/conan) package manager.

- C++20 (GCC 11, Clang 10)
- Boost 1.78 (On MacOS, use [brew](https://brew.sh/) to install boost)
- fmt
- scope-lite
