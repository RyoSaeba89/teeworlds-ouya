// Teeworlds -> OUYA: Android-specific native glue.
//
// gl4es reads its LIBGL_* configuration in a constructor(101) that runs at
// libGL load time (before main()). We must therefore export the environment
// from an *earlier* constructor(100). The critical one is LIBGL_NOTEST=1:
// gl4es' hardware-probe shaders use ES3/desktop GLSL syntax that makes the
// ancient Tegra 3 Cg compiler (libcgdrv.so) SIGSEGV on the real OUYA. NOTEST
// skips those probe shaders *and* gl4es' own EGL pbuffer probe context.
//
// (Same fix proven in the AssaultCube OUYA port.)

#include <stdlib.h>
#include <unistd.h>
#include "SDL.h"

__attribute__((constructor(100)))
static void tw_setup_gl4es_env(void)
{
    setenv("LIBGL_ES",      "2", 1);  // force the GLES2 backend
    setenv("LIBGL_NOTEST",  "1", 1);  // skip Tegra-Cg-crashing probe shaders
    setenv("LIBGL_NOHIGHP", "1", 1);  // Tegra fragment shaders: no highp guarantee
    setenv("LIBGL_NOPSA",   "1", 1);  // disable persistent shader assembly cache
    setenv("LIBGL_NOBANNER","1", 1);
    setenv("LIBGL_MIPMAP",  "3", 1);  // auto-generate + use mipmaps
    setenv("LIBGL_NOERROR", "1", 1);  // skip gl4es-internal glGetError checks
}

// Called from the top of Teeworlds main() (client.cpp) on Android.
//
// The Java AssetExporter has already unpacked the bundled assets/ (data/ +
// storage.cfg) into the app's external files dir. We point the engine there:
//  - chdir() so storage.cpp FindDataDir() finds "data/mapres" in the PWD and
//    so $CURRENTDIR (a writable path) resolves correctly;
//  - set HOME so fs_storage_path() / $USERDIR (the save path on UNIX) lands in
//    the same writable dir instead of an unwritable system location.
extern "C" void tw_android_setup(void)
{
    const char *pPath = SDL_AndroidGetExternalStoragePath();
    if(!pPath || !pPath[0])
        return;
    setenv("HOME", pPath, 1);
    if(chdir(pPath) != 0)
        SDL_Log("teeworlds: chdir(%s) failed", pPath);
    else
        SDL_Log("teeworlds: data/save base = %s", pPath);
}
