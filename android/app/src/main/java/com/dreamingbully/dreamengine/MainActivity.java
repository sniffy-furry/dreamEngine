package com.dreamingbully.dreamengine;

import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import android.net.Uri;
import android.provider.Settings;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Button;
import java.io.*;
import java.util.*;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;

public final class MainActivity extends Activity {
    static { System.loadLibrary("dream_engine_android"); }
    private static final int PICK_MODULE_DIR = 7001;
    private static native void nativeStart(String moduleDir, String pythonHome);
    private static native void nativeStop();
    private static native void nativeUpdate(double dt);
    private static native void nativeReload();
    private static native String nativeDrainLog();
    private final Handler tickHandler = new Handler(Looper.getMainLooper());
    private long lastTickNanos;
    private final Runnable tick = new Runnable() {
        @Override public void run() {
            if (isFinishing()) return;
            long now = System.nanoTime();
            double dt = lastTickNanos == 0 ? 1.0 / 60.0 : Math.min((now - lastTickNanos) * 1.0e-9, 0.25);
            lastTickNanos = now;
            nativeUpdate(dt);
            if (++logTicks % 30 == 0) appendLog(nativeDrainLog());
            tickHandler.postDelayed(this, 16);
        }
    };
    private File moduleDir;
    private int logTicks;
    private TextView logView;
    private android.widget.ScrollView logScroll;
    private final StringBuilder logText = new StringBuilder();
    private void appendLog(String t) {
        if (t == null || t.isEmpty() || logView == null) return;
        logText.append(t);
        if (logText.length() > 12000) logText.delete(0, logText.length() - 12000);
        logView.setText(logText);
        logScroll.post(() -> logScroll.fullScroll(android.view.View.FOCUS_DOWN));
    }

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        moduleDir = new File(getFilesDir(), "modules");
        if (!moduleDir.exists()) moduleDir.mkdirs();
        copyBundledModules();
        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "arm64-v8a";
        File pythonHome = new File(getFilesDir(), "python/" + abi);
        copyBundledPython(abi, pythonHome);
        LinearLayout root = new LinearLayout(this); root.setOrientation(LinearLayout.VERTICAL); root.setPadding(32,32,32,32);
        TextView status = new TextView(this); status.setText("DreamEngine\nHot-swappable C++ modules + Python scripts"); status.setTextSize(20f);
        Button importButton = new Button(this); importButton.setText("Import modules from Downloads"); importButton.setOnClickListener(v -> pickModuleDirectory());
        logView = new TextView(this); logView.setTextSize(11f); logView.setTypeface(android.graphics.Typeface.MONOSPACE);
        logScroll = new android.widget.ScrollView(this); logScroll.addView(logView);
        root.addView(status); root.addView(importButton);
        root.addView(logScroll, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));
        setContentView(root);
        nativeStart(moduleDir.getAbsolutePath(), pythonHome.getAbsolutePath());
        appendLog(nativeDrainLog());
        lastTickNanos = System.nanoTime();
        tickHandler.post(tick);
    }
    private void copyBundledModules() {
        try {
            String[] names = getAssets().list("modules");
            if (names == null) return;
            for (String name : names) {
                File out = new File(moduleDir, name);
                if (out.exists()) continue;
                try (InputStream in = getAssets().open("modules/" + name); OutputStream os = new FileOutputStream(out)) {
                    byte[] b = new byte[8192]; int n; while ((n=in.read(b))!=-1) os.write(b,0,n);
                }
            }
        } catch (IOException ignored) {}
    }

    private void copyBundledPython(String abi, File destination) {
        try {
            if (destination.exists() && new File(destination, "pyvenv.cfg").exists()) return;
            copyAssetTree("python/" + abi, destination);
        } catch (IOException ignored) {}
    }

    private void copyAssetTree(String assetPath, File outDir) throws IOException {
        if (!outDir.exists() && !outDir.mkdirs()) throw new IOException("Cannot create " + outDir);
        String[] children = getAssets().list(assetPath);
        if (children == null || children.length == 0) {
            File out = new File(outDir, assetPath.substring(assetPath.lastIndexOf('/') + 1));
            try (InputStream in = getAssets().open(assetPath); OutputStream os = new FileOutputStream(out)) {
                byte[] b = new byte[16384]; int n; while ((n = in.read(b)) != -1) os.write(b, 0, n);
            }
            return;
        }
        for (String child : children) copyAssetTree(assetPath + "/" + child, new File(outDir, child));
    }

    private void pickModuleDirectory() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(i, PICK_MODULE_DIR);
    }
    @Override protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode,resultCode,data);
        if (requestCode != PICK_MODULE_DIR || resultCode != RESULT_OK || data == null) return;
        Uri tree = data.getData();
        try { getContentResolver().takePersistableUriPermission(tree, Intent.FLAG_GRANT_READ_URI_PERMISSION); } catch (Exception ignored) {}
        int copied = copyFromTree(tree);
        appendLog("imported " + copied + " file(s) (.py/.so, searched all subfolders)\n");
        if (copied > 0) nativeReload();   // live hot-swap, no restart needed
        appendLog(nativeDrainLog());
    }
    private int copyFromTree(Uri tree) {
        return walkTree(tree, android.provider.DocumentsContract.getTreeDocumentId(tree), 0);
    }
    private int walkTree(Uri tree, String docId, int depth) {
        if (depth > 6) return 0;
        int copied = 0;
        Uri children = android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree, docId);
        String[] projection = {
            android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            android.provider.DocumentsContract.Document.COLUMN_MIME_TYPE};
        try (android.database.Cursor c = getContentResolver().query(children, projection, null, null, null)) {
            if (c == null) return 0;
            while (c.moveToNext()) {
                String id = c.getString(0), name = c.getString(1), mime = c.getString(2);
                if (name == null) continue;
                if (android.provider.DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                    if (name.startsWith(".") || name.equals("Android")) continue;
                    copied += walkTree(tree, id, depth + 1);
                    continue;
                }
                if (!name.endsWith(".so") && !name.endsWith(".py")) continue;
                Uri fileUri = android.provider.DocumentsContract.buildDocumentUriUsingTree(tree, id);
                File tmp = new File(moduleDir, name + ".tmp");
                File out = new File(moduleDir, name);
                try (InputStream in = getContentResolver().openInputStream(fileUri); OutputStream os = new FileOutputStream(tmp)) {
                    byte[] b = new byte[8192]; int n; while ((n = in.read(b)) != -1) os.write(b, 0, n);
                    os.flush();
                    // rename = safe even if the old .so is still mapped
                    if (out.exists()) out.delete();
                    if (tmp.renameTo(out)) copied++;
                    appendLog("  copied " + name + "\n");
                } catch (Exception e) {
                    appendLog("  FAILED " + name + ": " + e + "\n");
                    tmp.delete();
                }
            }
        }
        return copied;
    }
    @Override protected void onDestroy() { tickHandler.removeCallbacks(tick); nativeStop(); super.onDestroy(); }
}
