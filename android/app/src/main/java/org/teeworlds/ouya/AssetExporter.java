package org.teeworlds.ouya;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Unpacks the bundled assets/ tree (Teeworlds data/ + storage.cfg) into the
 * app's external files dir, where the native engine reads them via normal
 * filesystem I/O (Teeworlds' storage layer is fopen-based, so it cannot read
 * straight out of the APK). Re-extracts only when the app versionCode changes.
 */
public class AssetExporter {
    private static final String TAG = "teeworlds";
    private static final String VERSION_FILE = "exported_versioncode.txt";

    public static void export(Context ctx, int versionCode) {
        File base = ctx.getExternalFilesDir(null);
        if (base == null) {
            Log.e(TAG, "getExternalFilesDir returned null; cannot export assets");
            return;
        }
        File stamp = new File(base, VERSION_FILE);
        if (alreadyExported(stamp, versionCode)) {
            Log.i(TAG, "assets already exported for versionCode " + versionCode);
            return;
        }
        Log.i(TAG, "exporting assets to " + base.getAbsolutePath());
        AssetManager am = ctx.getAssets();
        try {
            // Everything under assets/data plus the loose config files.
            copyAsset(am, "data", base);
            copyAssetFileIfPresent(am, "storage.cfg", base);
            copyAssetFileIfPresent(am, "autoexec.cfg", base);
            writeStamp(stamp, versionCode);
            Log.i(TAG, "asset export complete");
        } catch (IOException e) {
            Log.e(TAG, "asset export failed", e);
        }
    }

    private static boolean alreadyExported(File stamp, int versionCode) {
        if (!stamp.exists()) return false;
        try {
            byte[] buf = new byte[16];
            InputStream in = new java.io.FileInputStream(stamp);
            int n = in.read(buf);
            in.close();
            if (n <= 0) return false;
            return Integer.parseInt(new String(buf, 0, n).trim()) == versionCode;
        } catch (Exception e) {
            return false;
        }
    }

    private static void writeStamp(File stamp, int versionCode) throws IOException {
        OutputStream out = new FileOutputStream(stamp);
        out.write(Integer.toString(versionCode).getBytes());
        out.close();
    }

    /** Recursively copy an asset path (dir or file) into destParent. */
    private static void copyAsset(AssetManager am, String assetPath, File destParent) throws IOException {
        String[] children = am.list(assetPath);
        File dest = new File(destParent, assetPath);
        if (children != null && children.length > 0) {
            // directory
            if (!dest.exists() && !dest.mkdirs())
                Log.w(TAG, "could not create dir " + dest);
            for (String child : children)
                copyAsset(am, assetPath + "/" + child, destParent);
        } else {
            copyFile(am, assetPath, dest);
        }
    }

    private static void copyAssetFileIfPresent(AssetManager am, String assetPath, File destParent) {
        try {
            copyFile(am, assetPath, new File(destParent, assetPath));
        } catch (IOException e) {
            // storage.cfg is optional; ignore if not bundled
        }
    }

    private static void copyFile(AssetManager am, String assetPath, File dest) throws IOException {
        File parent = dest.getParentFile();
        if (parent != null && !parent.exists()) parent.mkdirs();
        InputStream in = am.open(assetPath);
        OutputStream out = new BufferedOutputStream(new FileOutputStream(dest), 64 * 1024);
        byte[] buf = new byte[64 * 1024];
        int n;
        while ((n = in.read(buf)) > 0)
            out.write(buf, 0, n);
        out.flush();
        out.close();
        in.close();
    }
}
