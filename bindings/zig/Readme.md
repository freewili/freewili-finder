freewili-finder zig bindings
========

Experimental bindings for the zig language.


How to build:
=========
This expects libcfwfinder shared library to be built in the root in the build directory.

**Note:** Tested against zig 0.14.1 and 0.15.1 on linux

```
$ cmake -B ../../build -S ../../
$ cmake --build ../../build
$ zig build
```
