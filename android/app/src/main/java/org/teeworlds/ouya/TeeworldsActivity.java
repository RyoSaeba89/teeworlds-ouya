package org.teeworlds.ouya;

import org.libsdl.app.SDLActivity;

/**
 * Teeworlds 0.7.5 on the OUYA.
 *
 * Extends SDL's base activity and lists the native libraries explicitly in
 * dependency order - the API-16 dynamic linker does not resolve transitive
 * DT_NEEDED entries from the app lib dir (main must be last: SDL loads SDL_main
 * from it).
 *
 * The bundled game data is unpacked by {@link LoaderActivity} (on a background
 * thread, behind a loading screen) BEFORE this activity is started, so by the
 * time SDL's native thread spins up the data is already on disk. This activity
 * is no longer the launcher entry point.
 */
public class TeeworldsActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "c++_shared",
            "hidapi",
            "SDL2",
            "main"
        };
    }
}
