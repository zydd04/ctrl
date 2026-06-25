package com.example.ctrl

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.graphics.Rect
import android.graphics.YuvImage
import android.os.Bundle
import android.util.Log
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.google.mlkit.vision.barcode.BarcodeScannerOptions
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.barcode.common.Barcode
import com.google.mlkit.vision.common.InputImage
import kotlinx.coroutines.*
import java.io.ByteArrayOutputStream
import java.io.OutputStream
import java.net.Socket
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {

    private lateinit var viewFinder: PreviewView
    private lateinit var ipInput: EditText
    private lateinit var statusText: TextView

    private lateinit var cameraExecutor: ExecutorService

    // Application States
    private var isScanning = true
    private var isStreaming = false

    // Networking
    private var socket: Socket? = null
    private var outputStream: OutputStream? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        viewFinder = findViewById(R.id.viewFinder)
        ipInput = findViewById(R.id.ipInput)
        statusText = findViewById(R.id.statusText)

        cameraExecutor = Executors.newSingleThreadExecutor()

        if (allPermissionsGranted()) {
            startCamera()
        } else {
            ActivityCompat.requestPermissions(this, REQUIRED_PERMISSIONS, REQUEST_CODE_PERMISSIONS)
        }
    }

    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener({
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            val preview = Preview.Builder()
                .build()
                .also {
                    it.setSurfaceProvider(viewFinder.surfaceProvider)
                }

            val imageAnalyzer = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()
                .also {
                    it.setAnalyzer(cameraExecutor, { imageProxy ->
                        processImageProxy(imageProxy)
                    })
                }

            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            try {
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(this, cameraSelector, preview, imageAnalyzer)
            } catch (exc: Exception) {
                Log.e(TAG, "Use case binding failed", exc)
            }

        }, ContextCompat.getMainExecutor(this))
    }

    @androidx.annotation.OptIn(androidx.camera.core.ExperimentalGetImage::class)
    private fun processImageProxy(imageProxy: ImageProxy) {
        val mediaImage = imageProxy.image
        if (mediaImage != null) {
            if (isScanning) {
                // Phase 1: Scan QR Code
                val image = InputImage.fromMediaImage(mediaImage, imageProxy.imageInfo.rotationDegrees)
                val options = BarcodeScannerOptions.Builder().setBarcodeFormats(Barcode.FORMAT_QR_CODE).build()
                val scanner = BarcodeScanning.getClient(options)

                scanner.process(image)
                    .addOnSuccessListener { barcodes ->
                        for (barcode in barcodes) {
                            barcode.rawValue?.let { apiKey ->
                                val ipAddress = ipInput.text.toString()
                                if (ipAddress.isNotEmpty()) {
                                    isScanning = false
                                    statusText.text = "Found API Key: $apiKey\nConnecting to Server..."
                                    connectAndStream(ipAddress, apiKey)
                                } else {
                                    runOnUiThread {
                                        Toast.makeText(this, "Please enter Server IP first", Toast.LENGTH_SHORT).show()
                                    }
                                }
                            }
                        }
                    }
                    .addOnCompleteListener {
                        imageProxy.close()
                    }
            } else if (isStreaming) {
                // Phase 2: Send camera frames to C++ backend
                try {
                    val yuvImage = YuvImage(
                        convertYUV420888ToNV21(imageProxy),
                        ImageFormat.NV21,
                        imageProxy.width,
                        imageProxy.height,
                        null
                    )
                    val stream = ByteArrayOutputStream()
                    yuvImage.compressToJpeg(Rect(0, 0, imageProxy.width, imageProxy.height), 50, stream)
                    val jpegByteArray = stream.toByteArray()

                    // Send raw bytes over TCP socket
                    outputStream?.write(jpegByteArray)
                    outputStream?.flush()

                } catch (e: Exception) {
                    Log.e(TAG, "Streaming error: ${e.message}")
                } finally {
                    imageProxy.close()
                }
            } else {
                imageProxy.close()
            }
        }
    }

    private fun connectAndStream(ipAddress: String, apiKey: String) {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                // Connect to the C++ server on port 1935
                socket = Socket(ipAddress, 1935)
                outputStream = socket?.getOutputStream()

                // Send the API key as the very first packet
                outputStream?.write((apiKey + "\n").toByteArray())
                outputStream?.flush()

                isStreaming = true
                withContext(Dispatchers.Main) {
                    statusText.text = "Streaming to $ipAddress:1935..."
                }

            } catch (e: Exception) {
                Log.e(TAG, "Connection failed: ${e.message}")
                withContext(Dispatchers.Main) {
                    statusText.text = "Connection Failed. Point at QR to retry."
                    isScanning = true
                }
            }
        }
    }

    // Helper to convert CameraX YUV format to NV21 for JPEG Compression
    private fun convertYUV420888ToNV21(image: ImageProxy): ByteArray {
        val yPlane = image.planes[0]
        val uPlane = image.planes[1]
        val vPlane = image.planes[2]

        val ySize = yPlane.buffer.remaining()
        val uSize = uPlane.buffer.remaining()
        val vSize = vPlane.buffer.remaining()

        val nv21 = ByteArray(ySize + ySize / 2)

        yPlane.buffer.get(nv21, 0, ySize)
        vPlane.buffer.get(nv21, ySize, vSize)
        uPlane.buffer.get(nv21, ySize + vSize, uSize)

        return nv21
    }

    private fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(baseContext, it) == PackageManager.PERMISSION_GRANTED
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
        socket?.close()
    }

    companion object {
        private const val TAG = "CtrlApp"
        private const val REQUEST_CODE_PERMISSIONS = 10
        private val REQUIRED_PERMISSIONS = arrayOf(Manifest.permission.CAMERA)
    }
}