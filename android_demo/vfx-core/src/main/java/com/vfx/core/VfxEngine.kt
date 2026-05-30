package com.vfx.core

import android.content.Context
import android.graphics.SurfaceTexture
import android.view.Surface

enum class CodecType(val value: Int) {
    H264(0),
    H265(1)
}

class VfxEngine {
    companion object {
        init {
            System.loadLibrary("vfx-jni")
        }
    }

    private var cameraSurfaceTexture: SurfaceTexture? = null
    var onCameraSurfaceReady: ((SurfaceTexture) -> Unit)? = null

    // Passed down from ActivityManager.deviceConfigurationInfo.reqGlEsVersion
    // e.g. 0x00030002 for GLES 3.2
    external fun init(glesVersionHex: Int, isVulkanSupported: Boolean)

    external fun setSurface(surface: Surface?)

    external fun generateCameraTexture()
    external fun notifyCameraFrameAvailable()

    external fun startRecording(outputPath: String, codecTypeInt: Int)
    external fun stopRecording()
    external fun destroy()

    fun startRecording(outputPath: String, codecType: CodecType = CodecType.H264) {
        startRecording(outputPath, codecType.value)
    }

    private fun onCameraTextureGenerated(textureId: Int) {
        cameraSurfaceTexture = SurfaceTexture(textureId)
        onCameraSurfaceReady?.invoke(cameraSurfaceTexture!!)
    }

    private fun updateCameraTexture(): FloatArray? {
        val st = cameraSurfaceTexture ?: return null
        st.updateTexImage()
        val matrix = FloatArray(16)
        st.getTransformMatrix(matrix)
        return matrix
    }
}
