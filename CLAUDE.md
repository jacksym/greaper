# greaper — git version control for REAPER projects

A native REAPER extension (C++, CMake, git, git-lfs) that adds fetch / commit / push,
an .rpp-aware merge, and a track-based diff view. See `prompt.md` for full goals and
the user's preferred workflow (small change → review → read docs → approve → test in
REAPER → commit). The user is learning agentic coding: explain *why*, cite `file:line`,
and keep changes small and reviewable.

## Repo layout
- `sws-project/` — SWS extension, **reference only** (~131k lines; don't build or modify it)
- `wdl/WDL/` — Cockos WDL: SWELL (Win32 API on Linux/macOS), LICE, string/list utils
- `reaper-sdk/sdk/` — official headers: `reaper_plugin.h`, `reaper_plugin_functions.h`
- `test.rpp` — a sample project for testing

## Environment
- Linux, REAPER at `/usr/local/bin/reaper`, resource dir `~/.config/REAPER/`
- Extensions are loaded from `~/.config/REAPER/UserPlugins/reaper_<name>.so` (currently empty)
- cmake at `/usr/bin/cmake`, g++ at `/usr/bin/g++`

## How a REAPER extension works (learned from SWS)

### 1. Entry point
REAPER `dlopen`s every `UserPlugins/reaper_*.so` at startup and calls one exported C function:
`REAPER_PLUGIN_ENTRYPOINT(hInstance, reaper_plugin_info_t* rec)` — `sws-project/sws_extension.cpp:675`
- `rec != NULL` → load: init everything, `return 1` to stay loaded (`:1341`)
- `rec == NULL` → unload (REAPER quitting): unregister everything, `return 0` (`:677-701`)
- Check `rec->caller_version == REAPER_PLUGIN_VERSION` first (`:715`)
- Keep `hInstance` and `rec->hwnd_main` in globals (the main window is the parent for dialogs)

### 2. Getting the REAPER API
The API is a table of function pointers looked up by name with `rec->GetFunc("Name")`.
- SWS does it by hand: the `IMPAPI(x)` macro (`sws_extension.cpp:669`, ~300 uses from `:733`) plus
  `sws-project/reaper/reaper.cpp` defining `REAPERAPI_IMPLEMENT`.
- **Simpler for us:** the SDK does it in one call. In exactly one .cpp:
  `#define REAPERAPI_IMPLEMENT`, optionally `#define REAPERAPI_MINIMAL` + `REAPERAPI_WANT_<Func>`
  for each function used, include `reaper_plugin_functions.h`, then call
  `REAPERAPI_LoadAPI(rec->GetFunc)`, which returns 0 on success (see header comment at
  `reaper-sdk/sdk/reaper_plugin_functions.h:37-48`).

### 3. Actions (the unit of everything)
Menu items, toolbar buttons and shortcuts all run *actions*. To register one:
1. `int id = plugin_register("command_id", (void*)"GREAPER_COMMIT")` gets a unique command ID
   that stays the same across sessions.
2. `plugin_register("gaccel", &gaccel_register_t{{0,0,id}, "greaper: Commit"})` adds it to the
   Actions list with a description.
3. `rec->Register("hookcommand", (void*)myHook)`: REAPER calls `bool myHook(int cmd, int flag)`
   for **every** action. Return true only if `cmd` is one of ours.
- SWS: `SWSRegisterCmd` `sws_extension.cpp:230`, dispatch `hookCommandProc` `:78`,
  hook registration `:1257-1268`. `COMMAND_T` (id, desc, callback) is at `sws_util.h:85`.
- `hookcommand2` (`:131`) is the newer variant with section/MIDI-CC params; we don't need it.
- `toggleaction` (`:201`): return -1/0/1 so a menu item shows a checkmark. Optional.
- Unregister with a `-` prefix, e.g. `plugin_register("-gaccel", ...)` (`:319`).

### 4. Menu items
- Call `AddExtensionsMainMenu()` (`sws_extension.cpp:1338`) so the top-level *Extensions* menu exists.
- `rec->Register("hookcustommenu", (void*)menuHook)` (`:1330`). REAPER calls
  `menuHook(const char* menuidstr, HMENU menu, int flag)`. When
  `flag==0 && !strcmp(menuidstr,"Main extensions")`, insert items whose IDs are our action IDs
  (`swsMenuHook` `:455`; SWS builds its submenu in `Menus.cpp:243`). Clicking an item runs the
  action through the hookcommand.
- Menus use the Win32 API (`CreatePopupMenu`, `InsertMenuItem`). On Linux these come from SWELL.

### 5. Dockable panel (for the push/remote panel)
`SWS_DockWnd` (`sws-project/sws_wnd.h:177`, `sws_wnd.cpp`):
- Create a modeless dialog: `CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD), hwnd_main, proc, this)` (`sws_wnd.cpp:102`)
- Dock it: `DockWindowAddEx(hwnd, title, id, true)` (`:188`), bring to front: `DockWindowActivate`
- Remember its layout per screenset: `screenset_registerNew(id, cb, this)` (`:63`)
- Dialog resources are .rc files; on Linux SWELL compiles them via `swell/mac_resgen.php` → `.rc_mac_dlg/menu`.

### 6. Storing data inside the .rpp (relevant for merge/diff later)
Register `project_config_extension_t` with `"projectconfig"` (`reaper-sdk/sdk/reaper_plugin.h:1196`):
REAPER calls `SaveExtensionConfig`/`ProcessExtensionLine` while saving/loading a project, so
the extension can keep its own `<GREAPER ...>` chunk in the .rpp. WDL's `lineparse.h` and
`projectcontext.h` parse the .rpp line format.

## Building on Linux
- Output must be a shared library named `reaper_<name>.so` with **no `lib` prefix**
  (`sws-project/CMakeLists.txt:254-257`), installed to `~/.config/REAPER/UserPlugins`
  (`:13-23`, `:271-275`).
- SWELL: plugins **do not** link a full SWELL. They compile `wdl/WDL/swell/swell-modstub-generic.cpp`
  with `-DSWELL_PROVIDED_BY_APP`, which takes SWELL from the running REAPER
  (`sws-project/cmake/FindSWELL.cmake:21-24`). Headers: `wdl/WDL/swell/swell.h`.
- Export the entry point (`REAPER_PLUGIN_DLL_EXPORT` = `visibility("default")`); building with `-fvisibility=hidden` is fine.

## Dev loop
- Source: `src/main.cpp`. First-time setup: `cmake -S . -B build` (Debug by default; writes
  `build/compile_commands.json` for clangd).
- REAPER can't unload or reload extensions at runtime: **rebuild → quit REAPER → relaunch**.
  `cmake --build build && cmake --install build && reaper test.rpp`
- Debug output: `ShowConsoleMsg("...\n")` opens REAPER's console window. Run `reaper` from a
  terminal to see stderr and `fprintf(stderr, ...)`.
- If the extension fails to load, check: the `reaper_` filename prefix, exported symbol
  (`nm -D reaper_greaper.so | grep ReaperPluginEntry`), and missing symbols (`ldd -r`).

## Conventions
- Languages: C++ and C only. Build: CMake.
- Action IDs: prefix `GREAPER_`. Descriptions: prefix `greaper: `.
- Don't commit build output (`build/`).
