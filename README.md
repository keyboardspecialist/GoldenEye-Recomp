# THIS IS A WORK IN PROGRESS

# GoldenEye 007 PC Recompilation

A native PC recompilation of the Xbox 360 version of GoldenEye 007.

## A quick note

I've been getting a lot of hate over this project, so I want to address it once and move on.

This started because I love GoldenEye and wanted to see it running natively on PC. That's it. It's a hobby project that I've spent a lot of my free time working on.

Yes, AI was used during development. I'm not hiding that. AI can help write code, explain things, and speed up development, but it doesn't magically build a project on its own. Every bug still has to be tracked down, every feature still has to be implemented, and every broken build still has to be fixed by a human being.

If AI-assisted development isn't your thing, that's completely fine. But calling the project worthless because AI was involved misses the point. The goal has always been to get a game I care about running properly on PC and share that work with people who are interested.

Anyway, enough of that.

## Installation

1. Create a folder called `assets` next to the executable.
2. Put the game files inside the `assets` folder.
3. Run `GoldenEye.exe` on Windows or `./GoldenEye` on Linux.

## Features

* Native PC executable
* Keyboard and mouse support
* Online multiplayer
* Graphics settings
* Post-processing effects
* Higher FPS gameplay

### Integrated upstream fixes

* [#118](https://github.com/SunJaycy/GoldenEye-Recomp/pull/118): delayed
  dead-body fades and animated Frigate water in enhanced graphics, including
  the low speedboat camera.
* [#137](https://github.com/SunJaycy/GoldenEye-Recomp/pull/137): the pause-screen
  watch arm shows only the current costume's sleeve in enhanced and classic
  graphics. Use `--ge_sleeve_fix=false` to disable this fix.
* [#138](https://github.com/SunJaycy/GoldenEye-Recomp/pull/138): mouse movement
  enables mouse aiming; right-stick movement returns aiming to the controller,
  preserving its native crosshair behavior and auto-aim choice. Keyboard
  right-stick bindings participate in the same input selection.
* [#114](https://github.com/SunJaycy/GoldenEye-Recomp/pull/114): native XACT
  watch music, Control/Caverns elevator music, and Mission Select music after
  missions, using the existing game music banks.
* [#116](https://github.com/SunJaycy/GoldenEye-Recomp/pull/116): horizontal
  mouse aiming for the mounted tank turret while retaining keyboard driving
  and vertical aiming. Its shared music fixes are included through #114.

## Online

To play online, someone needs to run a server.

1. Open `ESC -> ONLINE`
2. Enter your username, server address, and port
3. Enable online play
4. Save and restart
5. Host or join a match

## Building

If you want to build from source you'll need:

* ReXGlue SDK 0.10.0
* CMake 3.25 or newer and Ninja
* A C++23 compiler (GCC or Clang on Linux; Clang/MSVC on Windows)
* Your own game files

### Linux (x86-64)

1. Extract the Linux AMD64 SDK into `rexglue/linux-amd64/`. It should contain
   `bin/rexglue`, `include/`, `lib/`, and `share/`. An SDK installed elsewhere can
   be selected with `-DCMAKE_PREFIX_PATH=/path/to/sdk` when configuring.
2. Put the extracted Xbox 360 game files in `assets/` at the repository root.
   The executable must be named `assets/default.xex` (Linux paths are case-sensitive).
3. Generate the recompilation, then configure and build:

   ```sh
   ./rexglue/linux-amd64/bin/rexglue codegen ge_manifest.toml
   cmake --preset linux-amd64-release
   cmake --build --preset linux-amd64-release --parallel "$(nproc --all)"
   ```

   The Linux presets use the system compiler. To select a particular compiler,
   set `CXX` before the first configure, for example
   `CXX=clang++ cmake --preset linux-amd64-release`.

4. Run with the game files from the repository:

   ```sh
   ./out/build/linux-amd64-release/GoldenEye --game_data_root="$PWD/assets"
   ```

   For a standalone install, copy `GoldenEye` and the `.so` files from the build
   directory into one folder and put `assets/` beside them. The build stages the
   SDK runtime and Xenos GPU plugin beside the executable. Linux uses Vulkan;
   install your GPU's Vulkan driver and the X11/Wayland runtime libraries.
   Keyboard/mouse input uses the SDK's SDL window events and relative mouse mode.

   Rapid duplicate presses of the same controller button in title/watch menus
   are ignored for 150 ms after an accepted press. Tune this with
   `--ge_menu_button_debounce=0.15` (seconds), or set it to `0` to disable.

   Use `--resolution=1080p` (or `--resolution=1920x1080`) to select a window/video
   mode. `--window_width=1280 --window_height=720` sets the window size independently.
   The in-game **Internal Resolution** setting controls `resolution_scale` and
   requires a restart. Scene-copy textures use the game's current raster size;
   the SDK applies the configured internal scaling to them.

   Linux Vulkan defaults to `render_target_path_vulkan="fsi"` to prevent black
   frames during weapon changes and the intro. An explicit config, environment,
   or CLI value overrides this default (for example, `--render_target_path_vulkan=fbo`).

Generated files are local build inputs and are not committed. Run codegen before
the first CMake configure; subsequent builds regenerate when its inputs change.

### GitHub Actions release builds

The **Release build** workflow builds Linux x86-64 in Release mode inside an
Ubuntu 22.04 container using GCC 14, CMake 3.31.6, and the pinned ReXGlue 0.10.0
SDK. The older container keeps the glibc minimum at 2.35 for SteamOS compatibility.

1. Create a private GitHub repository named `goldeneye-build-inputs` under the
   same owner as this repository. Create a release tagged `build-inputs` and
   upload `default.xex` as a release attachment. Only this executable is needed
   for code generation; the workflow does not need the rest of the game assets.
2. Create a fine-grained personal access token with **Contents: Read-only** access
   to that private repository. In this repository's **Settings -> Secrets and
   variables -> Actions**, save it as a repository secret named `GAME_ASSETS_TOKEN`.
   Renew the secret before building if the token expires.
3. If you use a different repository or release tag, set the optional Actions
   repository variables `GAME_ASSETS_REPOSITORY` (in `owner/repository` format)
   and `GAME_ASSETS_TAG`. They default to `<repository-owner>/goldeneye-build-inputs`
   and `build-inputs`.
4. Open **Actions -> Release build -> Run workflow** and select the branch or tag
   to build. Download the `GoldenEye-linux-amd64` artifact when the run completes.
   It contains a `.tar.gz` archive and its SHA-256 checksum; artifacts are retained
   for 14 days.

Publishing a GitHub release also runs the workflow against that release's tag
and attaches the archive and checksum to the release. Manual runs only upload
workflow artifacts.

The archive contains `GoldenEye`, its ReXGlue runtime and Xenos GPU libraries,
the optional Tracy library, bundled `libstdc++`/`libgcc` runtimes, `README.md`,
and `LICENSE`. Extract it and put your game files in an `assets/` folder beside
`GoldenEye` before running. Game inputs, generated source files, and local runtime
settings are excluded from the archive.

The workflow's Linux builds target glibc 2.35 or newer, including SteamOS 3.5 and
newer. Vulkan and X11/Wayland libraries come from the host system. For Steam Deck,
add `GoldenEye` as a non-Steam game and run it natively, with the game's `assets/`
folder beside it. The C++ runtimes are bundled to avoid depending on SteamOS's
installed libstdc++ version; glibc and GPU drivers are not bundled. On-device
gameplay still needs to be verified after the first workflow build.

To package a local release build with the same file selection:

```sh
bash .github/scripts/package-linux-release.sh out/build/linux-amd64-release out/release local
```

Local packages inherit the local compiler and system's glibc requirements;
use the workflow's Ubuntu 22.04 environment for the SteamOS compatibility baseline.

## Known Issues

* AMD GPUs may crash or fail to boot the game.
* AMD compatibility is currently being worked on.

Please report any issues you find.


## Legal

This repository does not contain any game assets, game code, ROMs, XEX files, textures, audio, or other copyrighted material.

You must provide your own game files.

## License

Released under The Unlicense.
