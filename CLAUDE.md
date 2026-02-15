# FloatingDamage - Skyrim VR Mod

## Project Overview
A Skyrim VR SKSE mod that displays floating damage numbers above actors. Ported from flat Skyrim SE (v1.5.39) to VR using CommonLibVR.

## Build System
- CMake with Visual Studio 2022 generator
- Configure: `cmake --preset vr-release`
- Build: `cmake --build build --config Release`
- Output DLL: `build/Release/FloatingDamage.dll`
- Deploy to: `SKSE/Plugins/FloatingDamage.dll` in mod folder
- Dependencies managed via vcpkg (openvr, rsm-binary-io, spdlog, xbyak)

## Architecture

### Data Flow
```
Game Event (actor takes damage)
  → AdvanceMovie() → UpdateActorData() → ActorData::Update()
    → CheckActorValue() detects health/magicka/stamina change
      → SetDamage() calculates screen position via WorldPtToScreenPt3
        → uiMovie->Invoke("_root.widget.SetDamageText", args)
          → SWF creates animated text at screen position
```

### Key Source Files
| File | Role |
|------|------|
| `src/Plugin.cpp` | Entry point, SKSE init, message handling |
| `src/FloatingDamage.h/cpp` | Main menu class, event sinks, actor list management |
| `src/ActorData.h/cpp` | Per-actor damage tracking, screen projection, SWF invocation |
| `src/Settings.h/cpp` | INI file parsing (`FloatingDamage.ini`) |
| `src/PCH.h` | Precompiled header |

### SWF Files (in `swf/`)
| File | Role |
|------|------|
| `Main.as` | AS2 entry point, creates DamageWidget |
| `DamageWidget.as` | Widget class - SetSettings(), SetDamageText() |
| `TextWidget.swf` | Pre-compiled animation clips (DefaultIns, FreezeIns, AccelerateIns, BoundIns, DropIns) |

- Compiled SWF goes to `Interface/FloatingDamage.swf` in game data
- The compiled SWF contains all required symbols (verified by decompression)
- Uses CASA Lib (`org.casalib.util.MovieClipUtil`) for movie clip registration
- Compiled with MTASC (Motion-Twin ActionScript Compiler)

### Original Flat Skyrim Code
- Located in `dll/` directory (reference only, uses old SKSE64 API)
- Original menu flags: `0x10802` = `kAlwaysOpen | kAllowSaving | kAssignCursorToRenderer`
- Original depthPriority: `2`

## VR-Specific Findings

### Menu Rendering in VR - CRITICAL
- **Plain `IMenu` menus DO NOT render in Skyrim VR.** The VR engine uses `WorldSpaceMenu` (which creates a 3D NiNode in the scene graph) for visible HUD overlays.
- The VR `HUDMenu` inherits from `WorldSpaceMenu`, NOT directly from `IMenu`.
- `WorldSpaceMenu` is VR-only (`#ifdef SKYRIMVR`) and extends `IMenu` + `BSTEventSink<HudModeChangeEvent>`.
- It creates a `menuNode` (NiNode) attached to a parent node in the scene graph.
- Pure virtual methods: `GetMenuParentNode()` (where to attach), `SetTransform()` (position/rotation/scale).

### Menu Flags That DON'T Work in VR (tested)
- `kAlwaysOpen | kAllowSaving | kCustomRendering` — NOT visible
- `kAlwaysOpen | kAllowSaving` — NOT visible
- `kAlwaysOpen | kAllowSaving | kAssignCursorToRenderer` — NOT visible
- `kAlwaysOpen | kRequiresUpdate | kAllowSaving` — NOT visible
- All of the above with various `depthPriority` values (0, 2, 5) — NOT visible

### LoadMovie vs LoadMovieEx
- `LoadMovie` delegates to the engine's internal function (safe, does VR-specific setup)
- `LoadMovieEx` is a CommonLibVR wrapper that manually creates the movie — **CRASHES in VR** due to missing internal VR structure initialization
- Always use `LoadMovie` for VR

### VR HUD Architecture
- VR renders UI on 3D billboards/planes in the scene, not 2D overlays
- `PlayerCharacter::uiNode` (offset 0x428) is where VR UI menus attach
- Other VR mods (SpellWheelVR, Minimal Enemy Healthbar) use NIF meshes for 3D world-space rendering
- `interface/vr/` subfolder contains VR-optimized SWF files for built-in menus

### OStim NG VR Reference (working IMenu in VR)
- Uses `kAlwaysOpen | kRequiresUpdate | kAllowSaving`, depthPriority=0
- Does NOT call `RE::IMenu::AdvanceMovie()` (commented out)
- Uses `LoadMovieEx` (but this crashed for us)
- Does Show+Hide during registration as VR workaround
- Claims to work, but may only be visible in specific OStim camera modes

### WorldSpaceMenu Constructor - FLAG BUG
```cpp
WorldSpaceMenu(bool a_registerHudModeChangeEvent, bool a_matchAsTopMenu, bool a_queueUpdateFixup);
```
- Sets default flags: `kUsesCursor | kModal | kDisablePauseMenu | kTopmostRenderedMenu | kAllowSaving | kInventoryItemMenu`
- Sets depthPriority = 2
- **CRITICAL**: `menuFlags.set()` is ADDITIVE (bitwise OR via EnumSet). You MUST call `menuFlags.reset(...)` to clear unwanted flags before calling `menuFlags.set()`. Without this, `kModal` blocks all input, `kUsesCursor` shows cursor, `kDisablePauseMenu` prevents pause, and the game becomes unplayable. This was likely causing the "UI elements gone" issue.

### VR NIF Files (from SkyrimVR.exe strings)
Known VR NIF paths embedded in the binary:
- `skyVR_HMD_info.nif` — HMD info display panel (used by HUDMenu?)
- `skyVR_HMD_Compass.nif` — HMD compass
- `skyVR_compassDial.nif` — compass dial (`Data/Meshes/` prefix)
- `skyVR_crosshair.nif` — crosshair
- `skyVR_dialogue.nif` — dialogue menu panel
- `skyVR_map.nif` — map
- `skyVR_statsMenu.nif` — stats menu
- `skyVR_statPerkPanels.nif` / `skyVR_statSkillPanels.nif` — perk/skill panels
- `vr_EnemyHealthBar.nif` — enemy health bar (also from Minimal Enemy Healthbar VR mod)
- Game path: `F:\Steam\steamapps\common\SkyrimVR`

### EnumSet API (REX::EnumSet)
- `set(args...)` — adds flags (bitwise OR)
- `set(bool, args...)` — adds if true, removes if false
- `reset(args...)` — removes specific flags
- `reset()` — clears ALL flags
- `any(args...)` / `all(args...)` / `none(args...)` — test flags
- `underlying()` — returns raw integer value

### Screen Position Calculation
- Uses `RE::Main::WorldRootCamera()` + `RE::NiCamera::WorldPtToScreenPt3()` for 3D→2D projection
- Returns normalized coordinates (0-1 range) — verified working correctly in VR
- Viewport confirmed as 1440x1440 in VR

## Configuration
- INI file: `data/SKSE/Plugins/FloatingDamage.ini`
- Font: `$EverywhereMediumFont` (requires SkyUI VR)
- EnableHealth: 0=off, 1=damage only, 2=heal only, 3=both
- DamageDisplayMode: 0=Default, 1=Freeze, 2=Accelerate, 3=Bound, 4=Drop

## Debugging
- Log file: `FloatingDamage.log` in SKSE logs folder
- Debug logging added at: OnMenuOpen, UpdateActorList (actor counts), SetDamage (screen positions, culling), Invoke calls (return value)
- All Invoke calls return `true` — confirming SWF functions exist and are callable
- The C++ pipeline is 100% verified working; the issue was purely VR menu visibility
