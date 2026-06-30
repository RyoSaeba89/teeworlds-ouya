package org.teeworlds.ouya;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

/**
 * OUYA launcher entry point.
 *
 * On a fresh install (or after a versionCode bump) the bundled Teeworlds data/
 * tree — ~28 MB across ~660 small files — is unpacked from the APK assets to
 * external storage, which takes a couple of minutes on the OUYA's slow flash.
 * Doing that synchronously inside SDLActivity.onCreate (as TeeworldsActivity used
 * to) blocks the UI thread the whole time: the user just sees a BLACK SCREEN for
 * minutes and Android may trip its ANR watchdog and force-finish the game.
 *
 * So this lightweight Activity shows a "Loading…" screen, runs the export on a
 * background thread, and only starts the real (SDL) TeeworldsActivity once the
 * data is on disk. On subsequent launches the export is stamped/skipped and this
 * screen flashes by in well under a second.
 *
 * This activity also carries the launcher / OUYA "GAME" categories so it is what
 * shows up (with the ouya_icon tile) in the OUYA games menu.
 */
public class LoaderActivity extends Activity {

    private static final String TAG = "teeworlds";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(buildLoadingView());

        int versionCode = 1;
        try {
            versionCode = getPackageManager()
                .getPackageInfo(getPackageName(), 0).versionCode;
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "versionCode lookup failed", e);
        }
        final int finalVersionCode = versionCode;

        new Thread(new Runnable() {
            @Override
            public void run() {
                // export() is versioned + idempotent: copies on first launch,
                // returns immediately once the stamp matches.
                AssetExporter.export(LoaderActivity.this, finalVersionCode);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        startActivity(new Intent(LoaderActivity.this, TeeworldsActivity.class));
                        finish();
                    }
                });
            }
        }, "asset-export").start();
    }

    private ViewGroup buildLoadingView() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setBackgroundColor(Color.BLACK);
        root.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        TextView title = new TextView(this);
        title.setText("Teeworlds");
        title.setTextColor(Color.WHITE);
        title.setTextSize(34);
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        ProgressBar spinner = new ProgressBar(this); // default = indeterminate spinner
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        sp.topMargin = 48;
        sp.bottomMargin = 24;
        spinner.setLayoutParams(sp);
        root.addView(spinner);

        TextView sub = new TextView(this);
        sub.setText("Unpacking game data…\n(first launch only, please wait)");
        sub.setTextColor(Color.LTGRAY);
        sub.setTextSize(16);
        sub.setGravity(Gravity.CENTER);
        root.addView(sub);

        return root;
    }
}
