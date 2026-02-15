# SkyrimVR-DamageHUD

SKSE VR mod that displays floating damage and heal numbers on the HUD for Skyrim VR.

- Damage dealt to your target shown center-right
- Damage received by the player shown center-left
- Only displays for the player's current hit target
- Configurable via `FloatingDamage.ini`

## Requirements

- [Skyrim VR](https://store.steampowered.com/app/611670/The_Elder_Scrolls_V_Skyrim_VR/)
- [SKSEVR](https://skse.silverlock.org/)
- [SkyUI VR](https://github.com/ArranzCNL/SkyUI-VR) (for fonts)

## Build

### Prerequisites

- Visual Studio 2022 with C++ desktop workload
- [CMake](https://cmake.org/) 3.21+
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` environment variable set

### Steps

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/jrubimf/SkyrimVR-DamageHUD.git
cd SkyrimVR-DamageHUD

# Configure
cmake --preset vr-release

# Build
cmake --build build --config Release
```

Output DLL: `build/Release/FloatingDamage.dll`

### Install

Copy `FloatingDamage.dll` to your `Data/SKSE/Plugins/` folder (or mod manager equivalent).

The compiled SWF (`FloatingDamage.swf`) goes in `Data/Interface/`.

## Configuration

Edit `Data/SKSE/Plugins/FloatingDamage.ini`:

| Setting | Description |
|---------|-------------|
| `EnableHealth` | 0=off, 1=damage only, 2=heal only, 3=both |
| `EnableMagicka` / `EnableStamina` | Same as above |
| `Size` | Base text size |
| `Alpha` | Text transparency (0-100) |
| `DamageDisplayMode` | 0=Default, 1=Freeze, 2=Accelerate, 3=Bound, 4=Drop |
| `HealDisplayMode` | Same animation modes for healing |

## Credits

- Original FloatingDamage SE mod by Flavor
- Ported to VR using [CommonLibVR](https://github.com/alandtse/CommonLibVR)
