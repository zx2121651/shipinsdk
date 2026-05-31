package com.vfx.preview

import android.app.ActivityManager
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.SurfaceTexture
import android.util.Log
import android.view.Surface
import android.view.TextureView
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import com.vfx.core.VfxEngine
import java.util.concurrent.Executors

@Composable
fun VfxPreviewView(
    engine: VfxEngine,
    modifier: Modifier = Modifier
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val cameraExecutor = remember { Executors.newSingleThreadExecutor() }

    // Init Engine
    DisposableEffect(Unit) {
        val activityManager = context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
        val glesVersion = activityManager.deviceConfigurationInfo.reqGlEsVersion
        val isVulkanSupported = context.packageManager.hasSystemFeature(PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL)
        engine.init(glesVersion, isVulkanSupported)

        onDispose {
            cameraExecutor.shutdown()
            engine.destroy()
        }
    }

    AndroidView(
        modifier = modifier.fillMaxSize(),
        factory = { ctx ->
            TextureView(ctx).apply {
                surfaceTextureListener = object : TextureView.SurfaceTextureListener {
                    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
                        engine.setSurface(Surface(surface))
                        engine.onCameraSurfaceReady = { surfaceTexture ->
                            // Bind CameraX
                            val cameraProviderFuture = ProcessCameraProvider.getInstance(ctx)
                            cameraProviderFuture.addListener({
                                val cameraProvider = cameraProviderFuture.get()
                                val preview = Preview.Builder().build().also {
                                    it.setSurfaceProvider { request ->
                                        engine.setCameraTextureSize(request.resolution.width, request.resolution.height)
                                        surfaceTexture.setDefaultBufferSize(request.resolution.width, request.resolution.height)
                                        val cameraSurface = Surface(surfaceTexture)

                                        surfaceTexture.setOnFrameAvailableListener {
                                            engine.notifyCameraFrameAvailable()
                                        }

                                        request.provideSurface(cameraSurface, cameraExecutor) {
                                            cameraSurface.release()
                                        }
                                    }
                                }

                                try {
                                    cameraProvider.unbindAll()
                                    cameraProvider.bindToLifecycle(lifecycleOwner, CameraSelector.DEFAULT_BACK_CAMERA, preview)
                                } catch (exc: Exception) {
                                    Log.e("VfxPreviewView", "Camera binding failed", exc)
                                }
                            }, ContextCompat.getMainExecutor(ctx))
                        }
                        engine.generateCameraTexture()
                    }

                    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {}
                    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
                        engine.setSurface(null)
                        return true
                    }
                    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}
                }
            }
        }
    )
}
