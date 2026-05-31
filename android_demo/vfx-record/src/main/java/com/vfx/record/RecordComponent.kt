package com.vfx.record

import android.widget.Toast
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.vfx.core.CodecType
import com.vfx.core.VfxEngine
import java.io.File

@Composable
fun RecordPanel(engine: VfxEngine, modifier: Modifier = Modifier) {
    val context = LocalContext.current
    var isRecording by remember { mutableStateOf(false) }

    Column(
        modifier = modifier
            .fillMaxWidth()
            .padding(bottom = 32.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {

        // Mode Selector
        Row(
            modifier = Modifier.padding(bottom = 24.dp),
            horizontalArrangement = Arrangement.Center
        ) {
            Text("15s", color = Color.White.copy(alpha = 0.5f), fontSize = 14.sp, modifier = Modifier.padding(end = 24.dp))
            Text("60s", color = Color.White.copy(alpha = 0.5f), fontSize = 14.sp, modifier = Modifier.padding(end = 24.dp))
            Text("Video", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 14.sp)
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Effects Button
            Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.clickable {
                Toast.makeText(context, "Effects (TODO)", Toast.LENGTH_SHORT).show()
            }) {
                Image(painterResource(android.R.drawable.ic_menu_camera), contentDescription = "Effects", modifier = Modifier.size(32.dp))
                Text("Effects", color = Color.White, fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp))
            }

            // Record Button
            Box(
                modifier = Modifier
                    .size(72.dp)
                    .clip(CircleShape)
                    .background(Color.White.copy(alpha = 0.3f))
                    .clickable {
                        if (isRecording) {
                            engine.stopRecording()
                            isRecording = false
                            Toast.makeText(context, "Saved to cache", Toast.LENGTH_SHORT).show()
                        } else {
                            val outputFile = File(context.cacheDir, "vfx_record_out.mp4")
                            engine.startRecording(outputFile.absolutePath, CodecType.H265)
                            isRecording = true
                        }
                    },
                contentAlignment = Alignment.Center
            ) {
                // Inner shape morphs based on recording state
                Box(
                    modifier = Modifier
                        .size(if (isRecording) 32.dp else 56.dp)
                        .clip(if (isRecording) RoundedCornerShape(8.dp) else CircleShape)
                        .background(Color.Red)
                )
            }

            // Upload/Gallery Button
            Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.clickable {
                Toast.makeText(context, "Upload (TODO)", Toast.LENGTH_SHORT).show()
            }) {
                Image(painterResource(android.R.drawable.ic_menu_gallery), contentDescription = "Upload", modifier = Modifier.size(32.dp))
                Text("Upload", color = Color.White, fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp))
            }
        }
    }
}
