# VIDRA 1.0.2

An experimental native Luau scripting environment for Linux desktops, built with C++20 and a custom desktop interface.

## Build

From this directory:

```bash
./build.sh --no-install
```

The build script configures the project with CMake and Ninja, builds the application, and runs the test suite. Use `./build.sh` to install the application under `/usr` (sudo may be requested).

## Dependencies

- CMake 3.28 or newer
- Ninja
- Clang / Clang++
- Python 3
- pkg-config
- X11 development headers: X11, Xext, Xfixes, Xi
- FreeType and HarfBuzz development headers
- OpenGL development headers and libraries
- Vulkan development headers are optional

Luau 0.653 is fetched automatically by CMake from its upstream Git repository.

## Runtime scope

VIDRA currently executes Luau locally in its own runtime. The `game` and `workspace` globals are placeholders; this is not Roblox process injection, and it does not implement Roblox APIs, anti-cheat bypasses, or ban avoidance.

## Project status

This is an early experimental project. Review [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for limitations and incomplete features.

The repository is licensed under GNU GPL-3.0; see the repository-root `LICENSE`.
