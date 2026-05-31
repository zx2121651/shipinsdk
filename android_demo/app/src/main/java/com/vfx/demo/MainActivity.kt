package com.vfx.demo

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ColorFilter
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.vfx.core.VfxEngine
import com.vfx.preview.VfxPreviewView
import com.vfx.record.RecordPanel

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            val navController = rememberNavController()

            NavHost(navController = navController, startDestination = "home") {
                composable("home") {
                    HomeScreen(
                        onNavigateToCamera = { navController.navigate("camera") }
                    )
                }
                composable("camera") {
                    CameraScreen(
                        onClose = { navController.popBackStack() }
                    )
                }
            }
        }
    }
}

@Composable
fun HomeScreen(onNavigateToCamera: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black)
    ) {
        // Top Header
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 48.dp, start = 24.dp, end = 24.dp, bottom = 16.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("CapCut Clone", color = Color.White, fontSize = 20.sp, fontWeight = FontWeight.Bold)
            Image(painterResource(R.drawable.ic_close), contentDescription = null, colorFilter = ColorFilter.tint(Color.White))
        }

        // Hero Actions
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Column(
                modifier = Modifier
                    .weight(1f)
                    .height(90.dp)
                    .background(Color(0xFF3665FF), RoundedCornerShape(12.dp)),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Image(painterResource(R.drawable.ic_new_project), contentDescription = null)
                Text("New project", color = Color.White, fontSize = 14.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(top = 8.dp))
            }

            Column(
                modifier = Modifier
                    .weight(1f)
                    .height(90.dp)
                    .background(Color(0xFF2C2C2E), RoundedCornerShape(12.dp))
                    .clickable { onNavigateToCamera() },
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Image(painterResource(R.drawable.ic_camera_hero), contentDescription = null)
                Text("Camera", color = Color.White, fontSize = 14.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(top = 8.dp))
            }
        }

        // Quick Tools
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 24.dp, vertical = 24.dp),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            QuickTool(R.drawable.ic_filter, "AutoCut")
            QuickTool(R.drawable.ic_effects, "Templates")
            QuickTool(R.drawable.ic_beauty, "Retouch")
            QuickTool(R.drawable.ic_gallery, "Captions")
        }

        Spacer(modifier = Modifier.height(1.dp).fillMaxWidth().background(Color(0xFF2C2C2E)).padding(vertical = 24.dp))

        // Projects Section
        Text("Projects", color = Color.White, fontSize = 16.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(start = 24.dp, top = 24.dp, bottom = 16.dp))

        Column(modifier = Modifier.weight(1f).padding(horizontal = 16.dp)) {
            DraftItem("My Vlog 05/28", "00:15 | 128 MB")
            Spacer(Modifier.height(8.dp))
            DraftItem("Travel.mp4", "01:20 | 450 MB")
        }

        // Bottom Navigation
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .height(64.dp)
                .background(Color(0xFF121212)),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            BottomNavItem(R.drawable.ic_edit, "Edit", true)
            BottomNavItem(R.drawable.ic_templates, "Templates", false)
            BottomNavItem(R.drawable.ic_inbox, "Inbox", false)
            BottomNavItem(R.drawable.ic_me, "Me", false)
        }
    }
}

@Composable
fun QuickTool(iconRes: Int, label: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Image(painterResource(iconRes), contentDescription = null, modifier = Modifier.size(24.dp))
        Text(label, color = Color(0xFFA0A0A0), fontSize = 11.sp, modifier = Modifier.padding(top = 8.dp))
    }
}

@Composable
fun DraftItem(title: String, subtitle: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(80.dp)
            .background(Color(0xFF1A1A1A), RoundedCornerShape(8.dp))
            .padding(8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(modifier = Modifier.size(64.dp).background(Color(0xFF333333)))
        Column(modifier = Modifier.padding(start = 12.dp)) {
            Text(title, color = Color.White, fontSize = 14.sp, fontWeight = FontWeight.Bold)
            Text(subtitle, color = Color(0xFF888888), fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp))
        }
    }
}

@Composable
fun BottomNavItem(iconRes: Int, label: String, isActive: Boolean) {
    val color = if (isActive) Color(0xFF3665FF) else Color(0xFF888888)
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Image(painterResource(iconRes), contentDescription = null, colorFilter = ColorFilter.tint(color), modifier = Modifier.size(24.dp))
        Text(label, color = color, fontSize = 10.sp, modifier = Modifier.padding(top = 4.dp))
    }
}

@Composable
fun CameraScreen(onClose: () -> Unit) {
    val engine = androidx.compose.runtime.remember { VfxEngine() }

    Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
        // Background Render Surface
        VfxPreviewView(engine = engine)

        // Top Controls
        Row(
            modifier = Modifier.fillMaxWidth().padding(top = 48.dp, start = 16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Image(
                painterResource(R.drawable.ic_close),
                contentDescription = "Close",
                modifier = Modifier.size(40.dp).padding(8.dp).clickable { onClose() }
            )
            Text(
                "Select Music",
                color = Color.White,
                modifier = Modifier.background(Color(0x40000000), RoundedCornerShape(16.dp)).padding(horizontal = 16.dp, vertical = 8.dp).weight(1f)
            )
            Spacer(Modifier.width(56.dp))
        }

        // Right Sidebar
        Column(
            modifier = Modifier.align(Alignment.TopEnd).padding(top = 48.dp, end = 16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            SidebarTool(R.drawable.ic_flip, "Flip")
            SidebarTool(R.drawable.ic_timer, "Timer")
            SidebarTool(R.drawable.ic_filter, "Filters")
            SidebarTool(R.drawable.ic_beauty, "Beauty")
        }

        // Floating Bottom Panel
        RecordPanel(
            engine = engine,
            modifier = Modifier.align(Alignment.BottomCenter)
        )
    }
}

@Composable
fun SidebarTool(iconRes: Int, label: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally, modifier = Modifier.padding(bottom = 16.dp)) {
        Image(painterResource(iconRes), contentDescription = null, modifier = Modifier.size(40.dp).padding(8.dp))
        Text(label, color = Color.White, fontSize = 10.sp)
    }
}
