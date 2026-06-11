<div align="center">

# Teeworlds — OUYA Port

**[Teeworlds](https://www.teeworlds.com/)** — the free, fast 2D retro multiplayer shooter — running **natively on the OUYA** console, with full gamepad support.

[![Download](https://img.shields.io/badge/Download-APK-2ecc71?style=for-the-badge)](https://github.com/RyoSaeba89/teeworlds-ouya/releases/latest)

NVIDIA Tegra 3 &nbsp;·&nbsp; Android 4.1 (API 16) &nbsp;·&nbsp; OpenGL ES 2.0 &nbsp;·&nbsp; armeabi-v7a &nbsp;·&nbsp; Teeworlds 0.7.5

</div>

---

## What this is

A native OUYA build of **Teeworlds 0.7.5** — the upstream release, unmodified gameplay, plus
the engine and platform work needed to make it boot, render and play on the OUYA's NVIDIA Tegra 3
(Android 4.1 / OpenGL ES 2.0 only). Everything ships in one ~30 MB APK: the game, its free
maps/skins/sounds, and a gamepad control scheme so the whole game is playable from the couch with
no keyboard or mouse.

Online multiplayer against standard Teeworlds 0.7 servers works.

## Features

- 🎮 **Full OUYA controller support** — twin-stick play: the right stick aims the crosshair, the
  left stick (and D-pad) moves, with hook, fire, jump, weapon-switch, scoreboard and a menu button
  all mapped to the pad. The menus are fully gamepad-drivable too.
- 🖥️ **GLES2 rendering via gl4es** — Teeworlds' desktop fixed-function OpenGL is translated to
  OpenGL ES 2.0, including the `LIBGL_NOTEST` workaround for the Tegra 3 Cg shader-compiler crash
  that kills most GLES ports on this hardware.
- 🗺️ **Tilemap rendering on a 3D-texture-less GPU** — Tegra 3 reports `GL_MAX_3D_TEXTURE_SIZE = 0`,
  so Teeworlds' tileset arrays are remapped to a 2D texture atlas (with a half-texel inset to keep
  tile edges seamless). Maps render correctly.
- ⚡ **Tuned for the Tegra 3** — the render surface is drawn at 540p and upscaled to fullscreen,
  giving a smooth, vsync-locked framerate where native resolution was GPU-bound.
- 🌐 **Online multiplayer** — join Teeworlds 0.7 servers over the network.
- 🔊 SDL audio; no proprietary data — Teeworlds ships its own free art, maps and sounds.

## Download & install

Grab the APK from the **[latest release](https://github.com/RyoSaeba89/teeworlds-ouya/releases/latest)**,
then either copy it to the console and open it with a file manager, or sideload over ADB:

```bash
adb connect <your-ouya-ip>:5555
adb install Teeworlds-OUYA.apk
```

The first launch unpacks the game data (~28 MB) to the console — give it a moment on the first run only.

## Controls

| Control | Action | | Control | Action |
|---|---|:-:|---|---|
| **Right stick** | Aim the crosshair | | **L1** | Jump |
| **Left stick / D-pad** | Move left / right | | **R1** | Next weapon |
| **R2** | Fire | | **L3** | Scoreboard (hold) |
| **L2** | Hook | | **Face button** | Open / close menu · Back |
| **A** | Confirm / click (in menus) | | | |

Menu cursor speed (`ui_joystick_sens`), aim sensitivity (`joystick_sens`) and dead-zone
(`joystick_tolerance`) are adjustable and persisted. The full mapping lives in the shipped
[`autoexec.cfg`](android/app/src/main/assets/autoexec.cfg) and can be re-bound on-device.

## Building from source

The complete build recipe and engineering log — toolchain, the hand-cross-compiled native
dependencies (gl4es, SDL2, FreeType), every runtime fix, the controller mapping and the
performance pass — is in **[OUYA_PORT.md](OUYA_PORT.md)**.

```powershell
# 1. native: CMake/ninja -> libmain.so -> staged into jniLibs
cd android
cmd /c build_native.bat

# 2. package: Java + jniLibs + assets -> APK
$env:JAVA_HOME = "<portable JDK 11>"
.\gradlew.bat assembleDebug --no-daemon
```

> The OUYA targets **API 16 / GLES2**, so it needs **NDK 23.2.x** (the last NDK that targets
> `android-16` on armeabi-v7a) and a Java ≤ 16 JDK for AGP 7.0.2. The build packages a prebuilt
> `libmain.so` directly (gradle's `externalNativeBuild` `.so` copy is broken on the Windows host) —
> see §6 of the port log.

## Credits & license

- **Teeworlds** © the Teeworlds developers — <https://www.teeworlds.com/>. Engine/code under a
  zlib-style license; art and data under CC-BY-SA 3.0. See [`license.txt`](license.txt).
- **OUYA port** by RyoSaeba89, building on the toolchain and native-dependency work from the
  [AssaultCube OUYA port](https://github.com/RyoSaeba89/assaultcube-ouya).
- [gl4es](https://github.com/ptitSeb/gl4es) (MIT) — OpenGL → GLES2 translation.
- [SDL2](https://www.libsdl.org/) (zlib) · [FreeType](https://freetype.org/) (FTL).

This is an unofficial, non-commercial fan port. Not affiliated with the Teeworlds project or OUYA.
