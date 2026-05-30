package com.vfx.demo

import android.os.Bundle
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.constraintlayout.widget.ConstraintSet
import com.vfx.preview.PreviewComponent
import com.vfx.record.RecordComponent

class MainActivity : AppCompatActivity() {

    private lateinit var previewComponent: PreviewComponent
    private lateinit var recordComponent: RecordComponent

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Use ConstraintLayout for full-screen immersive composition
        val rootLayout = ConstraintLayout(this).apply {
            id = android.view.View.generateViewId()
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
        }

        // Initialize UI components
        previewComponent = PreviewComponent(this).apply {
            id = android.view.View.generateViewId()
        }

        recordComponent = RecordComponent(this).apply {
            id = android.view.View.generateViewId()
        }

        // Wire up the engine instance
        recordComponent.attachEngine(previewComponent.getEngine())

        rootLayout.addView(previewComponent)
        rootLayout.addView(recordComponent)

        // Setup constraints: Preview fills screen, Record floats at bottom
        val constraintSet = ConstraintSet()
        constraintSet.clone(rootLayout)

        // Preview constraints (MATCH_PARENT equivalent)
        constraintSet.connect(previewComponent.id, ConstraintSet.TOP, ConstraintSet.PARENT_ID, ConstraintSet.TOP)
        constraintSet.connect(previewComponent.id, ConstraintSet.BOTTOM, ConstraintSet.PARENT_ID, ConstraintSet.BOTTOM)
        constraintSet.connect(previewComponent.id, ConstraintSet.START, ConstraintSet.PARENT_ID, ConstraintSet.START)
        constraintSet.connect(previewComponent.id, ConstraintSet.END, ConstraintSet.PARENT_ID, ConstraintSet.END)
        constraintSet.constrainWidth(previewComponent.id, ConstraintSet.MATCH_CONSTRAINT)
        constraintSet.constrainHeight(previewComponent.id, ConstraintSet.MATCH_CONSTRAINT)

        // Record panel constraints (floating at bottom)
        constraintSet.connect(recordComponent.id, ConstraintSet.BOTTOM, ConstraintSet.PARENT_ID, ConstraintSet.BOTTOM, 100) // Bottom margin
        constraintSet.connect(recordComponent.id, ConstraintSet.START, ConstraintSet.PARENT_ID, ConstraintSet.START)
        constraintSet.connect(recordComponent.id, ConstraintSet.END, ConstraintSet.PARENT_ID, ConstraintSet.END)
        constraintSet.constrainWidth(recordComponent.id, ConstraintSet.WRAP_CONTENT)
        constraintSet.constrainHeight(recordComponent.id, ConstraintSet.WRAP_CONTENT)

        constraintSet.applyTo(rootLayout)

        setContentView(rootLayout)
    }

    override fun onDestroy() {
        super.onDestroy()
        previewComponent.onDestroy()
    }
}
