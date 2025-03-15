# Smart Meter System

This repository contains the code for a Smart Meter System that leverages LoRa communication and Firebase Realtime Database integration. The project is organized into multiple branches, each tailored for a specific device role.

## Project Overview

The system consists of two primary components:

- **Node Device**  
  Responsible for gathering measurement data from sensors and transmitting this data over a LoRa network.

- **Gateway Device**  
  Listens for incoming LoRa transmissions from Node devices, processes the received data, and uploads it to Firebase Realtime Database for centralized storage and remote monitoring.

## Key Features

- **LoRa Communication:**  
  Utilizes LoRa for long-range, low-power wireless communication between devices.

- **Data Processing and Upload:**  
  The Gateway validates measurement packets using a checksum, converts binary data into JSON, and sends it to Firebase Realtime Database.

- **Cloud Integration:**  
  Integration with Firebase Realtime Database enables centralized data collection and remote monitoring.

- **Device Coordination:**  
  The Gateway sends broadcast commands to initiate data transmission from Node devices and collects multiple data packets.

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/install)
- [Visual Studio Code](https://code.visualstudio.com/)
- A working LoRa hardware setup (with proper wiring according to the project configuration).
- A configured Firebase Realtime Database (update your Firebase credentials and project settings in the code).

### Setup Instructions

1. **Clone the Repository**

   Clone this repository to your local machine:
   ```bash
   git clone https://github.com/your-username/your-repository.git
   cd your-repository
