// Arduino I2C Scanner for OLED Display
// This sketch scans the I2C bus for connected devices and prints their addresses.
// It is useful for finding the address of your 0.96" OLED display.

#include <Wire.h>

// I2C Scanner sketch based on work by Nick Gammon.
// This is a standard and reliable way to find I2C device addresses.

void setup() {
  // Start the I2C bus (Wire library)
  Wire.begin();
  
  // Start the serial communication at 115200 baud
  // The Serial Monitor in the Arduino IDE must be set to the same baud rate.
  Serial.begin(115200);
  
  // Wait for the Serial Monitor to be ready
  while (!Serial);
  
  Serial.println("\nI2C Scanner");
}

void loop() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("Scanning...");
  
  // Iterate through all possible I2C addresses (from 1 to 127)
  for (address = 1; address < 127; address++) {
    // Begin a transmission to the current address
    Wire.beginTransmission(address);
    
    // End the transmission and check the return value
    // 0 = success, other values indicate an error
    error = Wire.endTransmission();
    
    if (error == 0) {
      // If the transmission was successful, a device was found
      Serial.print("I2C device found at address 0x");
      
      // Print the address in hexadecimal format
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");
      
      deviceCount++;
    } else if (error == 4) {
      // Other error (e.g., a connection issue)
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  
  // Print the final scan results
  if (deviceCount == 0) {
    Serial.println("No I2C devices found.");
  } else {
    Serial.print(deviceCount);
    Serial.println(" device(s) found.");
  }
  
  // Wait 5 seconds before scanning again
  Serial.println("Done scanning. Retrying in 5 seconds.");
  delay(5000);
}
