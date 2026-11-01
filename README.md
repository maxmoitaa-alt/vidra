# VIDRA

A modern, experimental Luau scripting environment for Linux desktops.

VIDRA combines a local Luau runtime, a native C++ desktop application, and an expressive interface. The current runtime is local: it does not inject into Roblox processes or bypass Roblox protections.

## Repository layout

The application source is in [`vidra/`](vidra/). The repository-level license is GNU GPL-3.0.

## Build on Linux

```bash
git clone https://github.com/maxmoitaa-alt/vidra.git
cd vidra/vidra
./build.sh --no-install
```

The build downloads Luau through CMake FetchContent. Install the development packages listed in [the project README](vidra/README.md) before building.

## Documentation

- [Build instructions and project details](vidra/README.md)
- [Known issues and current limitations](vidra/KNOWN_ISSUES.md)
- [GNU GPL v3 license](LICENSE)

VIDRA is experimental. Features and compatibility may change.
