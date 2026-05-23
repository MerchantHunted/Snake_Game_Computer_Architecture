# 🐍 Arcade Snake: MZ_APO Hardware Integration

A fully functional, arcade-style Snake game built from scratch in C for the MicroZed-based **MZ_APO board**, featuring an **ESP32-C6** hardware bridge for analog controls and audio feedback. 

This project was developed as the final assignment for the **Computer Architecture** course at the **Czech Technical University in Prague (CTU FEL)**.

## 🌟 Key Features
* **Custom Memory-Mapped Graphics:** Engineered rendering functions to push RGB565 pixel data directly to the LCD register addresses, bypassing standard graphics libraries.
* **Dynamic Hardware Control:** Utilizes the MZ_APO board's rotary encoders to allow real-time adjustments mid-game. Players can physically dial the game speed, apple spawn rate, and win-condition goals on the fly.
* **Peripheral Hardware Bridge (UART):** Integrates an ESP32-C6 microcontroller via `/dev/ttyUSB0` to read raw analog joystick voltages and trigger an active buzzer for cinematic audio feedback.
* **Real-Time LED Feedback:** The 32-bit LED line acts as a left-to-right progress bar toward the winning score, while RGB LEDs strobe for specific game events (eating apples, pausing, and win/loss states).
* **Non-Blocking I/O:** Implements seamless input handling via Linux `poll()` and `termios` to process both SSH keyboard inputs (WASD) and serial UART joystick commands without freezing the game loop.

## 🛠️ Hardware Architecture

### 1. MZ_APO Board (Main Game Engine)
* **Processor:** ARM Cortex-A9 (Xilinx Zynq-7000)
* **OS:** GNU/Linux
* **Peripherals Used:** LCD Display, 3x Push-Button Rotary Encoders, 2x RGB LEDs, 32-LED Line.

### 2. ESP32-C6 DevKitC-1 (Controller Bridge)
The ESP32-C6 handles safe 3.3V analog readings and audio output, communicating with the MZ_APO via 115200 baud serial.
* **Analog Joystick:** * `VRx` -> GPIO 6
  * `VRy` -> GPIO 5
  * `VCC` -> 3.3V (Strict)
* **Active Buzzer:** * `Positive (+)` -> GPIO 12

## 🚀 How to Build and Run

### Software Requirements
* **WSL / Linux Environment**
* **ARM Cross-Compiler:** `arm-linux-gnueabihf-gcc`
* **Arduino IDE:** (For flashing the ESP32-C6)

### Installation
1. Flash the `esp32_controller.ino` code to the ESP32-C6 using the Arduino IDE.
2. Connect the ESP32-C6 to the MZ_APO board via USB.
3. Clone this repository to your local Linux/WSL environment.
4. Ensure your SSH keys are loaded to allow password-less access to the MZ_APO board:
   ```bash
   eval "$(ssh-agent -s)"
   ssh-add ~/.ssh/mzapo-root-key
