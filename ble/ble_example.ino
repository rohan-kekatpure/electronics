#include <ArduinoBLE.h>

# define SERVICE_ID "19B10000-E8F2-537E-4F6C-D104768A1214"
# define CHAR_ID "19B10001-E8F2-537E-4F6C-D104768A1214"

BLEService ledService(SERVICE_ID);
BLEByteCharacteristic ledCharacteristic(CHAR_ID, BLERead | BLEWrite);
BLEDescriptor ledDescriptor("2901", "LED state");

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }


  BLE.setLocalName("First BLE program");
  BLE.setAdvertisedService(ledService);
  ledCharacteristic.addDescriptor(ledDescriptor);
  ledService.addCharacteristic(ledCharacteristic);

  BLE.addService(ledService);

  // Set the initial value of the characteristic
  ledCharacteristic.writeValue(0);

  // // Set a handler for when a BLE central connects
  // BLE.setEventHandler(BLEConnected, bleConnectHandler);

  // // Set a handler for when a BLE central disconnects
  // BLE.setEventHandler(BLEDisconnected, bleDisconnectHandler);

  // // Set a handler for when the characteristic is written to
  // ledCharacteristic.setEventHandler(BLEWritten, ledCharacteristicWritten);

  // Start advertising the service
  BLE.advertise();
  Serial.println("BLE advertising started.");
}

void loop() { 
  BLE.poll();
  int v;
  if (ledCharacteristic.written()) {
    v = ledCharacteristic.value();
    Serial.print("Received: ");
    Serial.println(v);
    if (v > 0){
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

// void bleConnectHandler(BLEDevice central) {
//   Serial.print("Connected to central: ");
//   Serial.println(central.address());

//   // Blink the LED to indicate a successful connection
//   for (int i = 0; i < 5; i++) {
//     digitalWrite(ledPin, HIGH);
//     delay(100);
//     digitalWrite(ledPin, LOW);
//     delay(100);
//   }
// }

// void bleDisconnectHandler(BLEDevice central) {
//   Serial.print("Disconnected from central: ");
//   Serial.println(central.address());
// }

// void ledCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
//   // Read the new value from the characteristic
//   unsigned char value;
//   characteristic.readValue(&value, 1);

//   if (value == 1) {
//     digitalWrite(ledPin, HIGH);
//     Serial.println("LED ON");
//   } else if (value == 0) {
//     digitalWrite(ledPin, LOW);
//     Serial.println("LED OFF");
//   }
// }