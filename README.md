# ESP32 Guitar Tuner
Joshua Weisner and Brad Willard Spring 2025
Automatic Guitar Tuner


This project implements a guitar tuner using an ESP32 microcontroller with an INMP441 I2S microphone and automatic string tuning capability using an N30 motor. The tuner can detect the frequency of a plucked guitar string, provide tuning guidance, and automatically adjust the tuning.

## Features

- Detects standard guitar string frequencies (E2, A2, D3, G3, B3, E4)
- Provides accurate frequency measurement using the Goertzel algorithm
- Shows tuning information in cents (±)
- Guides tuning with visual indicators (too high/low/in tune)
- Implements noise reduction and signal processing techniques
- Automatic string tuning with N30 5V 42 RPM motor
- Portable with 18650 battery power

## Hardware Requirements

- ESP32 development board (ESP32-WROOM, ESP32-DevKitC, etc.)
- INMP441 I2S MEMS microphone
- TB6612FNG motor driver
- N30 5V 42 RPM DC motor
- 18650 battery
- Boost converter and charge board for battery management
- Breadboard and jumper wires
- USB cable for programming and power

## Wiring Connections

### INMP441 I2S Microphone
Connect the INMP441 microphone to the ESP32 as follows:

| Microphone Pin | ESP32 Pin | Description       |
|----------------|-----------|-------------------|
| SD / DATA      | GPIO 40   | Serial Data       |
| WS / LR / L/R  | GPIO 38   | Word Select       |
| SCK / BCLK     | GPIO 36   | Serial Clock      |
| GND            | GND       | Ground            |
| VDD            | 5V        | Power             |

### TB6612FNG Motor Driver
Connect the TB6612FNG motor driver to the ESP32:

| TB6612FNG Pin | Connection             | Description       |
|---------------|------------------------|-------------------|
| VM            | 5V from Boost Converter| Motor Power       |
| VCC           | 5V from ESP32          | Logic Power       |
| GND           | GND                    | Ground            |
| PWMA          | GPIO 25                | Motor A PWM       |
| AIN1          | GPIO 26                | Motor A Direction |
| AIN2          | GPIO 27                | Motor A Direction |
| STBY          | 3.3V                   | Standby (Active High) |

### N30 DC Motor
Connect the N30 DC motor to the TB6612FNG:

| Motor Wire | TB6612FNG Pin | Description |
|------------|---------------|-------------|
| Red        | A01           | Motor + Terminal |
| Black      | A02           | Motor - Terminal |

### Battery & Power Management
Connect the 18650 battery, boost converter and charge board:

| Component | Connection |
|-----------|------------|
| 18650 Battery | B+ and B- on charge board |
| Charge Board 5V Out | Boost Converter Input |
| Boost Converter 5V Out | VM on TB6612FNG |
| GND connections | Common ground for all components |

Note: You can adjust the GPIO pin assignments in the code if needed.

## Software Requirements

- Arduino IDE (1.8.x or later)
- ESP32 Arduino Core (install via Boards Manager)

## Installation

1. Install the Arduino IDE from [arduino.cc](https://www.arduino.cc/)
2. Add ESP32 support to Arduino IDE:
   - Open Arduino IDE
   - Go to File > Preferences
   - Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to "Additional Boards Manager URLs"
   - Go to Tools > Board > Boards Manager
   - Search for "esp32" and install "ESP32 by Espressif Systems"

3. Download the Goertzl.ino file to your computer
4. Open the file in Arduino IDE
5. Select your ESP32 board from Tools > Board menu
6. Connect your ESP32 board via USB
7. Select the correct port from Tools > Port
8. **Important:** Before uploading, go to Tools > USB CDC On Boot and select "Enabled"
9. Click Upload button (right arrow) to program the ESP32

Note: Enabling USB CDC On Boot allows the serial monitor to work properly when the ESP32 restarts.

## Usage

1. After uploading the code, open the Serial Monitor (Tools > Serial Monitor)
2. Set the Serial Monitor baud rate to 115200
3. You should see "Guitar Tuner Ready" and "Play a string to begin tuning..."
4. Ensure the 18650 battery is charged and connected to the power management circuit
5. Pluck a guitar string near the microphone
6. The tuner will display:
   - Detected frequency in Hz
   - Identified guitar string (E2, A2, D3, G3, B3, or E4)
   - Tuning error in cents
   - Tuning guidance (too low, too high, or in tune)
7. If using automatic tuning, the motor will adjust the string tension as needed:
   - When too low (negative cents), the motor will tighten the string
   - When too high (positive cents), the motor will loosen the string
   - When in tune (±5 cents), the motor will stop

## Troubleshooting

- **No response when plucking strings**: Make sure the microphone is working and positioned close to the guitar
- **Inconsistent readings**: Try in a quieter environment or adjust the RMS threshold in code
- **Wrong string detection**: Make sure your guitar is not too far off from standard tuning initially
- **Motor not running**: Check battery voltage and motor connections
- **ESP32 not responding**: Make sure USB CDC On Boot is enabled in Tools menu
- **Battery drains quickly**: Reduce motor timeout settings or use a higher capacity 18650 cell

## Customization

You can customize the code to:
- Adjust sampling parameters for better performance
- Change tuning frequency standards for alternate tunings
- Modify detection sensitivity by changing thresholds
- Adjust motor control parameters:
  - `MOTOR_SPEED_MIN` and `MOTOR_SPEED_MAX` for speed control
  - `TUNE_THRESHOLD` for automatic tuning trigger point
  - `MOTOR_TIMEOUT` for safety timeout duration
- Add a display (OLED, LCD) for visual feedback
- Implement button controls for mode selection

## Motor Integration Instructions

1. Add the motor control code (available in the repository) to your Goertzl.ino file
2. Make pin assignments match your wiring configuration if different from default
3. Adjust motor parameters based on your specific tuning pegs and mechanical setup
4. Uncomment the `adjustTuning()` function call in the main loop where indicated
5. Test automatic tuning with gentle string adjustments first

## Power Management Tips

- A fully charged 18650 battery typically provides about 3.7-4.2V
- The boost converter raises this to a stable 5V for the motor
- Average runtime depends on how much auto-tuning is done
- To extend battery life:
  - Increase `TUNE_THRESHOLD` to reduce motor activations
  - Decrease `MOTOR_TIMEOUT` value
  - Add a physical power switch when not in use
