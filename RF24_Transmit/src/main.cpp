#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <Wire.h>
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

// Define pins for the Teensy 4.0 (adjust for your hardware)
#define CE_PIN 2
#define CSN_PIN 21

#define loadcell_switch 16

NAU7802 myScale; // Create instance of the NAU7802 class

const bool branchNode = false; // Set to true if this node is a branch node

// Assign this node a unique address
const uint16_t thisNode = 021;   // Example: Sub-node 01
const uint16_t masterNode = 01; // Master node address

int readingOffset = 0; // Offset to calibrate the scale

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

struct DataPacket
{
  int sensorValue; // Example data structure
};

void setup()
{
  Serial.begin(115200);

  if (!radio.begin())
  {
    Serial.println("NRF24L01 not detected!");
    while (1)
      ; // Halt if no module is found
  }

  network.begin(90, thisNode); // Channel 90, Sub-node address
  Serial.println("Sub-node initialized.");

  Wire.begin();

  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }
  Serial.println("Scale detected!");

  myScale.powerUp(); // Power up scale. This scale takes ~600ms to boot and take reading.
}

void loop()
{

  if(millis() % 1000 == 0){
    digitalWrite(loadcell_switch, HIGH);
  }else{
    digitalWrite(loadcell_switch, LOW);
  }
  // myScale.powerDown(); // Power down to ~200nA
  // delay(1000);

  // Time how long it takes for scale to take a reading
  unsigned long startTime = millis();
  while (myScale.available() == false)
    delay(1);

  int32_t currentReading = myScale.getReading();
  // Serial.print("Startup time: ");
  // Serial.print(millis() - startTime);
  // Serial.print(", ");
  Serial.println(currentReading);

  // Update the network to handle incoming/outgoing messages
  network.update();

  // Send a message to the master node
  DataPacket dataToSend = {random(0, 100)}; // Random sensor value for demonstration
  RF24NetworkHeader header(masterNode);     // Header for the master node
  bool success = network.write(header, &dataToSend, sizeof(dataToSend));

  if (success)
  {
    Serial.print("Message sent to master: ");
    Serial.println(dataToSend.sensorValue);
  }
  else
  {
    //Serial.println("Message sending failed.");
  }

  while (branchNode && network.available())
  {
    RF24NetworkHeader header;
    char message[32];
    network.read(header, &message, sizeof(message));

    // Print the received message
    Serial.print("Received message from Node ");
    Serial.print(header.from_node);
    Serial.print(": ");
    Serial.println(message);

    // Forward the message to the master (Node 00)
    RF24NetworkHeader forwardHeader(00); // Destination: Master Node
    network.write(forwardHeader, &message, sizeof(message));

    Serial.println("Message relayed to Master Node (00).");
  }

  delay(10);
}