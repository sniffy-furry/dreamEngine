package com.dreamingbully.dreamengine

import android.app.Activity
import android.os.Bundle
import android.widget.TextView

class MainActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        System.loadLibrary("dream_engine_android")

        val view = TextView(this).apply {
            text = "DreamEngine\nC++ core loaded"
            textSize = 24f
            setPadding(48, 48, 48, 48)
        }
        setContentView(view)

        nativeStart()
    }

    override fun onDestroy() {
        nativeStop()
        super.onDestroy()
    }

    private external fun nativeStart()
    private external fun nativeStop()
}
