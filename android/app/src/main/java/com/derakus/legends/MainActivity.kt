package com.derakus.legends

import android.app.AlertDialog
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.ViewGroup
import android.widget.ProgressBar
import android.widget.TextView
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.lifecycleScope
import com.derakus.legends.databinding.ActivityMainBinding
import com.google.androidgamesdk.GameActivity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

private const val TAG = "MainActivity"

class MainActivity : GameActivity() {

    private lateinit var androidManager: AndroidManager
    private var extractionDialog: AlertDialog? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        Log.i(TAG, "onCreate: starting")
        val splashScreen = installSplashScreen()
        super.onCreate(savedInstanceState)
        Log.i(TAG, "onCreate: super.onCreate done")
        val binding = ActivityMainBinding.inflate(layoutInflater)
        findViewById<ViewGroup>(contentViewId).addView(binding.root)
        Log.i(TAG, "onCreate: view binding done")

        androidManager = AndroidManager(
            context = this,
            editText = binding.editText as FakeEditText,
            previewContainer = binding.keyboardPreviewContainer,
            previewText = binding.inputPreviewText,
        ).apply {
            nativeInit()
        }

        checkAndExtractAssets {
            androidManager.nativeSetAudioEnabled(true)
        }

        var lastImeVisible = false
        ViewCompat.setOnApplyWindowInsetsListener(binding.keyboardPreviewContainer) { view, insets ->
            val imeInsets = insets.getInsets(WindowInsetsCompat.Type.ime())
            val imeVisible = imeInsets.bottom > 0
            if (imeVisible != lastImeVisible) {
                lastImeVisible = imeVisible
                androidManager.onImeVisibilityChanged(imeVisible)
            }
            view.translationY = -imeInsets.bottom.toFloat()
            insets
        }

        hideSystemBars()
        Log.i(TAG, "onCreate: complete")
    }

    override fun onStart() {
        Log.i(TAG, "onStart: starting")
        super.onStart()
        Log.i(TAG, "onStart: complete")
    }

    override fun onResume() {
        Log.i(TAG, "onResume: starting")
        super.onResume()
        androidManager.nativeSetAudioEnabled(true)
        Log.i(TAG, "onResume: complete")
    }

    override fun onPause() {
        Log.i(TAG, "onPause: starting")
        androidManager.nativeSetAudioEnabled(false)
        super.onPause()
        Log.i(TAG, "onPause: complete")
    }

    override fun onStop() {
        Log.i(TAG, "onStop: starting")
        super.onStop()
        Log.i(TAG, "onStop: complete")
    }

    override fun onDestroy() {
        Log.i(TAG, "onDestroy: starting")
        androidManager.nativeSetAudioEnabled(false)
        super.onDestroy()
        Log.i(TAG, "onDestroy: complete")
    }

    private fun hideSystemBars() {
        val windowInsetsController = WindowCompat.getInsetsController(window, window.decorView)
        windowInsetsController.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        windowInsetsController.hide(WindowInsetsCompat.Type.systemBars())
    }

    private fun checkAndExtractAssets(onComplete: () -> Unit) {
        val initFile = File(filesDir, "game_data/init.lua")
        
        if (initFile.exists()) {
            onComplete()
            return
        }

        showExtractionDialog()

        lifecycleScope.launch(Dispatchers.IO) {
            var lastProgress = 0
            val progressCheckJob = launch {
                while (true) {
                    delay(500)
                    
                    val dataDir = File(filesDir, "game_data")
                    if (dataDir.exists()) {
                        val extractedSize = dataDir.walkTopDown()
                            .filter { it.isFile }
                            .map { it.length() }
                            .sum()
                        
                        val estimatedTotalSize = 350L * 1024 * 1024
                        val progress = ((extractedSize.toFloat() / estimatedTotalSize) * 100).toInt().coerceIn(0, 95)
                        
                        if (progress > lastProgress) {
                            lastProgress = progress
                            withContext(Dispatchers.Main) {
                                updateExtractionProgress(progress)
                            }
                        }
                    }
                }
            }

            delay(100)

            withContext(Dispatchers.Main) {
                onComplete()
            }

            delay(500)
            progressCheckJob.cancel()

            withContext(Dispatchers.Main) {
                updateExtractionProgress(100)
                delay(300)
                dismissExtractionDialog()
            }
        }
    }

    private fun showExtractionDialog() {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_extraction_progress, null)
        
        extractionDialog = AlertDialog.Builder(this, android.R.style.Theme_DeviceDefault_Dialog_NoActionBar_MinWidth)
            .setView(dialogView)
            .setCancelable(false)
            .create()
        
        extractionDialog?.window?.setBackgroundDrawableResource(android.R.color.transparent)
        extractionDialog?.show()
    }

    private fun updateExtractionProgress(progress: Int) {
        extractionDialog?.findViewById<ProgressBar>(R.id.progressBar)?.progress = progress
        extractionDialog?.findViewById<TextView>(R.id.tvExtractionProgress)?.text = "$progress%"
    }

    private fun dismissExtractionDialog() {
        extractionDialog?.dismiss()
        extractionDialog = null
    }

    companion object {
        init {
            System.loadLibrary("DerakusLegends")
        }
    }
}
