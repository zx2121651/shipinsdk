package com.vfx.core

import android.view.Surface

class VfxEngine {
    companion object {
        init {
            System.loadLibrary("vfx-jni")
        }
    }

    external fun init()
    external fun setSurface(surface: Surface?)

    // New JNI interfaces for Camera feed bridging
    external fun setCameraTexture(textureId: Int, width: Int, height: Int)
    external fun notifyCameraFrameAvailable()

    external fun startRecording(outputPath: String)
    external fun stopRecording()
    external fun destroy()
}
