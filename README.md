# GitCringe

A local version control system. Like Git, but cringe.

GitCringe is an experimental, local-only Version Control System. It explores an alternative approach to traditional VCS architecture by storing the entire repository graph, index, and file blobs inside a relational database rather than relying on a flat file system structure.

While it implements standard version control concepts (commits, branches, detached HEAD, staging area), its architectural choices make it strictly a proof-of-concept.

## Installation

### Prerequisites
* A C++23 compatible compiler (GCC 13+ or Clang 16+).
* CMake (>= 3.24).
* *Note: SQLiteCpp is fetched automatically via FetchContent during the build process.*

### Build from Source

```bash
# 1. Clone the repository
git clone https://github.com/Ra1w/GitCringe.git
cd GitCringe

# 2. Generate build files
cmake -B build

# 3. Compile the project
cmake --build build
```

### Debian / Ubuntu Package
The project includes a CPack configuration to easily generate a `.deb` package. This is the recommended way to install GitCringe, as it sets up autocomplete and manual pages automatically.

```bash
cd build
cpack -G DEB
sudo dpkg -i gitcringe-1.0.0-Linux.deb
```

## Quick Start

The CLI interface mimics Git closely, making it familiar to use.

```bash
# Initialize a new repository
gitcringe init

# Create a file
echo "Hello, world!" > cringe.lol

# Add it to the index
gitcringe add .

# Commit your changes
gitcringe commit -m "initial commit"

# Check the history
gitcringe log

# Request help
gitcringe help
# Output: No help! Cry about it.
```

### Direct Database Access
For debugging (or cringe) purposes, GitCringe provides a direct SQL interface to the underlying repository database. You can inspect the internal tables (`commits`, `files`, `blobs`, `commit_links`, etc.) and improve your SQL skill:

```bash
gitcringe exec "SELECT id, author, message FROM commits"
```

## Disclaimer

This is an educational project and is not intended for production use. Due to its memory consumption limits during diff operations and inability to interact with remote repositories, it should not be used to track actual source code. Use at your own risk. This program is (probably) full of bugs and errors. Try to find them all!

## Authors

* Artyom Chugunov (<ArtyomCh2007@yandex.ru>)
* Mihail Afanasiev (<michele.afanasiev@gmail.com>)
