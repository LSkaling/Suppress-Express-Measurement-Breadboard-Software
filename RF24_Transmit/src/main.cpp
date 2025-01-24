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

// Fast sample-averaging
#define POWER 4
#define N_AVG (1 << POWER)
int buffer[N_AVG];
int read_index = 0;
int writeMask = N_AVG - 1;
int sum = 0;

int readAvg(int in) {
  sum -= buffer[read_index];
  buffer[read_index] = in;
  sum += in;
  read_index++;
  read_index &= writeMask;
  return (sum >> POWER);
}

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
}

void loop() {
  int32_t weightA128 = hx711.readChannelBlocking(CHAN_A_GAIN_128);

  int out = readAvg(weightA128);

  //Serial.print("Channel A (Gain 128): ");
  Serial.println(out);

  // For Ben's purposes, don't bother sending on radio
  /*
  // Example data to send
  dataToSend.sensorValue = weightA128;

  if (radio.write(&dataToSend, sizeof(dataToSend))) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Data transmission failed");
  }
  */

  delay(1000 / 80); // ADC runs at 80 samples per second
}