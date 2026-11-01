# Known issues

This file describes limitations confirmed by source inspection. It is not a claim that every possible defect has been found.

## Build and graphics

- CMake 4.x needs a policy compatibility floor for the older CMake minimum declared by the pinned Luau 0.653 dependency. The project now supplies `CMAKE_POLICY_VERSION_MINIMUM=3.5` for CMake 4.x and the Linux build script passes the same setting.
- The Vulkan backend expects precompiled SPIR-V files (`scene.vert.spv`, `scene.frag.spv`, `blur.vert.spv`, and `blur.frag.spv`). The current build does not generate those files. Vulkan initialization can therefore fail and the application may fall back to OpenGL.
- The OpenGL renderer requires a working X11/GLX display and an OpenGL 3.3-capable driver.

## Runtime

- The `game` and `workspace` globals are empty placeholder tables. Roblox APIs are not implemented.
- `task.wait` currently returns its argument instead of yielding, and `task.spawn` runs the callback synchronously.
- The configured memory and wall-clock limits are not currently enforced. The instruction-count interrupt is the only implemented execution limit.
- `Vm::stop()` is not yet wired into the VM interrupt callback.

## Interface

- Some screens and controls are prototypes. For example, the console clear button currently has no clearing action, and settings controls are not fully wired.
- Window-state restoration is incomplete.

## Verification status

The repository now includes a GitHub Actions build-and-test workflow. A release package should be treated as validated only after that workflow completes successfully for the corresponding tag.
