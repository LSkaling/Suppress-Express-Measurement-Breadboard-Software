#include <SPI.h>
#include <RF24.h>

// Define pins for Teensy 4.0
#define CE_PIN 19
#define CSN_PIN 18

RF24 radio(CE_PIN, CSN_PIN);

// Address for the receiver
const byte receiverAddress[6] = "1Node";

struct DataPacket {
  int sensorValue;
};

DataPacket receivedData;

void setup() {
  Serial.begin(115200);

  if (!radio.begin()) {
    Serial.println("NRF24L01 not detected!");
    while (1);
  }

  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(115);
  radio.openReadingPipe(1, receiverAddress);
  radio.startListening();
  Serial.println("Receiver ready on Teensy 4.0");
}

void loop() {
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));
    Serial.println(receivedData.sensorValue);
  }
}
