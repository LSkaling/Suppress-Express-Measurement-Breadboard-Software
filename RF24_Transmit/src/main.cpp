#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <Wire.h>
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

// Define pins for the Teensy 4.0 (adjust for your hardware)
#define CE_PIN PB1
#define CSN_PIN PB0

#define LED PB10

NAU7802 myScale; // Create instance of the NAU7802 class

const bool branchNode = false; // Set to true if this node is a branch node

// Assign this node a unique address
const uint16_t thisNode = 01;   // Example: Sub-node 01
const uint16_t masterNode = 00; // Master node address

int readingOffset = 0; // Offset to calibrate the scale

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

struct DataPacket
{
  int sensorValue; // Example data structure
};

void setup()
{
  delay(5000);

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


  Serial1.begin(9600);

  while(!Serial1){
    digitalWrite(LED, LOW);
  }

  delay(2000);
  Serial1.println("Starting...");

  digitalWrite(LED, HIGH);

  if (!radio.begin())
  {
    while (1){
      Serial1.println("NRF24L01 not detected!");
      delay(1000);
    }
  }

  Serial1.println("NRF24L01 detected!");

  network.begin(90, thisNode); // Channel 90, Sub-node address
  Serial1.println("Sub-node initialized.");

  digitalWrite(LED, LOW);

  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin();

  if (myScale.begin() == false)
  {
    while (1){
      Serial1.println("Scale not detected!");
      delay(1000);
    }
  }
  Serial1.println("Scale detected!");

  myScale.powerUp(); // Power up scale. This scale takes ~600ms to boot and take reading.
}

void loop()
{

  // myScale.powerDown(); // Power down to ~200nA
  // delay(1000);

  // Time how long it takes for scale to take a reading
  unsigned long startTime = millis();
  while (myScale.available() == false)
    delay(1);

  int32_t currentReading = myScale.getReading();
  // Serial1.print("Startup time: ");
  // Serial1.print(millis() - startTime);
  // Serial1.print(", ");
  Serial1.println(currentReading);

  // Update the network to handle incoming/outgoing messages
  network.update();

  uint32_t transmit_msg = abs(currentReading) / 100;

  // Send a message to the master node
  DataPacket dataToSend = {transmit_msg};   // Random sensor value for demonstration
  RF24NetworkHeader header(masterNode);     // Header for the master node
  bool success = network.write(header, &dataToSend, sizeof(dataToSend));

  if (success)
  {
    Serial1.print("Message sent to master: ");
    Serial1.println(dataToSend.sensorValue);
  }
  else
  {
    //Serial1.println("Message sending failed.");
  }
  */

  while (branchNode && network.available())
  {
    RF24NetworkHeader header;
    char message[32];
    network.read(header, &message, sizeof(message));

    // Print the received message
    Serial1.print("Received message from Node ");
    Serial1.print(header.from_node);
    Serial1.print(": ");
    Serial1.println(message);

    // Forward the message to the master (Node 00)
    RF24NetworkHeader forwardHeader(00); // Destination: Master Node
    network.write(forwardHeader, &message, sizeof(message));

    Serial1.println("Message relayed to Master Node (00).");
  }
  delay(1000 / 80); // ADC runs at 80 samples per second
}