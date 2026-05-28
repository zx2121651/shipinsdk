package com.vfx.demo

import android.os.Bundle
import android.widget.LinearLayout
import androidx.appcompat.app.AppCompatActivity
import com.vfx.preview.PreviewComponent
import com.vfx.record.RecordComponent

class MainActivity : AppCompatActivity() {

    private lateinit var previewComponent: PreviewComponent
    private lateinit var recordComponent: RecordComponent

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this)
        layout.orientation = LinearLayout.VERTICAL

        // Initialize UI components
        previewComponent = PreviewComponent(this)

        // Give preview some fixed layout params for demo
        val previewParams = LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 800
        )
        previewComponent.layoutParams = previewParams

        recordComponent = RecordComponent(this)

        // Wire up the engine instance
        recordComponent.attachEngine(previewComponent.getEngine())

        layout.addView(previewComponent)
        layout.addView(recordComponent)

        setContentView(layout)
    }

    override fun onDestroy() {
        super.onDestroy()
        previewComponent.onDestroy()
    }
}
