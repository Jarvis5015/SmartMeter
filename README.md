# Node Device Firmware

This branch contains the firmware for the Node device in our Smart Meter System. The Node device is responsible for acquiring measurement data from sensors and transmitting this data over a LoRa network when requested by a Gateway device.

## What Does the Code Do?

### 1. Initialization
- **Hardware Setup:**  
  The firmware configures the LoRa module using predefined pin assignments and frequency settings.
- **Measurement Data Handling:**  
  The Node device maintains a circular buffer to store measurement records. These records include parameters like voltage, current, power, and RMS values.
- **LoRa Communication:**  
  The device continuously monitors for incoming broadcast commands from the Gateway. When a broadcast is received, the Node device transmits its stored measurement data as a binary packet over LoRa.
- **Data Integrity:**  
  Each data packet is constructed with a header and checksum to ensure that the information is transmitted correctly.

### 2. Data Transmission
- **Triggering Transmission:**  
  Upon detecting a broadcast command from the Gateway, the Node device prepares a data packet.  
- **Packet Composition:**  
  The firmware builds a structured packet that includes:
  - Device identifiers (meterID, discomID, customerID)
  - A set of measurement records (up to a maximum defined number)
  - A message field indicating the type of transmission (e.g., "RESPONSE")
  - A checksum for packet integrity.
- **Sending the Data:**  
  Once the packet is assembled and validated, it is sent over the LoRa network to the Gateway.

## How to Use This Firmware

### Prerequisites
- **Hardware Requirements:**
  - A LoRa-compatible microcontroller (e.g., ESP32) equipped with a LoRa module.
  - Sensors connected to the device (ensure proper wiring according to your design).
- **Software Requirements:**
  - [PlatformIO](https://platformio.org/) installed (ideally integrated with Visual Studio Code).
  - A properly configured LoRa network.
  
### Configuration Steps

1. **Update Sensor and Hardware Settings:**  
   - Modify the sensor reading routines (if needed) to match your actual hardware.
   - Confirm that the LoRa pin definitions (`LORA_SS`, `LORA_RST`, `LORA_DIO0`) and `LORA_FREQUENCY` are set according to your wiring and regional regulations.

2. **Configure Device Identifiers:**  
   - Set the `METER_ID`, `DISCOM_ID`, and `CUSTOMER_ID` constants as required by your application.

3. **Prepare for Transmission:**  
   - The code listens for a broadcast command from the Gateway. Ensure that your Gateway is set up and broadcasting appropriately.

### Building and Uploading

1. **Open the Project:**  
   - Launch Visual Studio Code and open the Node branch of the repository.
   - Make sure the PlatformIO extension is installed and active.

2. **Build the Firmware:**  
   - Use the PlatformIO sidebar or command palette (`Ctrl+Shift+P` → `PlatformIO: Build`) to compile the firmware.

3. **Upload the Firmware:**  
   - Connect your device via USB.
   - Use the PlatformIO "Upload" command to flash the firmware onto your Node device.

4. **Monitor the Output:**  
   - Open the PlatformIO Serial Monitor to view log messages. You should see messages indicating when the device records measurement data and when it transmits data in response to a broadcast.

## Next Steps

- **Explore the Code:**  
  Review the source code in the `src/` folder to understand how measurement data is acquired, stored, and transmitted.
- **Test with Gateway:**  
  Ensure that the Node device and Gateway are properly paired on your LoRa network. Verify that the Node responds correctly to broadcast commands.
- **Customize and Enhance:**  
  Adapt the sensor reading and data processing routines as needed to suit your specific application requirements.

## Repository Structure

- **src/**: Contains the source code for the Node firmware.
- **platformio.ini**: PlatformIO configuration file.
- **README.md**: This document provides an overview of the Node device firmware and instructions for use.

## Contributing

Contributions are welcome! If you have suggestions, improvements, or bug fixes, please fork the repository, commit your changes, and submit a pull request. Testing changes using PlatformIO is recommended before merging.

## License

This project is licensed under the [MIT License](LICENSE).

---

This README gives an overview of the Node firmware, explains how it collects and transmits measurement data over LoRa, and provides clear instructions on configuring, building, and deploying the code. For more detailed information on the overall system, refer to the README on the main branch.
