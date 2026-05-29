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
    private lateinit var cameraExecutor: ExecutorService

    private var isCameraStarted = false

    init {
        addView(textureView)
        vfxEngine.init()

        textureView.surfaceTextureListener = object : TextureView.SurfaceTextureListener {
            override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
                // The main window surface for the engine to render INTO
                vfxEngine.setSurface(Surface(surface))
                startCamera()
            }

            override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {}

            override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
                vfxEngine.setSurface(null)
                return true
            }

            override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}
        }

        cameraExecutor = Executors.newSingleThreadExecutor()
    }

    private fun startCamera() {
        if (isCameraStarted) return

        val lifecycleOwner = context as? LifecycleOwner
        if (lifecycleOwner == null) {
            Log.e("PreviewComponent", "Context is not a LifecycleOwner")
            return
        }

        val cameraProviderFuture = ProcessCameraProvider.getInstance(context)

        cameraProviderFuture.addListener({
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            // Setup CameraX Preview UseCase
            val preview = Preview.Builder().build().also {
                it.setSurfaceProvider { request ->
                    // 1. Create a SurfaceTexture that acts as the sink for Camera frames
                    // In a real engine, we'd generate a texture ID via GLES on the RenderThread first.
                    // For demo integration, we'll let CameraX provide it via SurfaceTexture (mocking the ID).

                    val surfaceTexture = SurfaceTexture(10) // Mock ID 10
                    surfaceTexture.setDefaultBufferSize(request.resolution.width, request.resolution.height)

                    val surface = Surface(surfaceTexture)

                    // We must notify our C++ engine about this camera texture
                    vfxEngine.setCameraTexture(10, request.resolution.width, request.resolution.height)

                    // Whenever camera updates the texture
                    surfaceTexture.setOnFrameAvailableListener {
                        vfxEngine.notifyCameraFrameAvailable()
                    }

                    request.provideSurface(surface, cameraExecutor) {
                        surface.release()
                        surfaceTexture.release()
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
