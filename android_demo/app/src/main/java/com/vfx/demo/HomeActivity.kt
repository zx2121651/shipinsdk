package com.vfx.demo

import android.content.Intent
import android.os.Bundle
import android.widget.LinearLayout
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class HomeActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_home)

        // Hero Button: Camera (Launches VFX Preview Engine)
        findViewById<LinearLayout>(R.id.btn_camera).setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            startActivity(intent)
        }

        // Hero Button: New Project (Stub)
        findViewById<LinearLayout>(R.id.btn_new_project).setOnClickListener {
            Toast.makeText(this, "Opening video editor...", Toast.LENGTH_SHORT).show()
        }

        // Quick Tools Stubs
        findViewById<LinearLayout>(R.id.ll_quick_tools).setOnClickListener {
            Toast.makeText(this, "Quick tools selected", Toast.LENGTH_SHORT).show()
        }

        // Bottom Nav Stubs
        findViewById<LinearLayout>(R.id.bottom_nav).setOnClickListener {
            Toast.makeText(this, "Navigation disabled in Demo", Toast.LENGTH_SHORT).show()
        }
    }
}
