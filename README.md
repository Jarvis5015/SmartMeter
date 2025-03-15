# Gateway Device Firmware

This branch contains the firmware for the Gateway device in our Smart Meter System. The gateway is designed to receive data over a LoRa network, process incoming measurement packets, and upload the validated data to a Firebase Realtime Database. This document explains how the code functions and guides you on how to configure, build, and deploy the firmware.

## What Does the Code Do?

### 1. Initialization
- **LoRa & WiFi Setup:**  
  The firmware initializes the LoRa module using defined pin assignments and frequency settings. It then connects to a WiFi network using your provided credentials.
- **Firebase Authentication:**  
  The device performs an anonymous sign-in with Firebase to retrieve an access token, which is later used for uploading data to the Realtime Database.
- **Receive Mode:**  
  After initialization, the gateway enters a receive mode, ready to listen for LoRa transmissions.

### 2. Broadcast & Data Collection
- **Broadcasting:**  
  The gateway continuously sends a "BROADCAST" message at regular intervals (e.g., every second) until it detects a response from a Node device.
- **Data Collection:**  
  Once a response is received, the gateway switches to data collection mode. It listens for incoming LoRa packets that contain measurement data.
- **Packet Validation:**  
  The firmware checks each received packet for:
  - **Header Validation:** The packet must start with a predefined header.
  - **Checksum Verification:** The integrity of the packet is confirmed by computing and comparing its checksum.

### 3. Data Processing & Upload
- **Data Conversion:**  
  Valid measurement packets are converted from their binary format into JSON format.
- **Firebase Upload:**  
  The JSON data is then uploaded to the Firebase Realtime Database using an HTTP POST request.
- **Cycle Reset:**  
  After a period with no new packets (a data collection timeout), the gateway concludes the current cycle, waits for a defined delay, and then resets its state to start a new broadcast wave.

## How to Use This Firmware

### Prerequisites
- **Hardware Requirements:**
  - A LoRa-compatible microcontroller (e.g., ESP32) with a LoRa module.
  - Proper wiring according to the pin definitions.
- **Software Requirements:**
  - [PlatformIO](https://platformio.org/) installed (ideally within Visual Studio Code).
  - A working WiFi network.
  - A configured Firebase Realtime Database (update your Firebase credentials and URL in the code).

### Configuration Steps
1. **WiFi Setup:**  
   In the code, update the `WIFI_SSID` and `WIFI_PASSWORD` macros with your network credentials.

2. **Firebase Settings:**  
   Update the Firebase configuration variables (`projectId`, `apiKey`, etc.) to match your Firebase Realtime Database setup. Also, verify that the Realtime Database URL used in the `sendToFirebaseRTDB()` function is correct.

3. **LoRa Setup:**  
   Confirm that the LoRa pin definitions (`LORA_SS`, `LORA_RST`, `LORA_DIO0`) and the `LORA_FREQUENCY` match your hardware configuration.

### Building and Uploading
1. **Open the Project in VSCode:**  
   - Launch Visual Studio Code and open this gateway branch.
   - Make sure the PlatformIO extension is installed and active.

2. **Build the Firmware:**  
   - Click the PlatformIO "Build" button in the bottom toolbar or use the command palette (`Ctrl+Shift+P`) and run `PlatformIO: Build`.

3. **Upload the Firmware:**  
   - Connect your device via USB.
   - Click the PlatformIO "Upload" button or run `PlatformIO: Upload` from the command palette.

4. **Monitor the Output:**  
   - Open the PlatformIO Serial Monitor to view log messages.
   - The serial output will display messages such as "Broadcast sent," "Node responded. Fetching transmitted data...," and "Data collection complete." These messages help you verify that the firmware is functioning correctly.

## Next Steps
- **Explore the Code:**  
  Review the source files in the `src/` directory to understand how the LoRa communication, data collection, and Firebase upload processes are implemented.
- **Check Complementary Branches:**  
  For the Node device firmware and more detailed instructions for that part of the system, please refer to the [node branch](https://github.com/your-username/your-repository/tree/node).
- **Customize and Extend:**  
  Modify configuration parameters (e.g., broadcast intervals, timeout durations) or add new features as needed to suit your application.

## Repository Structure
- **src/**: Contains the firmware source code.
- **platformio.ini**: Contains the PlatformIO project configuration.
- **README.md**: This document explaining the project and how to use it.

## Contributing
Contributions are welcome! If you have improvements or fixes, please fork the repository, make your changes, and submit a pull request.

## License
This project is licensed under the [MIT License](LICENSE).

---

This README provides a clear explanation of what the gateway firmware does and guides users through configuring, building, and deploying the code without mentioning simulated components.
