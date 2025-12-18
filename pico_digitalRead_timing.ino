/*
 * Arduino Pico (Earl Philhower Core) - digitalRead Timing Example
 * 
 * This sketch demonstrates timing measurements for digitalRead operations
 * on the Raspberry Pi Pico using the Earl Philhower Arduino core.
 * 
 * Hardware:
 * - Raspberry Pi Pico (RP2040)
 * - Connect a button to GPIO 2 with pull-up resistor, or use internal pull-up
 * - LED on GPIO 25 (built-in LED on most Pico boards)
 * 
 * Core: https://github.com/earlephilhower/arduino-pico
 * 
 * Author: QtOpencv Project
 * License: MIT
 */

// Pin definitions
const int INPUT_PIN = 2;    // GPIO pin for digital input
const int LED_PIN = 25;     // Built-in LED on most Pico boards
const int TEST_ITERATIONS = 1000;  // Number of iterations for timing test

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {
    ; // Wait for serial port to connect, timeout after 5 seconds
  }
  
  Serial.println("Arduino Pico - digitalRead Timing Test");
  Serial.println("======================================");
  Serial.println("Core: Earl Philhower Arduino-Pico");
  Serial.println();
  
  // Configure pins
  pinMode(INPUT_PIN, INPUT_PULLUP);  // Set input pin with internal pull-up
  pinMode(LED_PIN, OUTPUT);           // Set LED pin as output
  
  // Blink LED to indicate startup
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  Serial.println("Setup complete. Starting timing tests...\n");
}

void loop() {
  // Test 1: Single digitalRead timing
  testSingleDigitalRead();
  delay(1000);
  
  // Test 2: Multiple digitalRead timing (average)
  testMultipleDigitalReads();
  delay(1000);
  
  // Test 3: digitalRead with state change detection
  testDigitalReadWithChange();
  delay(1000);
  
  // Separator for readability
  Serial.println("\n--- Next test cycle in 5 seconds ---\n");
  delay(5000);
}

/**
 * Test single digitalRead operation timing
 */
void testSingleDigitalRead() {
  Serial.println("Test 1: Single digitalRead timing");
  
  noInterrupts();  // Disable interrupts for precise timing
  unsigned long startTime = micros();
  int value = digitalRead(INPUT_PIN);
  unsigned long endTime = micros();
  interrupts();  // Re-enable interrupts
  unsigned long duration = endTime - startTime;
  
  Serial.print("  digitalRead value: ");
  Serial.println(value);
  Serial.print("  Time taken: ");
  Serial.print(duration);
  Serial.println(" microseconds");
  Serial.println();
}

/**
 * Test multiple digitalRead operations and calculate average timing
 */
void testMultipleDigitalReads() {
  Serial.print("Test 2: Average timing over ");
  Serial.print(TEST_ITERATIONS);
  Serial.println(" iterations");
  
  unsigned long totalTime = 0;
  int lastValue = 0;
  
  noInterrupts();  // Disable interrupts for precise timing
  unsigned long startTime = micros();
  for (int i = 0; i < TEST_ITERATIONS; i++) {
    lastValue = digitalRead(INPUT_PIN);
  }
  unsigned long endTime = micros();
  interrupts();  // Re-enable interrupts
  
  totalTime = endTime - startTime;
  float averageTime = (float)totalTime / TEST_ITERATIONS;
  
  Serial.print("  Last read value: ");
  Serial.println(lastValue);
  Serial.print("  Total time: ");
  Serial.print(totalTime);
  Serial.println(" microseconds");
  Serial.print("  Average time per read: ");
  Serial.print(averageTime, 3);
  Serial.println(" microseconds");
  Serial.print("  Reads per second: ~");
  Serial.println((int)(1000000.0 / averageTime));
  Serial.println();
}

/**
 * Test digitalRead with state change detection and timing
 */
void testDigitalReadWithChange() {
  Serial.println("Test 3: digitalRead with change detection (10 second window)");
  Serial.println("  Tip: Press button connected to GPIO 2 to see state changes");
  
  int lastState = digitalRead(INPUT_PIN);
  int changeCount = 0;
  unsigned long testStart = millis();
  unsigned long totalReadTime = 0;
  int readCount = 0;
  
  // Monitor for 10 seconds
  while (millis() - testStart < 10000) {
    unsigned long readStart = micros();
    int currentState = digitalRead(INPUT_PIN);
    unsigned long readEnd = micros();
    
    totalReadTime += (readEnd - readStart);
    readCount++;
    
    if (currentState != lastState) {
      changeCount++;
      Serial.print("  State change detected at ");
      Serial.print(millis() - testStart);
      Serial.print(" ms: ");
      Serial.print(lastState);
      Serial.print(" -> ");
      Serial.println(currentState);
      lastState = currentState;
      
      // Blink LED on state change
      digitalWrite(LED_PIN, currentState);
    }
    
    delayMicroseconds(100);  // Small delay between reads
  }
  
  Serial.print("  Test complete. Changes detected: ");
  Serial.println(changeCount);
  Serial.print("  Total reads: ");
  Serial.println(readCount);
  
  if (readCount > 0) {
    float avgTime = (float)totalReadTime / readCount;
    Serial.print("  Average read time: ");
    Serial.print(avgTime, 3);
    Serial.println(" microseconds");
  }
  Serial.println();
  
  digitalWrite(LED_PIN, LOW);  // Turn off LED
}
