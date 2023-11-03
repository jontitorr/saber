# Saber

A WIP Discord bot written in C++

## Features

- Low memory footprint
- Commands system, with commands that can be loaded at runtime

## Getting Started

### Prerequisites

- [CMake](https://cmake.org/download/) (version >= 3.16)
- Compiler with C++17 support, i.e. MSVC, GCC, Clang

### Installing

This application uses [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) to manage dependencies. It is an amazing package manager for CMake projects and allows us to install the entire library using the following commands:

```bash
  git clone https://www.github.com/xminent/saber
  cd saber
  cmake -S . -B build
  cmake --build build --target install
```

## Contributing

Contributions, issues and feature requests are welcome. After cloning and setting up project locally, you can just submit
a PR to this repo and it will be deployed once it's accepted.

Take full advantage of the [.clang-format](.clang-format) file located in the root of the project to ensure that your code is properly formatted.

## Dependencies

### Third party Dependencies

- [ekizu](https://github.com/xminent/ekizu) - C++ library for Discord API
- [spdlog](https://github.com/gabime/spdlog) - C++ logging library

## License

[MIT](https://choosealicense.com/licenses/mit/)