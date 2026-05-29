package com.vfx.core

import android.graphics.SurfaceTexture
import android.view.Surface

class VfxEngine {
    companion object {
        init {
            System.loadLibrary("vfx-jni")
        }
    }

    private var cameraSurfaceTexture: SurfaceTexture? = null
    var onCameraSurfaceReady: ((SurfaceTexture) -> Unit)? = null

    external fun init()
    external fun setSurface(surface: Surface?)

    // Generates texture ID asynchronously on RenderThread
    external fun generateCameraTexture()

    // Signals C++ to draw. C++ will call updateCameraTexture synchronously on its RenderThread.
    external fun notifyCameraFrameAvailable()

    external fun startRecording(outputPath: String)
    external fun stopRecording()
    external fun destroy()

    // Called from C++ RenderThread
    private fun onCameraTextureGenerated(textureId: Int) {
        cameraSurfaceTexture = SurfaceTexture(textureId)
        onCameraSurfaceReady?.invoke(cameraSurfaceTexture!!)
    }

    // Called from C++ RenderThread to update frame and get matrix safely
    private fun updateCameraTexture(): FloatArray? {
        val st = cameraSurfaceTexture ?: return null
        st.updateTexImage()
        val matrix = FloatArray(16)
        st.getTransformMatrix(matrix)
        return matrix
    }
}
