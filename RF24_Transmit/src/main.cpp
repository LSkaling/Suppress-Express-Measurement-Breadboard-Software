#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <Wire.h>
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

// Define pins for the Teensy 4.0 (adjust for your hardware)
#define CE_PIN PB1
#define CSN_PIN PB0

#define NODE_ID_0 PB5
#define NODE_ID_1 PB4
#define NODE_ID_2 PB3
#define NODE_ID_3 PA12
#define NODE_ID_4 PA11
#define NODE_ID_5 PA8
#define NODE_ID_6 PB15
#define NODE_ID_7 PB14
#define NODE_ID_8 PB13
#define LEAF_NODE PB12

#define LED PB10

NAU7802 myScale; // Create instance of the NAU7802 class

// Assign this node a unique address
uint16_t masterNode;
bool branchNode = false;

int readingOffset = 0; // Offset to calibrate the scale

const int time_between_readings = 500; // Time between readings in milliseconds

int node_id = 0;

int lastReadingMillis = 0;

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

struct DataPacket
{
  uint16_t sensorId; // Example data structure
  int sensorValue; // Example data structure
};

void setup()
{
  delay(5000);

  pinMode(LED, OUTPUT);

  pinMode(NODE_ID_0, INPUT);
  pinMode(NODE_ID_1, INPUT);
  pinMode(NODE_ID_2, INPUT);
  pinMode(NODE_ID_3, INPUT);
  pinMode(NODE_ID_4, INPUT);
  pinMode(NODE_ID_5, INPUT);
  pinMode(NODE_ID_6, INPUT);
  pinMode(NODE_ID_7, INPUT);
  pinMode(NODE_ID_8, INPUT);
  pinMode(LEAF_NODE, INPUT);

  node_id |= digitalRead(NODE_ID_0) << 8;
  node_id |= digitalRead(NODE_ID_1) << 7;
  node_id |= digitalRead(NODE_ID_2) << 6;
  node_id |= digitalRead(NODE_ID_3) << 5;
  node_id |= digitalRead(NODE_ID_4) << 4;
  node_id |= digitalRead(NODE_ID_5) << 3;
  node_id |= digitalRead(NODE_ID_6) << 2;
  node_id |= digitalRead(NODE_ID_7) << 1;
  node_id |= digitalRead(NODE_ID_8) << 0;

  if (node_id % 10 == 0) { // Branch node
    masterNode = 00;
    branchNode = true;
  } else { //leaf node
    masterNode = node_id / 10;
  }

  int scaled_node_id = (node_id / 10);

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

  network.begin(90, scaled_node_id); // Channel 90, Sub-node address
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

  Serial1.print("Node ID: ");
  Serial1.println(node_id);

  Serial1.print("Master Node: ");
  Serial1.println(masterNode);

  Serial1.print("Scaled Node ID: ");
  Serial1.println(scaled_node_id);

  myScale.powerUp(); // Power up scale. This scale takes ~600ms to boot and take reading.
}

void loop()
{
  if (millis() - lastReadingMillis > time_between_readings)
  {
    while (myScale.available() == false)
      digitalWrite(LED, HIGH);
    digitalWrite(LED, LOW);

    int32_t currentReading = myScale.getReading();
    Serial1.println(currentReading);



    uint32_t transmit_msg = abs(currentReading) / 100;

    // Send a message to the master node
    DataPacket dataToSend = {node_id, transmit_msg};
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

    lastReadingMillis = millis();
  }

  // Update the network to handle incoming/outgoing messages
  network.update();

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

}