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

uint16_t node_id = 0;

int lastReadingMillis = 0;

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

uint16_t readDIPSwitch()
{
  uint16_t address = 0;
  address |= digitalRead(NODE_ID_0) << 8;
  address |= digitalRead(NODE_ID_1) << 7;
  address |= digitalRead(NODE_ID_2) << 6;
  address |= digitalRead(NODE_ID_3) << 5;
  address |= digitalRead(NODE_ID_4) << 4;
  address |= digitalRead(NODE_ID_5) << 3;
  address |= digitalRead(NODE_ID_6) << 2;
  address |= digitalRead(NODE_ID_7) << 1;
  address |= digitalRead(NODE_ID_8) << 0;
  return address;
}

bool isBranchNode(uint16_t node_address)
{
  // If you consider the master node (0) also as a "branch," remove the second check.
  return (node_address % 8 == 0) && (node_address != 0);
}

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

  uint16_t node_address = readDIPSwitch();
  uint16_t parent_address = node_address >> 3; // Assuming a 3-bit branch depth

  node_id = node_address;

  branchNode = isBranchNode(node_address);

  masterNode = node_id >> 3;

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

  Serial1.print("Node Address: ");
  Serial1.println(node_address, OCT);
  Serial1.print("Parent Address: ");
  Serial1.println(parent_address, OCT);

  myScale.powerUp(); // Power up scale. This scale takes ~600ms to boot and take reading.

  network.begin(90, node_id); // Channel 90, Sub-node address
}

void loop()
{
  // if (millis() - lastReadingMillis > time_between_readings)
  // {
  //   while (myScale.available() == false)
  //     digitalWrite(LED, HIGH);
  //   digitalWrite(LED, LOW);

  int32_t currentReading = myScale.getReading();
  //   Serial1.println(currentReading);



  uint32_t transmit_msg = abs(currentReading) / 100;

  //   // Send a message to the master node
  //   DataPacket dataToSend = {node_id, transmit_msg};
  //   RF24NetworkHeader header(masterNode);     // Header for the master node
  //   bool success = network.write(header, &dataToSend, sizeof(dataToSend));

  //   if (success)
  //   {
  //     Serial1.print("Message sent to master: ");
  //     Serial1.println(dataToSend.sensorValue);
  //   }
  //   else
  //   {
  //     //Serial1.println("Message sending failed.");
  //   }

  //   lastReadingMillis = millis();
  // }

  // Update the network to handle incoming/outgoing messages
  network.update();

  static unsigned long lastSend;
  if (millis() - lastSend > 1000 && node_id != 0)
  {
    lastSend = millis();

    // Suppose we read from scale (dummy example):
    int32_t scaleReading = 123;
    // if (myScale.available()) scaleReading = myScale.getReading();

    DataPacket pkt;
    pkt.sensorId = node_id; // who am I
    pkt.sensorValue = transmit_msg;

    // **Auto-routing** to master (address=0)
    RF24NetworkHeader masterHeader(0);
    bool ok = network.write(masterHeader, &pkt, sizeof(pkt));

    if (ok)
    {
      Serial1.print("Auto-routed to master: ");
      Serial1.println(pkt.sensorValue);
    }
    else
    {
      Serial1.println("Send failed!");
    }
  }
}