package com.example.rccarcontroller

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothSocket
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.ListView
import android.widget.TextView
import android.widget.Toast
import android.view.MotionEvent
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.IOException
import java.io.OutputStream
import java.util.UUID

/**
 * Main activity for the RC Car Controller application.
 * Handles Bluetooth permissions, device discovery, connection, and command sending.
 */
class MainActivity : AppCompatActivity() {

    private var bluetoothAdapter: BluetoothAdapter? = null // System Bluetooth adapter.
    private val REQUEST_ENABLE_BT = 1 // Request code for enabling Bluetooth.
    private val REQUEST_PERMISSIONS_CODE = 101 // Request code for runtime permissions.
    private var mmSocket: BluetoothSocket? = null // Socket for Bluetooth communication.
    private var mmOutputStream: OutputStream? = null // Output stream to send commands.
    private val SPP_UUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB") // Standard SPP UUID.

    // UI Elements
    private lateinit var statusTextView: TextView
    private lateinit var connectButton: Button
    private lateinit var devicesListView: ListView
    private lateinit var forwardButton: Button
    private lateinit var backwardButton: Button
    private lateinit var leftButton: Button
    private lateinit var rightButton: Button
    private lateinit var stopButton: Button

    // Adapters and Lists for Bluetooth devices
    private var deviceListAdapter: ArrayAdapter<String>? = null // Adapter for the ListView of devices.
    private val discoveredDevicesList = ArrayList<String>() // List of discovered device names and addresses.
    private val bluetoothDevices = ArrayList<BluetoothDevice>() // List of discovered BluetoothDevice objects.

    /**
     * Array of required Bluetooth permissions, determined by Android SDK version.
     * For Android S (API 31) and above, BLUETOOTH_SCAN and BLUETOOTH_CONNECT are needed.
     * For older versions, BLUETOOTH and BLUETOOTH_ADMIN are used.
     * ACCESS_FINE_LOCATION is included as it can be required for Bluetooth scanning on some platform versions.
     */
    private val requiredPermissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT,
            Manifest.permission.ACCESS_FINE_LOCATION // Required for BT Scan on some versions
        )
    } else {
        arrayOf(
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN,
            Manifest.permission.ACCESS_FINE_LOCATION // Required for BT Scan on some versions
        )
    }

    /**
     * Called when the activity is first created.
     * Initializes UI elements, Bluetooth adapter, and sets up listeners.
     * @param savedInstanceState If the activity is being re-initialized after previously being shut down,
     * this Bundle contains the data it most recently supplied in onSaveInstanceState(Bundle). Otherwise it is null.
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        statusTextView = findViewById(R.id.statusTextView)
        connectButton = findViewById(R.id.connectButton)
        devicesListView = findViewById(R.id.devicesListView)
        forwardButton = findViewById(R.id.forwardButton)
        backwardButton = findViewById(R.id.backwardButton)
        leftButton = findViewById(R.id.leftButton)
        rightButton = findViewById(R.id.rightButton)
        stopButton = findViewById(R.id.stopButton)

        // Disable control buttons initially
        forwardButton.isEnabled = false
        backwardButton.isEnabled = false
        leftButton.isEnabled = false
        rightButton.isEnabled = false
        stopButton.isEnabled = false

        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter

        if (bluetoothAdapter == null) {
            statusTextView.text = "Status: Bluetooth not supported on this device"
            connectButton.isEnabled = false
            return
        }

        deviceListAdapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, discoveredDevicesList)
        devicesListView.adapter = deviceListAdapter
        devicesListView.onItemClickListener = AdapterView.OnItemClickListener { _, _, position, _ ->
            if (position < bluetoothDevices.size) {
                val device = bluetoothDevices[position]
                // Name check for safety, though we try to filter out null names earlier
                val deviceName = if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    // This check is mainly for safety, name should be available if it was added to the list
                    "Unknown Device (No permission)"
                } else {
                    device.name ?: "Unknown Device"
                }
                statusTextView.text = "Status: Selected $deviceName"
                connectToDevice(device)
                devicesListView.visibility = View.GONE // Hide list after selection
            }
        }

        connectButton.setOnClickListener {
            if (checkAndRequestPermissions()) {
                // discoverDevices() // listPairedDevices() is called first if BT is already on.
                                  // If BT is off, onActivityResult will trigger listPairedDevices/discover.
                                  // We will start discovery after listing paired or enabling BT.
                // For clarity, let's ensure discoverDevices is called directly if BT is on and permissions are good
                if (bluetoothAdapter?.isEnabled == true) {
                    discoverDevices()
                } else {
                    // Request to enable Bluetooth, discovery will follow in onActivityResult
                    val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
                            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT)
                        } else {
                            statusTextView.text = "Status: BLUETOOTH_CONNECT permission needed to enable Bluetooth."
                            // Re-trigger general permission check if somehow missed.
                            checkAndRequestPermissions()
                        }
                    } else {
                        startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT)
                    }
                }
            }
        }

        // Setup button listeners
        forwardButton.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> { sendCommand('F'); true }
                MotionEvent.ACTION_UP -> { sendCommand('S'); true }
                else -> false
            }
        }
        backwardButton.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> { sendCommand('B'); true }
                MotionEvent.ACTION_UP -> { sendCommand('S'); true }
                else -> false
            }
        }
        leftButton.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> { sendCommand('L'); true }
                MotionEvent.ACTION_UP -> { sendCommand('S'); true }
                else -> false
            }
        }
        rightButton.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> { sendCommand('R'); true }
                MotionEvent.ACTION_UP -> { sendCommand('S'); true }
                else -> false
            }
        }
        stopButton.setOnClickListener {
            sendCommand('S')
        }
    }

    /**
     * BroadcastReceiver for handling Bluetooth discovery events.
     * - [BluetoothDevice.ACTION_FOUND]: Triggered when a new device is discovered.
     *   Adds the device to the list if it has a name and is not already listed.
     *   Requires `BLUETOOTH_CONNECT` permission on API 31+ to get device name and address.
     * - [BluetoothAdapter.ACTION_DISCOVERY_FINISHED]: Triggered when device discovery is completed.
     *   Updates the status TextView.
     */
    private val discoveryReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action: String? = intent.action
            when (action) {
                BluetoothDevice.ACTION_FOUND -> {
                    val device: BluetoothDevice? = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
                    device?.let {
                        var deviceName = "Unknown Device"
                        var deviceAddress = "Unknown Address"
                        // Permission check for BLUETOOTH_CONNECT needed for device name and address on API 31+
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                            if (ActivityCompat.checkSelfPermission(this@MainActivity, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
                                deviceName = it.name ?: "Unnamed Device" // Use "Unnamed Device" if name is null
                                deviceAddress = it.address
                            } else {
                                // Cannot get name/address without BLUETOOTH_CONNECT on S+
                                // Log or update status, device might not be added or added with placeholder
                                statusTextView.text = "Status: Found device (name/address requires CONNECT perm)"
                                // Optionally, do not add the device or add with placeholder
                                // return // Exit if crucial info is missing
                            }
                        } else {
                             // No explicit BLUETOOTH_CONNECT needed for name/address on older APIs
                            deviceName = it.name ?: "Unnamed Device"
                            deviceAddress = it.address
                        }

                        // Add device to list if it has a valid name and is not already present (checked by address)
                        if (deviceName != "Unnamed Device" && !discoveredDevicesList.any { str -> str.contains(deviceAddress) }) {
                            val deviceEntry = "$deviceName\n$deviceAddress"
                            discoveredDevicesList.add(deviceEntry)
                            bluetoothDevices.add(it) // Keep the BluetoothDevice object
                            deviceListAdapter?.notifyDataSetChanged()
                        }
                    }
                }
                BluetoothAdapter.ACTION_DISCOVERY_FINISHED -> {
                    statusTextView.text = "Status: Discovery finished. Select a device or scan again."
                    // Optionally re-enable scan button if it was disabled during scan
                }
            }
        }
    }

    /**
     * Checks if all required Bluetooth permissions are granted.
     * If not, requests the missing permissions.
     * @return `true` if all permissions are already granted, `false` otherwise (and requests permissions).
     */
    private fun checkAndRequestPermissions(): Boolean {
        val permissionsToRequest = ArrayList<String>()
        for (permission in requiredPermissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                permissionsToRequest.add(permission)
            }
        }

        return if (permissionsToRequest.isNotEmpty()) {
            ActivityCompat.requestPermissions(this, permissionsToRequest.toTypedArray(), REQUEST_PERMISSIONS_CODE)
            false
        } else {
            true // All permissions are already granted
        }
    }

    /**
     * Callback for the result from requesting permissions.
     * This method is invoked for every call on [requestPermissions(String[], int)].
     * @param requestCode The request code passed in [requestPermissions(String[], int)].
     * @param permissions The requested permissions. Never null.
     * @param grantResults The grant results for the corresponding permissions which is either [PackageManager.PERMISSION_GRANTED] or [PackageManager.PERMISSION_DENIED]. Never null.
     */
    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_PERMISSIONS_CODE) {
            var allPermissionsGranted = true
            for (grantResult in grantResults) {
                if (grantResult != PackageManager.PERMISSION_GRANTED) {
                    allPermissionsGranted = false
                    break
                }
            }

            if (allPermissionsGranted) {
                statusTextView.text = "Status: Permissions granted. Ready to discover devices."
                // It's good practice to call discoverDevices() here or when the user clicks "Connect" again
                // For now, we assume the user will click "Connect" again if they were in the middle of that flow.
                // discoverDevices() // Or, let the user re-initiate via button.
            } else {
                statusTextView.text = "Status: Permissions denied. Cannot discover devices."
                Toast.makeText(this, "Bluetooth and Location permissions are required for this app to function.", Toast.LENGTH_LONG).show()
            }
        }
    }

    /**
     * Initiates Bluetooth device discovery.
     * This involves:
     * 1. Ensuring all necessary permissions are granted.
     * 2. Clearing previous discovery results.
     * 3. Listing currently paired devices.
     * 4. Registering a [BroadcastReceiver] to listen for `ACTION_FOUND` and `ACTION_DISCOVERY_FINISHED`.
     * 5. Starting discovery with `bluetoothAdapter?.startDiscovery()`.
     * Requires `BLUETOOTH_SCAN` permission on API 31+ to start discovery.
     * Requires `BLUETOOTH_ADMIN` on older versions.
     */
    private fun discoverDevices() {
        // Ensure all necessary permissions are granted before proceeding
        if (!checkAndRequestPermissions()) { // This will re-request if not granted.
            statusTextView.text = "Status: Required permissions missing for discovery."
            return
        }

        // Clear previous discovery results
        discoveredDevicesList.clear()
        bluetoothDevices.clear()
        deviceListAdapter?.notifyDataSetChanged()

        // List paired devices first
        listPairedDevices() // This will populate the lists with paired devices

        // Now, start discovery for new devices
        // BLUETOOTH_SCAN permission check for startDiscovery
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                statusTextView.text = "Status: BLUETOOTH_SCAN permission needed to discover new devices."
                // checkAndRequestPermissions() // Should have been called already by the button
                return
            }
        }
        // For older versions, BLUETOOTH_ADMIN (already part of requiredPermissions) covers startDiscovery.

        // Register the BroadcastReceiver for ACTION_FOUND and ACTION_DISCOVERY_FINISHED
        val filterFound = IntentFilter(BluetoothDevice.ACTION_FOUND)
        val filterFinished = IntentFilter(BluetoothAdapter.ACTION_DISCOVERY_FINISHED)
        registerReceiver(discoveryReceiver, filterFound)
        registerReceiver(discoveryReceiver, filterFinished)

        // If already discovering, cancel it first (requires BLUETOOTH_SCAN on S+)
        if (bluetoothAdapter?.isDiscovering == true) {
            // Permission for cancelDiscovery is implicitly BLUETOOTH_SCAN on S+
            // or BLUETOOTH_ADMIN on older versions.
            bluetoothAdapter?.cancelDiscovery()
        }

        // Start discovery (requires BLUETOOTH_SCAN on S+ or BLUETOOTH_ADMIN on older)
        if (bluetoothAdapter?.startDiscovery() == true) {
            statusTextView.text = "Status: Scanning for devices..."
        } else {
            statusTextView.text = "Status: Failed to start discovery."
            // Clean up receiver if discovery fails to start
            try { unregisterReceiver(discoveryReceiver) } catch (e: IllegalArgumentException) { /* Ignore if not registered */ }
        }
        devicesListView.visibility = View.VISIBLE // Show the list view for results
    }

    /**
     * Handles the result of an activity started for a result.
     * Currently used for the result of [BluetoothAdapter.ACTION_REQUEST_ENABLE].
     * @param requestCode The integer request code originally supplied to startActivityForResult(), allowing you to identify who this result came from.
     * @param resultCode The integer result code returned by the child activity through its setResult().
     * @param data An Intent, which can return result data to the caller (various data can be attached to Intent "extras").
     */
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == RESULT_OK) {
                statusTextView.text = "Status: Bluetooth enabled. Starting discovery..."
                discoverDevices() // Proceed to discover devices now that Bluetooth is enabled
            } else {
                statusTextView.text = "Status: Bluetooth enabling was cancelled by user."
                Toast.makeText(this, "Bluetooth must be enabled to connect to the car.", Toast.LENGTH_LONG).show()
            }
        }
    }

    /**
     * Populates the [devicesListView] with currently paired Bluetooth devices.
     * Requires `BLUETOOTH_CONNECT` permission on API 31+ to access `bondedDevices`.
     * On older versions, `BLUETOOTH` permission is sufficient.
     * This function is called as part of the [discoverDevices] flow to show paired devices immediately.
     */
    private fun listPairedDevices() {
        // BLUETOOTH_CONNECT permission check for bondedDevices on API 31+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            statusTextView.text = "Status: BLUETOOTH_CONNECT permission needed to list paired devices."
            // checkAndRequestPermissions() // Optionally re-request, but might be called by discoverDevices already.
            return
        }
        // For older versions, BLUETOOTH permission (already part of requiredPermissions) covers this.

        val localPairedDevices: Set<BluetoothDevice>? = bluetoothAdapter?.bondedDevices
        // Note: discoveredDevicesList and bluetoothDevices are not cleared here
        // as this function is additive, called during the broader discoverDevices process.

        localPairedDevices?.forEach { device ->
            val deviceName = device.name ?: "Unnamed Paired Device" // Handle null names
            val deviceAddress = device.address
            // Add to list only if not already present (checked by address to avoid duplicates from ongoing scan)
            if (!bluetoothDevices.any { it.address == deviceAddress }) {
                discoveredDevicesList.add("$deviceName\n$deviceAddress (Paired)")
                bluetoothDevices.add(device)
            }
        }
        deviceListAdapter?.notifyDataSetChanged() // Update the ListView

        if (bluetoothDevices.isEmpty()) {
            statusTextView.text = "Status: No paired devices found. Scanning for new devices..."
        } else {
            statusTextView.text = "Status: Listed paired devices. Scanning for new/other devices..."
        }
    }

    /**
     * Attempts to connect to the selected [BluetoothDevice].
     * This function performs the connection in a separate thread to avoid blocking the UI.
     * - Cancels any ongoing Bluetooth discovery.
     * - Creates a [BluetoothSocket] using a standard SPP UUID.
     * - Calls `connect()` on the socket.
     * - On success, initializes `mmOutputStream` and updates UI (enables control buttons).
     * - On failure, updates UI and closes the socket.
     * Requires `BLUETOOTH_SCAN` (API 31+) or `BLUETOOTH_ADMIN` (older) to cancel discovery.
     * Requires `BLUETOOTH_CONNECT` (API 31+) or `BLUETOOTH` (older) to connect.
     * @param device The [BluetoothDevice] to connect to.
     */
    private fun connectToDevice(device: BluetoothDevice) {
        // Cancel discovery as it's resource-intensive and can interfere with connections.
        // BLUETOOTH_SCAN permission check for cancelDiscovery on API 31+.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                statusTextView.text = "Status: BLUETOOTH_SCAN permission needed to cancel discovery before connecting."
                // Optionally, do not proceed or re-request permissions. For now, we'll try to proceed.
                // return // Or proceed with caution
            }
        }
        // For older versions, BLUETOOTH_ADMIN (already part of requiredPermissions) covers cancelDiscovery.
        bluetoothAdapter?.cancelDiscovery()

        val deviceName = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            "Selected Device" // Placeholder if name permission is missing (shouldn't happen if listed)
        } else {
            device.name ?: "Selected Device" // Use placeholder if name is somehow null
        }
        statusTextView.text = "Status: Connecting to $deviceName..."

        // BLUETOOTH_CONNECT permission check for createRfcommSocketToServiceRecord and connect on API 31+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            statusTextView.text = "Status: BLUETOOTH_CONNECT permission needed to establish connection."
            return
        }

        Thread {
            try {
                // Create a BluetoothSocket for the connection with the given BluetoothDevice
                mmSocket = device.createRfcommSocketToServiceRecord(SPP_UUID)
                mmSocket?.connect() // Blocking call: initiates connection.
                mmOutputStream = mmSocket?.outputStream // Get the output stream after connection.

                // Connection successful, update UI on the main thread
                runOnUiThread {
                    statusTextView.text = "Status: Connected to $deviceName"
                    devicesListView.visibility = View.GONE
                    connectButton.isEnabled = false // Disable "Connect" button, or change to "Disconnect"
                    // Enable control buttons now that we are connected
                    forwardButton.isEnabled = true
                    backwardButton.isEnabled = true
                    leftButton.isEnabled = true
                    rightButton.isEnabled = true
                    stopButton.isEnabled = true
                }
            } catch (e: IOException) {
                // Connection failed or an error occurred
                runOnUiThread {
                    statusTextView.text = "Status: Connection failed: ${e.message}"
                    // Ensure control buttons are disabled if connection fails
                    forwardButton.isEnabled = false
                    backwardButton.isEnabled = false
                    leftButton.isEnabled = false
                    rightButton.isEnabled = false
                    stopButton.isEnabled = false
                    connectButton.isEnabled = true // Re-enable "Connect" button
                }
                // Attempt to close the socket and stream
                try {
                    mmOutputStream?.close()
                    mmSocket?.close()
                } catch (closeException: IOException) {
                    // Log.e("MainActivity", "Could not close the client socket/stream on connection failure", closeException)
                }
                mmOutputStream = null // Nullify stream
                mmSocket = null       // Nullify socket
            }
        }.start() // Start the connection thread
    }

    /**
     * Sends a command character to the connected ESP32 device via Bluetooth.
     * Writes the byte representation of the character to the `mmOutputStream`.
     * Updates `statusTextView` if an error occurs or if not connected.
     * @param command The character command to send (e.g., 'F', 'B', 'L', 'R', 'S').
     */
    private fun sendCommand(command: Char) {
        if (mmOutputStream != null && mmSocket?.isConnected == true) {
            try {
                mmOutputStream?.write(command.code) // Send the character's byte value
                // Toast.makeText(this, "Sent: $command", Toast.LENGTH_SHORT).show() // Optional: for debugging
            } catch (e: IOException) {
                statusTextView.text = "Status: Error sending command: ${e.message}"
                // Consider a more robust error handling, e.g., trying to reconnect or reset UI to disconnected state.
                // For now, just log and update status. User might need to disconnect and reconnect.
                // UI reset could be:
                // forwardButton.isEnabled = false; backwardButton.isEnabled = false; ...
                // connectButton.isEnabled = true;
                // mmSocket?.close(); mmOutputStream?.close(); mmSocket = null; mmOutputStream = null;
            }
        } else {
            statusTextView.text = "Status: Not connected. Cannot send command."
            // Toast.makeText(this, "Not connected. Please connect to the car first.", Toast.LENGTH_SHORT).show()
        }
    }

    /**
     * Called when the activity is being destroyed.
     * Performs necessary cleanup, such as unregistering the [BroadcastReceiver]
     * and closing the Bluetooth socket and output stream.
     */
    override fun onDestroy() {
        super.onDestroy()
        // Unregister the BroadcastReceiver to avoid memory leaks
        try {
            unregisterReceiver(discoveryReceiver)
        } catch (e: IllegalArgumentException) {
            // Log.e("MainActivity", "Receiver not registered", e)
        }
        // Close the Bluetooth socket and stream
        try {
            mmOutputStream?.close()
        } catch (e: IOException) {
            // Log.e("MainActivity", "Could not close the output stream", e)
        }
        try {
            mmSocket?.close()
        } catch (e: IOException) {
            // Log.e("MainActivity", "Could not close the socket", e)
        }
    }
}
