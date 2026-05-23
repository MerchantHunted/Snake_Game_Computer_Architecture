# 🐍 Arcade Snake: MZ_APO Hardware Integration

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Hardware](https://img.shields.io/badge/Hardware-MicroZed%20MZ__APO-orange.svg)
![Controller](https://img.shields.io/badge/Controller-ESP32--C6-green.svg)
![Course](https://img.shields.io/badge/Course-Computer%20Architecture-lightgrey.svg)

A fully functional, arcade-style Snake game built from scratch in **C** for the MicroZed-based **MZ_APO board**, featuring an **ESP32-C6** hardware bridge for analog joystick controls and cinematic audio feedback. 

This project was developed as the final assignment for the **Computer Architecture** course at the **Czech Technical University in Prague (CTU FEL)**.

---

## 🌟 Key Features

* **Custom Memory-Mapped Graphics:** Engineered rendering functions to push RGB565 pixel data directly to the LCD register addresses, bypassing standard high-level graphics libraries. Includes custom algorithms to scale arcade-style fonts.
* **Dynamic Hardware Control:** Utilizes the MZ_APO board's physical rotary encoders to allow real-time adjustments mid-game. Players can physically dial the game speed, apple spawn rate, and win-condition goals on the fly.
* **Peripheral Hardware Bridge (UART):** Integrates an ESP32-C6 microcontroller via `/dev/ttyUSB0` to safely read raw 3.3V analog joystick voltages and trigger an active buzzer for audio feedback without interrupting the main CPU.
* **Real-Time LED Feedback:** The 32-bit LED line acts as a left-to-right progress bar toward the winning score. The RGB LEDs strobe for specific game events, such as eating apples, pausing, and win/loss states.
* **Non-Blocking I/O:** Implements seamless input handling via Linux `poll()` and `termios` to process both SSH keyboard inputs (WASD) and serial UART joystick commands simultaneously without freezing the game loop.
* **AFK Auto-Pause:** The game engine tracks frame timings to detect inactivity, automatically pausing the game if no directional inputs are received for 10 seconds.

---

## 🛠️ Hardware Architecture

### 1. MZ_APO Board (Main Game Engine)
* **Processor:** ARM Cortex-A9 (Xilinx Zynq-7000)
* **OS:** GNU/Linux
* **Peripherals Used:** * LCD Display (Memory-Mapped)
  * 3x Push-Button Rotary Encoders
  * 2x RGB LEDs
  * 32-LED Line

### 2. ESP32-C6 DevKitC-1 (Controller Bridge)
The ESP32-C6 handles safe 3.3V analog readings and audio output, communicating with the MZ_APO via a 115200 baud serial connection.
* **Analog Joystick:** * `VRx` $\rightarrow$ GPIO 6
  * `VRy` $\rightarrow$ GPIO 5
  * `VCC` $\rightarrow$ 3.3V (Strictly 3.3V logic)
  * `GND` $\rightarrow$ GND
* **Active Buzzer:** * `Positive (+)` $\rightarrow$ GPIO 12
  * `Negative (-)` $\rightarrow$ GND

---

## 🚀 How to Build and Run

### Software Requirements
* **WSL / Linux Environment**
* **ARM Cross-Compiler:** `arm-linux-gnueabihf-gcc`
* **Arduino IDE:** (For flashing the ESP32-C6 controller)

### Installation & Execution

1. **Flash the Controller:**
   * Open `Snake_Game_Arduino_Code.ino` in the Arduino IDE.
   * Flash the code to your ESP32-C6.
   * Disconnect the ESP32 from your computer and plug it into the USB port of the MZ_APO board.

2. **Prepare the Build Environment:**
   * Clone this repository to your local Linux/WSL environment.
   * Ensure your SSH keys are loaded to allow password-less access to the MZ_APO board:
     ```bash
     eval "$(ssh-agent -s)"
     ssh-add ~/.ssh/mzapo-root-key
     ```

3. **Configure Target:**
   * Open the `Makefile` and verify the `TARGET_IP` matches your MZ_APO board's current IP address.

4. **Compile and Play:**
   * Execute the following command in your terminal to compile the code, transfer the executable to the board, and launch the game:
     ```bash
     make clean run
     ```

---

## 🎮 Controls

### Movement
* **Analog Joystick** (Primary)
* **W, A, S, D** (Secondary, via SSH terminal)

### Hardware Interface (MZ_APO Rotary Knobs)
* 🔴 **Red Knob:** Adjust Snake Speed (Levels 1-5).
* 🔵 **Blue Knob:** Adjust Apple Spawn Rate (Levels 1-5). 
  * *Press the Blue Knob to trigger a Hard Reset instantly mid-game.*
* 🟢 **Green Knob:** Adjust the Winning Goal (5 to 50 apples).
* **Pause / Unpause:** Press the **Red** or **Green** knob.

---

## 👨‍💻 Contributors

* **Kanan Abdullayev** - Hardware Integration, Memory Mapping, Game Engine Logic.
* **Ozan Cıncık** - Core implementation and testing.

*Developed for the Spring 2026 Semester.*
