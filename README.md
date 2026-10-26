# 9.10 Game Server

A work-in-progress game server for Fortnite **9.10**, built as a C++ DLL that hooks into the game and turns it into a listen server.

> **Status: WIP.** This is my first game server, so expect rough edges and missing features. Feedback and bug reports are welcome. Please be kind 😃

> **Disclaimer:** This is a fan-made, non-commercial project for learning purposes. It is not affiliated with or endorsed by Epic Games. You need your own copy of the game.

---

## Features

**Working**
- [x] Starts a net driver and listens for connections (default port `7777`)
- [x] Loads a configurable playlist (default: `Playlist_DefaultSolo`)
- [x] Forces server mode (`GIsServer` / `GIsClient` / `GetNetMode`)
- [x] Blocks kicks
- [x] Spawns player pawns at their start spot
- [x] Actor replication through the `TickFlush` hook
- [x] Leveled, colored logging to the console and a log file
- [x] Discord webhook that posts a "Server Up" embed when the server starts listening
- [x] Single-file config (`Config.h`) with build-time validation

**Planned**
- [ ] Inventory and item pickups
- [ ] Building
- [ ] Battle bus and dropping
- [ ] Storm
- [ ] Combat and elimination handling
- [ ] Match end and winner handling
- [ ] Multiple playlists

---

## Requirements

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the **Desktop development with C++** workload
- C++17 or newer
- [MinHook](https://github.com/TsudaKageyu/minhook)
- A Fortnite **9.10** client build
- A DLL injector of your choice

---

## Building

1. Clone the repo:
   ```
   git clone https://github.com/mcguffeeeaston-ship-it/9.10
   ```
2. Open the solution in Visual Studio.
3. Set the configuration to **Release | x64**.
4. Edit `Config.h` to your liking (see [Configuration](#configuration)).
5. Build the solution. The output is a `.dll`.

## Usage

1. Launch the 9.10 client.
2. Inject the built DLL into the game process.
3. A console window opens and shows startup logs. The title changes as the server progresses:
   - `9.10 | Starting...` while hooks are installed
   - `9.10 | Waiting for map` while the map loads
   - `9.10 | Listening` once the server accepts connections
4. Connect a client to `127.0.0.1:7777`.

---

## Configuration

Everything lives in `Config.h`. Edit a value, rebuild, done. Toggles are plain `true` / `false`.

| Setting | Default | What it does |
|---|---|---|
| `ServerPort` | `7777` | Port the server listens on |
| `ServerRegion` | `"NAE"` | Region shown in the webhook embed |
| `MapName` | `"Athena_Terrain"` | Map to open |
| `PlaylistName` | `"Playlist_DefaultSolo"` | Playlist to load |
| `MaxPlayers` | `100` | Maximum players |
| `WarmupPlayers` | `1` | Players needed before the match starts |
| `KickPlayerPatch` | `true` | Block kicks |
| `NetModePatch` | `true` | Force server net mode |
| `LogDebug` / `LogInfo` / `LogWarn` / `LogError` | `true` | Turn each log level on or off |
| `ColorLogging` | `true` | Colored console output |
| `FileLogging` | `true` | Also write logs to `LogFile` |
| `LogFile` | `"9.10-server.log"` | Log file name |
| `Heartbeat` | `true` | Periodic status line in the log |
| `HeartbeatSeconds` | `30` | Seconds between heartbeats |
| `DiscordWebhook` | `true` | Send a "Server Up" message |
| `WebhookUrl` | placeholder | Your Discord webhook URL |

Invalid values (like a port above 65535) fail the build with a clear message instead of breaking at runtime.

### Discord webhook

1. In Discord: **Channel Settings → Integrations → Webhooks → Copy Webhook URL**.
2. Paste it into `WebhookUrl` in `Config.h`.
3. Rebuild.

> **Keep your webhook URL private.** Anyone with it can post to your channel. Don't commit a real one to a public repo. If it leaks, delete the webhook in Discord and make a new one.

If you don't want the webhook, set `DiscordWebhook = false`.

---

## Project layout

| File | Purpose |
|---|---|
| `dllmain.cpp` | Entry point: installs hooks, forces server mode, opens the map |
| `Game/GameMode.h` | Game mode logic: playlist setup, net driver, pawn spawning, replication tick |
| `Config.h` | All user-facing settings |
| `Log.h` | Logging (levels, colors, file output) |
| `Webhook.h` | Async Discord webhook sender |
| `framework.h` | SDK includes, addresses, hook helpers |

---

## Troubleshooting

**`invalid macro definition: 910_EXPORTS`**
Visual Studio generated a macro name starting with a digit from the project name. Remove `910_EXPORTS` under **Properties → C/C++ → Preprocessor → Preprocessor Definitions**, or rename the project so it doesn't start with a number.

**Nothing happens after injecting**
Check the console and `9.10-server.log` for errors. Make sure the addresses in your project match the exact 9.10 build you're running.

**Clients can't connect**
Check that `ServerPort` is open in your firewall and forwarded on your router if you're hosting for people outside your network. Look for `InitListen succeeded` in the log.

**Webhook not sending**
Look for a `Webhook` line in the log. A rejected request means the URL is wrong. A warning that it was skipped means it still contains `REPLACE_ME`.

---

## Contributing

Issues and pull requests are welcome. If you're reporting a bug, include the contents of `9.10-server.log`.

## Credits

- [MinHook](https://github.com/TsudaKageyu/minhook) for function hooking
- *Add anyone whose SDK, research or code helped you here*

## License

*This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.*
