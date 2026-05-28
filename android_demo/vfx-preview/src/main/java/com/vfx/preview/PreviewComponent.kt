package com.vfx.preview

import android.content.Context
import android.util.AttributeSet
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.FrameLayout
import com.vfx.core.VfxEngine

class PreviewComponent @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr), SurfaceHolder.Callback {

    private val surfaceView = SurfaceView(context)
    private val vfxEngine = VfxEngine()

    init {
        addView(surfaceView)
        surfaceView.holder.addCallback(this)
        vfxEngine.init()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        vfxEngine.setSurface(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Engine handles resize via render thread logic.
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        vfxEngine.setSurface(null)
    }

    fun getEngine(): VfxEngine = vfxEngine

    fun onDestroy() {
        vfxEngine.destroy()
    }
}
