package com.vfx.preview

import android.content.Context
import android.graphics.SurfaceTexture
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.TextureView
import android.widget.FrameLayout
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.vfx.core.VfxEngine
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class PreviewComponent @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private val vfxEngine = VfxEngine()
    private val textureView = TextureView(context)
    private var cameraExecutor: ExecutorService = Executors.newSingleThreadExecutor()

    private var isCameraStarted = false

    init {
        addView(textureView)
        vfxEngine.init()

        textureView.surfaceTextureListener = object : TextureView.SurfaceTextureListener {
            override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
                // The main window surface for the engine to render INTO
                vfxEngine.setSurface(Surface(surface))

                // Request C++ to generate a real texture ID first
                vfxEngine.onCameraSurfaceReady = { surfaceTexture ->
                    startCamera(surfaceTexture)
                }
                vfxEngine.generateCameraTexture()
            }

            override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {}

            override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
                vfxEngine.setSurface(null)
                return true
            }

            override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}
        }
    }

    private fun startCamera(surfaceTexture: SurfaceTexture) {
        if (isCameraStarted) return

        val lifecycleOwner = context as? LifecycleOwner
        if (lifecycleOwner == null) {
            Log.e("PreviewComponent", "Context is not a LifecycleOwner")
            return
        }

        val cameraProviderFuture = ProcessCameraProvider.getInstance(context)

        cameraProviderFuture.addListener({
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            val preview = Preview.Builder().build().also {
                it.setSurfaceProvider { request ->

                    surfaceTexture.setDefaultBufferSize(request.resolution.width, request.resolution.height)
                    val surface = Surface(surfaceTexture)

                    surfaceTexture.setOnFrameAvailableListener {
                        vfxEngine.notifyCameraFrameAvailable()
                    }

                    request.provideSurface(surface, cameraExecutor) {
                        surface.release()
                        // Don't release surfaceTexture here, the engine owns it
                    }
                }
            }

            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            try {
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(lifecycleOwner, cameraSelector, preview)
                isCameraStarted = true
            } catch(exc: Exception) {
                Log.e("PreviewComponent", "Use case binding failed", exc)
            }

        }, ContextCompat.getMainExecutor(context))
    }

    fun getEngine(): VfxEngine = vfxEngine

    fun onDestroy() {
        cameraExecutor.shutdown()
        vfxEngine.destroy()
    }
}
