# Hanami

Hanami is a HTML viewer that I'm currently working on just for fun.
While this was definitely inspired by [Ladybird](https://ladybird.org/) I've deliberately
avoided looking at Ladybirds source, as I want this implementation to be *mine*.

While this repository is public, it's still a hobby project and I will most likely break things
frequently.

## Building

Currently Hanami only builds on Linux with a Wayland compositor.

### Required Packages
- Cairo (Rendering)
- simdjson
- libxkbcommon

### Tested Compilers
- Clang 20.1.6

### Cloning
```shell
git clone --recursive https://github.com/peter1745/Hanami.git && cd Hanami
```

Add these options: `--depth 1 --shallow-submodules` if you want to clone without the history / submodule history.

### Build
```shell
cmake -S . -B build
cmake --build build
```
