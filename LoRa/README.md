# LoRa Sender & Receiver 

This directory is for LoRa communication system. It includes a sender and a receiver that use LoRa to transmit sensor data. Specifically, this code sends moisture sensor values from the sender to the receiver.

## Libraries Required

- `LoRa`
  - Install via Arduino IDE: **Sketch > Include Library > Manage Libraries**, then search for "LoRa"
- `TinyGPSPlus`
  - Install via Arduino IDE: **Sketch > Include Library > Manage Libraries**, then search for "TinyGPSPlus"
- `XPowersLib`
  - Install via Arduino IDE: **Sketch > Include Library > Manage Libraries**, then search for "XPowersLib"

## Project Structure

```
.
└── LoRa/
    ├── LoRaBoards.cpp		# Board-specific configurations (pins, power, etc.)
    ├── LoRaBoards.h		# Header for board configurations
    ├── utilities.h			# Utility functions (helper methods, macros, etc.)
    ├── LoRaReceiver.ino	# Receiver code
    └── LoRaSender.ino		# Sender code
```

## Code Overview

### LoRa Sender

- **File**: `LoRaSender.ino`
- The sender reads moisture sensor values and transmits the data over LoRa at 915 MHz (default frequency).
- The data is structured as an array of 3 integers representing moisture values from three sensors.
- LoRa transmission settings include
  - Spreading factor: 10
  - Bandwidth: 125 kHz
  - Output power: 17 dBm
  - Coding rate: 7
  - Preamble length: 16
  - Sync word: 0xAB
- Moisture sensor readings are mock values for demonstration. Should be replaced with actual sensor data in the `readData()` function.

### LoRa Receiver

- **File**: `LoRaReceiver.ino`
- The receiver listens for LoRa packets, decodes them, and prints the received sensor data to the serial monitor.

## How to Use

1. Connect sensor board and upload `LoRaSender.ino` using Arduino IDE.
2. Power the sender board, either via cable connection or battery.
3. Connect receiver board and upload `LoraReceiver.ino` using Arduino IDE.
4. Open the Arduino Serial Monitor, set baud rate to 115200.
5. You should start seeing the transmitted moisture sensor data in the serial monitor.

## Modifying the Code

- To use actual moisture sensors, modify the `readData()` function in `LoRaSender.ino` to read values from the connected sensors.