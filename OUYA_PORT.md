# Teeworlds 0.7.5 → OUYA Port — Build & Engineering Log

Porting **Teeworlds** (the 2D retro multiplayer shooter, latest release **0.7.5**) to the
**OUYA** (NVIDIA Tegra 3, Android 4.1 / API 16, OpenGL ES 2.0 only, armeabi-v7a).

- **Status:** **FULLY PLAYABLE on a real OUYA** (Android 4.1.2). Boots to the menu (UI, fonts,
  country-flags AND the background map/tilemap all rendering via gl4es→GLES2); SDL audio + data +
  controller all OK; online multiplayer against Teeworlds 0.7 servers works. All session-2/3
  blockers fixed and verified on device — gamepad menu cursor, in-game movement, tile-grid seams,
  framerate and the full control mapping (see **§14, Session 3**). Package `org.teeworlds.ouya`, ~30 MB.

### On-device bring-up findings (session 2)
1. **Black screen was the `SDL_main` symbol.** SDL Android `dlsym`s `SDL_main`; the
   `#define main SDL_main` macro didn't fire and Teeworlds' `const char**` main was C++-mangled,
   so `libmain.so` exported `T main`, not `SDL_main` ("Couldn't find function SDL_main"). Fixed by
   exporting `extern "C" int SDL_main(int, char**)` directly on Android (client.cpp).
2. **`dbg_msg` now mirrors to logcat** (system.c, tag `teeworlds`) — SDL 2.0.14 doesn't forward
   stdout. Essential for everything below.
3. **Maps render via a 2D-texture fallback.** `GL_MAX_3D_TEXTURE_SIZE = 0` on Tegra 3/gl4es →
   tilesets (loaded 3D-only via `TEXLOAD_ARRAY_256`) had no texture → per-frame
   `invalid texture 25 3 0` flood and blank tile layers. Fix (Android-only, §3a/§7): keep the 2D
   atlas and map the tile index into 16×16 atlas UVs in `RenderTilemap`. Flood gone, maps draw.
4. **Gamepad input fully reaches the engine via the SDL Joystick API** (verified with
   `getevent` + in-engine event logging): left stick = axes 0,1; **right stick = axes 2,3**;
   A = button 0, B = 1; the OUYA touchpad also works as a real mouse. The **only** defect: the
   menu cursor moves 0.1 px/frame because `CMenus::OnCursorMove` applies no sensitivity to the
   joystick delta (see §14). A-as-click already works.
- **Base:** official `teeworlds/teeworlds` @ 0.7.5 (already SDL2 + fixed-function OpenGL).
- **Recipe reused from the shipped AssaultCube OUYA port** (`../AssaultCube/OUYA_PORT.md`):
  prebuilt gl4es `libGL.a` + `libSDL2.so` + `libhidapi.so` + `libc++_shared.so`, the
  `LIBGL_NOTEST` Tegra-Cg fix, the SDL `org.libsdl.app` Java, NDK 23 / JDK 11 toolchain.

---

## 1. Why Teeworlds is a safe OUYA target
- **Fixed-function OpenGL** (`backend_sdl.cpp`: `glOrtho`, `glVertexPointer`/`glTexCoordPointer`/
  `glColorPointer`, `glDrawArrays(GL_QUADS)`, `glEnable(GL_TEXTURE_2D)`) — exactly the
  immediate/array class **gl4es→GLES2** handles, same as the Cube engine.
- **SDL2** windowing → reuse the proven API-16 `libSDL2.so` (2.0.14).
- **2D** game → trivial for the Tegra 3 GPU (no fillrate concern, unlike AssaultCube).
- **BSD-like, ships its own free art/maps/sounds** → no proprietary game data needed.
- Simpler than AssaultCube: **no OpenAL** (Teeworlds mixes audio through SDL_Audio) and
  **no SDL2_image** (PNGs via the bundled `pnglite`).

## 2. Toolchain (this machine)
- NDK **23.2.8568313** (last NDK targeting `android-16` on armeabi-v7a).
- SDK CMake **3.22.1** + bundled `ninja`.
- Portable **Temurin JDK 11** (`../_jdk11/jdk-11.0.31+11`) for AGP 7.0.2 / Gradle 7.0.2.
- `adb` from `%LOCALAPPDATA%\Android\Sdk\platform-tools`. OUYA over Wi-Fi (`<OUYA-IP>:5555`).

## 3. Source code generation (one-time, Python)
Teeworlds generates network-protocol + content sources at build time. Run once with the
host Python (3.14 works) into `src/generated/`:
```
python datasrc/compile.py network_source        > src/generated/protocol.cpp
python datasrc/compile.py network_header        > src/generated/protocol.h
python datasrc/compile.py client_content_source > src/generated/client_data.cpp
python datasrc/compile.py client_content_header > src/generated/client_data.h
python scripts/git_revision.py                  > src/generated/git_revision.cpp
python scripts/cmd5.py src/engine/shared/protocol.h src/game/tuning.h \
       src/game/gamecore.cpp src/generated/protocol.h > src/generated/nethash.cpp
```
(Server-only `server_data.*` also generated but unused by the client.)

## 4. Native dependencies
| Dep | Form | Source |
|-----|------|--------|
| **gl4es** | `libGL.a` (static) | reused from the AssaultCube OUYA port (GLES2 backend) |
| **SDL2 2.0.14** | `libSDL2.so` (+ `libhidapi.so`) | reused from AssaultCube |
| **libc++_shared** | `libc++_shared.so` | NDK 23 (ANDROID_STL=c++_shared) |
| **freetype 2.13.2** | `libfreetype.a` (static) | **cross-compiled here** for armeabi-v7a/android-16 (CMake, all optional deps off) |
| **zlib / pnglite / wavpack / json-parser / md5** | compiled in | bundled in Teeworlds `src/engine/external/` |

No OpenAL, no SDL2_image.

## 5. Android project (`teeworlds/android/`)
Copied + pruned from the AssaultCube `android/` skeleton: gradle wrapper, `org.libsdl.app`
SDL Java, gl4es headers, the prebuilt `.so`/`.a` under `app/src/main/cpp/lib/`.
- `app/src/main/cpp/CMakeLists.txt` — **new, Teeworlds-specific**: globs base + engine
  (shared/client) + game (shared/client/editor) + generated + bundled externals + the android
  glue, links gl4es/freetype/SDL2/hidapi → **`libmain.so`** (SDL loads `SDL_main` from it).
- `AndroidManifest.xml` — `glEsVersion 0x20000`, minSdk 16, single `TeeworldsActivity`.
- `TeeworldsActivity` extends `SDLActivity`; `getLibraries()` = `c++_shared, hidapi, SDL2, main`
  (the API-16 linker won't follow transitive `DT_NEEDED`, so order matters; `main` last).
- `app/build.gradle` — app id `org.teeworlds.ouya`, NDK 23, abiFilters `armeabi-v7a`.

## 6. ★ The AGP native-build workaround (the build blocker on this host)
AGP 7.0.2's `externalNativeBuild` post-link `.so` copy
(`CxxRegularBuilder.hardLinkOrCopy` → `FileUtils.isSameFile`) throws
`java.io.IOException: The filename, directory or volume name is incorrect` on this Windows
machine **every time ninja produces `libmain.so`** (independent of the path-space issue).
**Fix:** stop driving CMake from gradle. Build `libmain.so` standalone with CMake/ninja and
the NDK toolchain (`android/build_native.bat`), stage it + the prebuilt `.so` into
`app/src/main/jniLibs/armeabi-v7a/`, and remove `externalNativeBuild` from `build.gradle`.
Gradle then only packages the prebuilt jniLibs + assets — fast and reliable.
```
cd android && build_native.bat        REM cmake/ninja -> libmain.so -> jniLibs
gradlew.bat assembleDebug --no-daemon REM packages jniLibs + assets + Java
```

## 7. Native code changes (engine)
1. **SHA-256 shim** — Teeworlds 0.7 has **no bundled SHA-256** (`hash_bundled.c` only wraps
   MD5; upstream links it from OpenSSL/libtomcrypt, neither of which we ship). Symptom: link
   error `undefined symbol: sha256_init/update/finish` (referenced by `base/hash.c`,
   `engine/shared/datafile.cpp`). Fix: `android/app/src/main/cpp/tw_sha256.c` — public-domain
   SHA-256 against the LibTomCrypt-layout `SHA256_CTX` that `base/hash_ctxt.h` declares.
2. **GLES2 context** (`src/engine/client/backend_sdl.cpp`) — under `#ifdef __ANDROID__`,
   request an ES 2.0 context before window creation
   (`SDL_GL_CONTEXT_PROFILE_ES` + major 2 / minor 0), else SDL asks EGL for a desktop-GL
   config and `SDL_GL_CreateContext` fails with `EGL_BAD_DISPLAY`.
3. **gl4es env constructor** (`android/.../cpp/tw_android.cpp`, `constructor(100)`) — sets
   `LIBGL_ES=2`, **`LIBGL_NOTEST=1`** (skips the ES3/desktop-GLSL probe shaders that SIGSEGV
   the Tegra 3 Cg compiler — the real OUYA blocker, proven in the AssaultCube port),
   `LIBGL_NOHIGHP/NOPSA/NOBANNER/MIPMAP=3/NOERROR`.
4. **Android data path** (`tw_android_setup()` in `tw_android.cpp`, called at the top of
   `main()` in `client.cpp`) — `chdir()` + set `HOME` to
   `SDL_AndroidGetExternalStoragePath()`, so `storage.cpp` `FindDataDir()` finds `data/mapres`
   in the PWD and `$USERDIR`/`$CURRENTDIR` (save paths) resolve to a writable dir on API 16.

## 8. Assets & data pipeline
- Teeworlds' runtime `data/` = the `datasrc/` tree filtered to
  `json,map,png,rules,ttc,ttf,txt,wv` (661 files, ~28 MB) — all **free**. Bundled into
  `android/app/src/main/assets/data/`.
- The fopen-based engine can't read from inside the APK, so `AssetExporter.java` unpacks
  `assets/` (data + `autoexec.cfg`) into the app external files dir on first launch (versioned,
  64 KB buffered copy), and `TeeworldsActivity.onCreate` runs it before SDL starts.

## 9. Gamepad — twin-stick (mostly config; Teeworlds 0.7 has native SDL joystick support)
Teeworlds already exposes joystick cvars (`joystick_enable/absolute/sens/x/y/tolerance`) and
bindable joystick keys (`joystick0..11`, `joy_hat0_*`, `joy_axisN_left/right`), so most of the
mapping is the shipped **`autoexec.cfg`**. The OUYA SDL button indices were **decoded on device**
(`inpdbg` event logging): face buttons = `joystick0..3`, **L1 = `joystick9`, R1 = `joystick10`,
L3 = `joystick7`, R3 = `joystick8`**; the **L2 trigger also emits digital button 15** in addition
to its axis. Important: the engine only maps joystick buttons **0–11** (`HandleJoystickButtonEvent`
drops `>= NUM_JOYSTICK_BUTTONS = 12`), so only `joystick0..11` are bindable. Final **audited 1:1
mapping** (no function bound to two inputs, except the deliberate left-stick/D-pad movement pair):
| Control | Action | | Control | Action |
|---------|--------|---|---------|--------|
| Right stick (axes 2,3, absolute) | aim | | L1 (`joystick9`) | jump |
| Left stick (axis 0) + D-pad | move L/R | | R1 (`joystick10`) | `+nextweapon` |
| R2 (`joy_axis5_right`) | fire | | L3 (`joystick7`) | `+scoreboard` |
| L2 (`joy_axis4_right`) | hook | | Face `joystick3` | menu / Back (Escape) |
| A (`joystick0`) | UI click (menus) | | | |

The **menu / Back** button has no console command (the menu is opened by the Escape *key*), so
`joystick3` is remapped to `KEY_ESCAPE` in `CMenus::OnInput` under `__ANDROID__` — it opens the
menu in-game and cancels/closes in menus, the single menu/Escape control. `A`-as-UI-click is in
`CUI::Update` (`ui.cpp`). All other binds are in `autoexec.cfg`.

## 10. Remaining / on-device bring-up (when the OUYA is powered on)
```
cd android
build_native.bat
gradlew.bat assembleDebug --no-daemon
adb connect <OUYA-IP>:5555
adb -s <OUYA-IP>:5555 install -r app\build\outputs\apk\debug\app-debug.apk
adb -s <OUYA-IP>:5555 shell am start -n org.teeworlds.ouya/.TeeworldsActivity
adb logcat -s teeworlds LIBGL SDL gfx joystick storage
```
Expect: boot → main menu (gl4es→GLES2) → join an online server. Watch-items:
- **★ Tilemap 3D textures** — Teeworlds uploads tilesets as `GL_TEXTURE_3D` arrays
  (`backend_sdl.cpp` ~425). GLES2 has no 3D textures; gl4es maps them to `GL_OES_texture_3D`,
  which Tegra 3 supports, so it *should* render. Fallback if maps are blank: force the tile
  layer to the 2D-atlas path (each texture also keeps a `STATE_TEX2D` variant).
- Confirm twin-stick axis/button indices; tune `autoexec.cfg`.
- Perf pass / 720p render-downscale (SDLActivity `sRenderMaxHeight`, inherited from AssaultCube)
  — likely unnecessary for a 2D game but available.
- Release signing (mirror AssaultCube §11) + OUYA store banner for publication.

## 14. Session 3 — gameplay polish (all fixed & verified on device)
The session-2 bring-up left four playability issues. Each was **measured on device, not guessed**
(the temporary `inpdbg`/`curdbg`/`uidbg` logging from session 2 + `dumpsys SurfaceFlinger`).

1. **Menu cursor speed.** Measured: at full right-stick deflection `Input()->CursorRelative()`
   returns a joystick delta of **±0.10 px/frame** vs **±6–7 px** for the mouse — ~60× too slow.
   **Fix** (`CUI::ConvertCursorMove`, `ui.cpp` ~184): for `CURSOR_JOYSTICK`,
   `Factor = m_UiJoystickSens/100.0f * 60.0f`. A fixed UI-only ×60 (hardcoded, *not* a default-cvar
   bump — `ui_joystick_sens` is `CFGFLAG_SAVE` and already pinned at 100 in the device's
   `settings.cfg`, so a new default would be overridden). In-game aim is on a separate
   `joystick_sens` path (`controls.cpp`) and is untouched.

2. **★ Left-stick "random" in-game movement.** The tee moved left/right erratically — pushes were
   swallowed, releases missed. Root cause: **two writers of `m_aInputState` fighting**.
   `CInput::Update()` zeroes the joystick keys and `UpdateJoystickState()` re-polls them every
   frame *before* the SDL event loop; `HandleJoystickAxisMotionEvent()` then read that same
   `m_aInputState[LeftKey/RightKey]` as its "previous state" for `PRESS`/`RELEASE` **edge
   detection** — so the per-frame poll corrupted the edges, emitting `+left`/`+right` events at
   random. **Fix:** give the event handler its own persistent edge array
   `m_aJoystickAxisEventState[NUM_JOYSTICK_AXES*2]` (`input.h`, zeroed in the ctor); polling still
   maintains `m_aInputState` for `KeyIsPressed` held-state. (The pad hardware is clean — a 41 s
   rest window produced **zero** axis events, ruling out drift.)

3. **Map tile-grid seams.** The session-2 2D-atlas tilemap fallback (§3/§7, `render_map.cpp`)
   sampled exactly at each atlas cell border, so linear filtering bled the neighbouring cell into
   every tile edge → a visible grid over repeated ground tiles. **Fix:** inset each cell by ~half a
   texel — `In = TS/128`, map the local tile UV `0..1` into `[In, TS-In]`.

4. **Framerate.** Measured: CPU **8 %** (not CPU/RAM-bound; ~62 MB PSS), real **18.8 FPS** at 720p
   (`dumpsys SurfaceFlinger --latency SurfaceView`); a vsync-off test gave 20.6 FPS with continuous
   ~48 ms frames → **GPU-bound, vsync is not the cap**. **Fix:** drop the render-surface scale
   `sRenderMaxHeight` **720 → 540** (`SDLActivity.java`, 0.56× the pixels); vsync (`gfx_vsync 1`,
   kept — off only added tearing) then locks a smooth ~30. The downscaled buffer is upscaled to the
   display by SurfaceFlinger.

See **§9** for the final audited gamepad mapping that was also settled this session.

## Appendix — files created/changed
- `src/base/system.c` (dbg_msg -> logcat), `src/engine/client/client.cpp` (SDL_main entry + setup)
- `src/engine/client/graphics_threaded.cpp` + `src/game/client/render_map.cpp` (2D tilemap fallback)
- `src/game/client/ui.cpp` (A/B -> UI click), temp input diagnostics (input.cpp/gameclient.cpp/ui.cpp)
- `src/generated/*` (codegen output, checked in)
- `src/engine/client/backend_sdl.cpp` (ES2 context under `__ANDROID__`)
- `src/engine/client/client.cpp` (`tw_android_setup()` call at top of `main()`)
- `android/app/src/main/cpp/CMakeLists.txt`, `tw_android.cpp`, `tw_sha256.c`
- `android/app/src/main/cpp/lib/**` (prebuilt gl4es/SDL2/freetype/c++_shared), `include/**` (SDL2/freetype/gl4es headers)
- `android/app/src/main/jniLibs/armeabi-v7a/*.so` (staged native libs)
- `android/app/src/main/assets/data/**`, `assets/autoexec.cfg`
- `android/app/src/main/java/org/teeworlds/ouya/{TeeworldsActivity,AssetExporter}.java`
- `android/app/src/main/java/org/libsdl/app/SDLActivity.java` (dropped AssaultCube imports)
- `android/app/src/main/AndroidManifest.xml`, `res/values/strings.xml`
- `android/app/build.gradle` (app id, no externalNativeBuild), `android/build_native.bat`
