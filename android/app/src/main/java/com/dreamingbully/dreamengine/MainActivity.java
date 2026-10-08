package com.dreamingbully.dreamengine;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public final class MainActivity extends Activity {
    static {
        System.loadLibrary("dream_engine_android");
    }

    private static native void nativeStart();
    private static native void nativeStop();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        TextView view = new TextView(this);
        view.setText("DreamEngine\nC++ core loaded");
        view.setTextSize(24f);
        view.setPadding(48, 48, 48, 48);
        setContentView(view);

        nativeStart();
    }

    @Override
    protected void onDestroy() {
        nativeStop();
        super.onDestroy();
    }
}
