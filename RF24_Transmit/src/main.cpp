#include <SPI.h>
#include <RF24.h>
#include "Arduino.h"
#include "Adafruit_HX711.h"


// Define pins for Teensy 4.0
#define CE_PIN 16
#define CSN_PIN 17

// Define the pins for the HX711 communication
const uint8_t DATA_PIN = 3;  // Can use any pins!
const uint8_t CLOCK_PIN = 2; // Can use any pins!

RF24 radio(CE_PIN, CSN_PIN);

Adafruit_HX711 hx711(DATA_PIN, CLOCK_PIN);

// Address for the transmitter
const byte receiverAddress[6] = "1Node";

struct DataPacket {
  int sensorValue;
};

DataPacket dataToSend;

void setup() {
  Serial.begin(115200);

  if (!radio.begin()) {
    Serial.println("NRF24L01 not detected!");
    while (1);
  }

  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(115);
  radio.openWritingPipe(receiverAddress);
  radio.stopListening();
  Serial.println("Transmitter ready on Teensy 4.0");

    // Initialize the HX711
  hx711.begin();

  // read and toss 3 values each
  Serial.println("Tareing....");
  for (uint8_t t=0; t<3; t++) {
    hx711.tareA(hx711.readChannelRaw(CHAN_A_GAIN_128));
    hx711.tareA(hx711.readChannelRaw(CHAN_A_GAIN_128));
    hx711.tareB(hx711.readChannelRaw(CHAN_B_GAIN_32));
    hx711.tareB(hx711.readChannelRaw(CHAN_B_GAIN_32));
  }

}

void loop() {

  int32_t weightA128 = hx711.readChannelBlocking(CHAN_A_GAIN_128);

  //average out of 100 readings
  for (int i = 0; i < 10; i++) {
    weightA128 += hx711.readChannelBlocking(CHAN_A_GAIN_128);
  }
  weightA128 /= 100;

  //Serial.print("Channel A (Gain 128): ");
  Serial.println(weightA128);

  // Example data to send
  dataToSend.sensorValue = weightA128;

  if (radio.write(&dataToSend, sizeof(dataToSend))) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Data transmission failed");
  }



  delay(10);
}