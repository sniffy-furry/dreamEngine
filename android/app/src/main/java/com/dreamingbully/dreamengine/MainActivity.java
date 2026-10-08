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

public final class MainActivity extends Activity {
    static { System.loadLibrary("dream_engine_android"); }
    private static final int PICK_MODULE_DIR = 7001;
    private static native void nativeStart(String moduleDir);
    private static native void nativeStop();
    private File moduleDir;

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        moduleDir = new File(getFilesDir(), "modules");
        if (!moduleDir.exists()) moduleDir.mkdirs();
        copyBundledModules();
        LinearLayout root = new LinearLayout(this); root.setOrientation(LinearLayout.VERTICAL); root.setPadding(32,32,32,32);
        TextView status = new TextView(this); status.setText("DreamEngine\nHot-swappable C++ modules + Python scripts"); status.setTextSize(20f);
        Button importButton = new Button(this); importButton.setText("Import modules from Downloads"); importButton.setOnClickListener(v -> pickModuleDirectory());
        root.addView(status); root.addView(importButton); setContentView(root);
        nativeStart(moduleDir.getAbsolutePath());
    }
    private void copyBundledModules() {
        try {
            String[] names = getAssets().list("modules");
            if (names == null) return;
            for (String name : names) {
                File out = new File(moduleDir, name);
                try (InputStream in = getAssets().open("modules/" + name); OutputStream os = new FileOutputStream(out)) {
                    byte[] b = new byte[8192]; int n; while ((n=in.read(b))!=-1) os.write(b,0,n);
                }
            }
        } catch (IOException ignored) {}
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
        copyFromTree(tree);
        // The current process keeps old modules loaded. Restart the app after import for a clean hot swap.
    }
    private void copyFromTree(Uri tree) {
        android.database.Cursor c = null;
        // Use DocumentsContract URIs directly to avoid a support-library dependency.
        Uri children = android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree, android.provider.DocumentsContract.getTreeDocumentId(tree));
        String[] projection = {android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID, android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME};
        c = getContentResolver().query(children, projection, null, null, null);
        if (c == null) return;
        int idCol=c.getColumnIndex(projection[0]), nameCol=c.getColumnIndex(projection[1]);
        while(c.moveToNext()) {
            String id=c.getString(idCol), name=c.getString(nameCol);
            if (!name.endsWith(".so") && !name.endsWith(".py")) continue;
            Uri fileUri=android.provider.DocumentsContract.buildDocumentUriUsingTree(tree,id);
            File out=new File(moduleDir,name);
            try(InputStream in=getContentResolver().openInputStream(fileUri); OutputStream os=new FileOutputStream(out)) {
                byte[] b=new byte[8192]; int n; while((n=in.read(b))!=-1) os.write(b,0,n);
            } catch(Exception ignored) {}
        }
        c.close();
    }
    @Override protected void onDestroy() { nativeStop(); super.onDestroy(); }
}
