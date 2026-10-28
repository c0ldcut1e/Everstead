# Everstead

Everstead is a hobby Minecraft clone that tries to combine the feel of older Minecraft with features from newer versions. It is written in C++ with its own engine, and has online multiplayer and Lua mod support.

## Repository layout

| Folder | Contents |
| --- | --- |
| `game/` | The game client and the dedicated server, built from the same sources |
| `launcher/` | The Qt launcher |
| `backend/` | The Go backend for accounts, sessions and the UDP rendezvous used for peer to peer connections |
| `modkit/` | The Lua library and an example mod |
| `cmake/` | Build scripts, including the mod binding generator |

## Assets

The font, sound and texture folders under `game/bin/assets/` are not included in this repository. You have to provide your own files in these folders to run the game:

- `game/bin/assets/font/`
- `game/bin/assets/sound/`
- `game/bin/assets/texture/`

## Building

Everything is built inside Docker, so the only requirement is Docker itself. The VS Code tasks in `.vscode/tasks.json` wrap every step below.

Build the toolchain image:

```sh
docker build --target toolchain --tag everstead-builder:latest .
```

Configure and build a Linux release:

```sh
docker run --rm --user "$(id -u):$(id -g)" --volume "$PWD:$PWD" --workdir "$PWD" everstead-builder:latest cmake --preset docker-linux-release
docker run --rm --user "$(id -u):$(id -g)" --volume "$PWD:$PWD" --workdir "$PWD" everstead-builder:latest cmake --build --preset docker-linux-release --parallel
```

Other presets are listed in `CMakePresets.json`, covering Linux arm64, Windows and the launcher. Windows builds use the `toolchain-windows` image target, tagged as `everstead-builder-windows:latest`.

## Backend

The backend runs with Docker Compose together with MongoDB and PostgreSQL:

```sh
docker compose up --build
```

Set `EVERSTEAD_PUBLIC_HOST` to the address clients should reach the server at. `docker-compose.production.yml` holds the production setup.

The game talks to the backend over HTTPS by default. To use a local backend, start the game with `--server http://127.0.0.1:8088`.

## Modding

Mods are written in Lua and loaded from the `mods` folder in the game's data directory:

| Platform | Path |
| --- | --- |
| Linux | `~/.local/share/Everstead/mods/` |
| Windows | `%LOCALAPPDATA%\Everstead\mods\` |
| macOS | `~/Library/Application Support/Everstead/mods/` |

Each mod is a folder with a `mod.json` and a `main.lua`:

```json
{
    "id": "example",
    "name": "Example Mod",
    "version": "1.0.0",
    "gameVersions": ["0.2"],
    "side": "both"
}
```

`side` decides who needs the mod when playing together:

| Side | Meaning |
| --- | --- |
| `both` | The default. Players can only join a world or server that runs the same mods at the same versions |
| `client` | Only runs in the game, for things like rendering or UI. Servers skip it and nobody else needs it |
| `server` | Only matters where the world runs. Players can join without it |

Dedicated servers load mods from a `mods` folder next to their `server.json`. A `both` mod can check `DedicatedServer ~= nil` to tell when it runs there, since client classes don't exist on a dedicated server.

Mods hook game functions by name and call game classes directly from Lua:

```lua
hook("ItemRegistry::Init", function(original)
    original()

    local ruby = new("Item")
    ruby:SetIconPath(mod.directory .. "/ruby.png")
    ruby:SetId(ItemRegistry.GetInstance():Register("example:ruby", ruby))
end)
```

Mods can ship translations in `lang/<code>.loc`, which are merged over the game's own strings. See `modkit/example/` for a complete mod.

For editor autocompletion, point your Lua language server at `modkit/library/`. The build generates `game.lua` there from the game's headers, and `modkit/.luarc.json` sets this up for VS Code.

## Community

[Discord server](https://discord.gg/7jUxqYfJNT)
