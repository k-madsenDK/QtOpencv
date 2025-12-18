# Arduino Pico digitalRead Timing Example

This directory contains an Arduino sketch for the Raspberry Pi Pico (RP2040) that demonstrates timing measurements for `digitalRead()` operations using the Earl Philhower Arduino core.

## Hardware Requirements

- **Raspberry Pi Pico** (RP2040-based board)
- **Button** (optional): Connect to GPIO 2 with a pull-up resistor, or use internal pull-up
- **USB Cable**: For programming and serial communication

## Software Requirements

### Earl Philhower Arduino-Pico Core

This sketch requires the Earl Philhower Arduino core for the Raspberry Pi Pico:

**GitHub Repository**: https://github.com/earlephilhower/arduino-pico

### Installation Instructions

1. Open Arduino IDE (version 1.8.13 or later, or Arduino IDE 2.x)
2. Go to **File → Preferences**
3. In "Additional Boards Manager URLs", add:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "pico" and install **"Raspberry Pi Pico/RP2040"** by Earl Philhower
6. Select your board: **Tools → Board → Raspberry Pi Pico/RP2040 → Raspberry Pi Pico**

## Sketch Overview

The `pico_digitalRead_timing.ino` sketch performs three types of timing tests:

### Test 1: Single digitalRead Timing
Measures the time taken for a single `digitalRead()` operation in microseconds.

### Test 2: Average Timing Over Multiple Iterations
Performs 1000 consecutive `digitalRead()` operations and calculates:
- Total time taken
- Average time per read
- Estimated reads per second

### Test 3: digitalRead with State Change Detection
Monitors the input pin for 10 seconds, detecting state changes and measuring timing:
- Counts state changes (useful for button press detection)
- Calculates average read time
- Blinks the built-in LED on state changes

## Pin Configuration

- **GPIO 2**: Digital input pin (with internal pull-up enabled)
- **GPIO 25**: Built-in LED (used for visual feedback)

You can modify these pin assignments in the sketch if needed.

## Usage

1. **Upload the sketch** to your Raspberry Pi Pico
2. **Open Serial Monitor** at 115200 baud rate
3. **Observe timing results** printed to the serial console
4. **Optional**: Press a button connected to GPIO 2 during Test 3 to see state change detection

## Expected Output

```
Arduino Pico - digitalRead Timing Test
======================================
Core: Earl Philhower Arduino-Pico

Setup complete. Starting timing tests...

Test 1: Single digitalRead timing
  digitalRead value: 1
  Time taken: 0 microseconds

Test 2: Average timing over 1000 iterations
  Last read value: 1
  Total time: 2145 microseconds
  Average time per read: 2.145 microseconds
  Reads per second: ~466200

Test 3: digitalRead with change detection (10 second window)
  Tip: Press button connected to GPIO 2 to see state changes
  State change detected at 1234 ms: 1 -> 0
  State change detected at 1456 ms: 0 -> 1
  Test complete. Changes detected: 2
  Total reads: 99980
  Average read time: 2.156 microseconds
```

## Performance Notes

The Earl Philhower Arduino core for RP2040 typically achieves:
- **~0-3 microseconds** per `digitalRead()` operation
- **~300,000 - 500,000 reads per second** depending on clock speed and optimization settings

These timings can vary based on:
- CPU clock speed (default is 125 MHz)
- Compiler optimization settings
- Other running interrupts or background tasks

## Troubleshooting

### Serial Port Not Appearing
- Make sure you're holding the BOOTSEL button while plugging in the Pico for the first upload
- After the first upload, the sketch will automatically run on power-up

### No Output in Serial Monitor
- Check that the baud rate is set to 115200
- The sketch waits up to 5 seconds for the serial connection

### Unexpected Timing Values
- Very high values might indicate timing overflow (unlikely but possible)
- Very low values (0 microseconds) might occur due to `micros()` resolution

## Modifying the Sketch

### Change Number of Test Iterations
Modify the constant at the top of the sketch:
```cpp
const int TEST_ITERATIONS = 1000;  // Change to desired value
```

### Change Input/Output Pins
Modify the pin definitions:
```cpp
const int INPUT_PIN = 2;    // Change to your desired GPIO
const int LED_PIN = 25;     // Change to your desired GPIO
```

### Adjust Test Duration
In `testDigitalReadWithChange()`, modify the duration:
```cpp
while (millis() - testStart < 10000) {  // Change 10000 to desired milliseconds
```

## Integration with QtOpencv Project

This Arduino sketch is provided as a utility for developers who want to:
- Interface hardware sensors with video processing systems
- Understand timing characteristics of GPIO operations on RP2040
- Prototype embedded vision systems with external triggers

While the main QtOpencv project focuses on video annotation and detection, this sketch demonstrates embedded integration possibilities.

## License

MIT License - See LICENSE file in the repository root

## Additional Resources

- [Earl Philhower Arduino-Pico Documentation](https://arduino-pico.readthedocs.io/)
- [RP2040 Datasheet](https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf)
- [Raspberry Pi Pico Documentation](https://www.raspberrypi.org/documentation/pico/getting-started/)

## Contributing

Contributions and improvements to this example are welcome! Please submit pull requests or open issues on the project repository.
