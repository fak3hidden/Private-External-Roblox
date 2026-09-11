# Private-External-Roblox

Private Windows client-research project. Two tools, one launcher.

## Quick start (Windows)

1. Clone to a **short path** (deep/nested folders hit Windows MAX_PATH errors):
   ```powershell
   git clone -b arena/01a07d35-private-external-roblox https://github.com/fak3hidden/Private-External-Roblox.git C:\ucr
   ```
   If you used the GitHub ZIP instead, extract it to a short path like `C:\ucr`
   and double-click `update.cmd` once — it turns the folder into a repo.
2. Install Visual Studio (Desktop development with C++ workload).
3. Double-click **`launcher.cmd`**.

## launcher.cmd

```
1. EXTERNAL   runs the overlay (ESP, aimbot, movement, ...)
2. INTERNAL   injects UCRCore.dll into a running Roblox
3. UPDATE     pulls the latest code (same as update.cmd)
4. QUIT
```

Missing binaries are built automatically on first run (needs MSBuild, which
comes with Visual Studio). Roblox must already be running before option 2.

## Build layout

Build outputs land in per-project `bin\<platform>\<config>\` folders (gitignored):

| Binary | Output path |
| --- | --- |
| External | `undetected-external-main/UCRobloxExternal/UCRobloxExternal/bin/x64/Release/UCRobloxExternal.exe` |
| Injector | `internal/Injector/bin/x64/Release/Injector.exe` |
| Core DLL | `internal/Core/bin/x64/Release/UCRCore.dll` |

Manual builds:

- External: open `undetected-external-main/UCRobloxExternal/UCRobloxExternal/UCRobloxExternal.vcxproj`, Release x64.
- Internal: open `internal/Internal.sln`, Release x64 (builds Injector + Core).

## updating

Double-click `update.cmd`. It replaces local files with the latest committed
version — no git required (it downloads the branch ZIP and copies it over; if
git is installed it uses that instead, which also makes a plain `git pull` work
here afterwards). Updates discard local edits to tracked files, so don't
hand-edit files and expect changes to survive; commit them instead.
