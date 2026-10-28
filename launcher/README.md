# Everstead Launcher

The Qt Quick launcher for Everstead. It downloads and installs game versions, keeps itself up to date, and starts the game.

## Features

- Installing, switching between and removing game versions
- Profiles that each pick a game version and their own environment variables
- A log window that shows the game's output while it runs
- The changelog of the latest release
- Self updates
- Light and dark themes, and an option to minimize while the game is running

Game versions, the changelog and launcher updates are downloaded from the release bucket, configured in `src/LauncherState.cpp`.

## Building

The launcher is built inside Docker like the game. It needs Qt 6.8 or newer and libarchive, which the toolchain image already includes.

From the repository root, build the toolchain image:

```sh
docker build --target toolchain --tag everstead-builder:latest .
```

Configure and build a Linux release:

```sh
docker run --rm --user "$(id -u):$(id -g)" --volume "$PWD:$PWD" --workdir "$PWD" everstead-builder:latest cmake --preset docker-launcher-linux-release
docker run --rm --user "$(id -u):$(id -g)" --volume "$PWD:$PWD" --workdir "$PWD" everstead-builder:latest cmake --build --preset docker-launcher-linux-release --parallel
```

The binary is written to `launcher/bin/`. Use the `docker-launcher-linux-arm64-*` presets for arm64. Windows builds use the `docker-launcher-windows-*` presets and the `toolchain-windows-launcher` image target, tagged as `everstead-builder-launcher-windows:latest`.

## Layout

| File | Contents |
| --- | --- |
| `src/LauncherState.cpp` | Versions, profiles, settings, downloads and starting the game |
| `src/LauncherUpdater.cpp` | Updating the launcher itself |
| `src/GameLogModel.cpp` | The game output shown in the log window |
| `qml/Main.qml` | The user interface |
