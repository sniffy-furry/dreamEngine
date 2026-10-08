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
    private static native void nativeSurfaceChanged(android.view.Surface surface, int w, int h);
    private static native void nativeSurfaceDestroyed();
    private static native void nativeRender(double dt);
    private static native int nativeUiRevision();
    private static native String nativeUiLayout();
    private static native float nativeGetProp(int id);
    private static native void nativeSetProp(int id, float v);
    private final Handler tickHandler = new Handler(Looper.getMainLooper());
    private long lastTickNanos;
    private final Runnable tick = new Runnable() {
        @Override public void run() {
            if (isFinishing()) return;
            long now = System.nanoTime();
            double dt = lastTickNanos == 0 ? 1.0 / 60.0 : Math.min((now - lastTickNanos) * 1.0e-9, 0.25);
            lastTickNanos = now;
            nativeUpdate(dt);
            nativeRender(dt);
            if (++logTicks % 30 == 0) { appendLog(nativeDrainLog()); syncUi(); }
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
        LinearLayout split = new LinearLayout(this); split.setOrientation(LinearLayout.HORIZONTAL);
        android.view.SurfaceView surfaceView = new android.view.SurfaceView(this);
        surfaceView.getHolder().addCallback(new android.view.SurfaceHolder.Callback() {
            @Override public void surfaceCreated(android.view.SurfaceHolder h) {}
            @Override public void surfaceChanged(android.view.SurfaceHolder h, int f, int w, int hh) { nativeSurfaceChanged(h.getSurface(), w, hh); }
            @Override public void surfaceDestroyed(android.view.SurfaceHolder h) { nativeSurfaceDestroyed(); }
        });
        LinearLayout root = new LinearLayout(this); root.setOrientation(LinearLayout.VERTICAL); root.setPadding(24,24,24,24);
        TextView status = new TextView(this); status.setText("DreamEngine\nHot-swappable C++ modules + Python scripts"); status.setTextSize(20f);
        Button importButton = new Button(this); importButton.setText("Import modules from Downloads"); importButton.setOnClickListener(v -> pickModuleDirectory());
        logView = new TextView(this); logView.setTextSize(11f); logView.setTypeface(android.graphics.Typeface.MONOSPACE);
        logScroll = new android.widget.ScrollView(this); logScroll.addView(logView);
        tabBar = new LinearLayout(this); tabBar.setOrientation(LinearLayout.HORIZONTAL);
        android.widget.HorizontalScrollView tabScroll = new android.widget.HorizontalScrollView(this); tabScroll.addView(tabBar);
        panelBox = new LinearLayout(this); panelBox.setOrientation(LinearLayout.VERTICAL);
        android.widget.ScrollView panelScroll = new android.widget.ScrollView(this); panelScroll.addView(panelBox);
        root.addView(status); root.addView(importButton); root.addView(tabScroll);
        root.addView(panelScroll, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));
        root.addView(logScroll, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));
        split.addView(surfaceView, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1f));
        split.addView(root, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1f));
        setContentView(split);
        nativeStart(moduleDir.getAbsolutePath(), pythonHome.getAbsolutePath());
        appendLog(nativeDrainLog());
        syncUi();
        lastTickNanos = System.nanoTime();
        tickHandler.post(tick);
    }

    // ---------- Minimal Android panel host: reads the engine's UI model, draws plain views ----------
    private LinearLayout tabBar, panelBox;
    private int uiRevSeen = -1, curPanel = 0;
    private final ArrayList<String> panelNames = new ArrayList<>();
    private final ArrayList<ArrayList<String[]>> panelRows = new ArrayList<>();
    private static final class Live { int id; char kind; float min, max, shown = Float.NaN; android.view.View view; TextView text; String label; }
    private final ArrayList<Live> live = new ArrayList<>();

    private void syncUi() {
        int rev = nativeUiRevision();
        if (rev != uiRevSeen) { uiRevSeen = rev; rebuildUi(); return; }
        for (Live l : live) {                       // only widgets on the visible panel
            float v = nativeGetProp(l.id);
            if (v == l.shown) continue;
            l.shown = v;
            if (l.kind == 'S') { ((android.widget.SeekBar) l.view).setProgress(Math.round((v - l.min) / (l.max - l.min) * 1000f)); l.text.setText(l.label + ": " + String.format("%.2f", v)); }
            else if (l.kind == 'T') ((android.widget.Switch) l.view).setChecked(v != 0f);
            else if (l.kind == 'V') l.text.setText(l.label + ": " + String.format("%.2f", v));
        }
    }

    private void rebuildUi() {
        panelNames.clear(); panelRows.clear();
        for (String line : nativeUiLayout().split("\n")) {
            if (line.isEmpty()) continue;
            String[] f = line.split("\t", -1);
            if (f[0].equals("P")) { panelNames.add(f[1]); panelRows.add(new ArrayList<>()); }
            else if (!panelRows.isEmpty()) panelRows.get(panelRows.size() - 1).add(f);
        }
        tabBar.removeAllViews();
        for (int i = 0; i < panelNames.size(); i++) {
            Button b = new Button(this); b.setText(panelNames.get(i));
            final int idx = i; b.setOnClickListener(v -> showPanel(idx));
            tabBar.addView(b);
        }
        if (curPanel >= panelNames.size()) curPanel = 0;
        showPanel(curPanel);
    }

    private void showPanel(int idx) {
        curPanel = idx; panelBox.removeAllViews(); live.clear();
        if (idx >= panelRows.size()) return;
        for (String[] f : panelRows.get(idx)) {
            TextView t = new TextView(this);
            if (f[0].equals("L")) { t.setText(f[1]); panelBox.addView(t); continue; }
            final Live l = new Live(); l.id = Integer.parseInt(f[1]); l.kind = f[0].charAt(0); l.label = f[2]; l.text = t;
            if (l.kind == 'S') {
                l.min = Float.parseFloat(f[3]); l.max = Float.parseFloat(f[4]);
                android.widget.SeekBar sb = new android.widget.SeekBar(this); sb.setMax(1000); l.view = sb;
                sb.setOnSeekBarChangeListener(new android.widget.SeekBar.OnSeekBarChangeListener() {
                    @Override public void onProgressChanged(android.widget.SeekBar s, int p, boolean fromUser) {
                        if (!fromUser) return;
                        float v = l.min + p / 1000f * (l.max - l.min);
                        nativeSetProp(l.id, v); l.shown = nativeGetProp(l.id);
                        l.text.setText(l.label + ": " + String.format("%.2f", l.shown));
                    }
                    @Override public void onStartTrackingTouch(android.widget.SeekBar s) {}
                    @Override public void onStopTrackingTouch(android.widget.SeekBar s) {}
                });
                panelBox.addView(t); panelBox.addView(sb);
            } else if (l.kind == 'T') {
                android.widget.Switch sw = new android.widget.Switch(this); sw.setText(l.label); l.view = sw;
                sw.setOnCheckedChangeListener((b, on) -> { nativeSetProp(l.id, on ? 1f : 0f); l.shown = nativeGetProp(l.id); });
                panelBox.addView(sw);
            } else {   // 'V' read-only
                panelBox.addView(t);
            }
            live.add(l);
        }
        uiRevSeen = nativeUiRevision();
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

    private static final String PY_MARKER = ".extracted_v2";

    private void copyBundledPython(String abi, File destination) {
        try {
            File marker = new File(destination, PY_MARKER);
            if (marker.isFile()) return;
            // Old/partial/broken extraction (previous version nested every file in a same-named folder): wipe it.
            deleteRecursive(destination);
            copyAssetTree("python/" + abi, destination);
            if (!new File(destination, "lib/python3.14/encodings").isDirectory())
                throw new IOException("stdlib missing after extraction (is the asset staged?)");
            try (OutputStream os = new FileOutputStream(marker)) { os.write('1'); }
        } catch (IOException e) {
            android.util.Log.e("DreamEngine", "python extract failed: " + e);
        }
    }

    private static void deleteRecursive(File f) {
        File[] kids = f.listFiles();
        if (kids != null) for (File k : kids) deleteRecursive(k);
        f.delete();
    }

    // `target` is the destination path for this asset (file OR directory).
    private void copyAssetTree(String assetPath, File target) throws IOException {
        String[] children = getAssets().list(assetPath);
        if (children != null && children.length > 0) {
            if (!target.isDirectory() && !target.mkdirs()) throw new IOException("Cannot create " + target);
            for (String child : children) copyAssetTree(assetPath + "/" + child, new File(target, child));
            return;
        }
        File parent = target.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) throw new IOException("Cannot create " + parent);
        try (InputStream in = getAssets().open(assetPath); OutputStream os = new FileOutputStream(target)) {
            byte[] b = new byte[16384]; int n; while ((n = in.read(b)) != -1) os.write(b, 0, n);
        } catch (FileNotFoundException e) {
            target.mkdirs(); // empty directory
        }
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
        syncUi();
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
