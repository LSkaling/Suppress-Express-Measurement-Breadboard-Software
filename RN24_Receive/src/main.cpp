#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>

#define CE_PIN 2
#define CSN_PIN 21

const uint16_t thisNode = 00; // Master node address

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

struct DataPacket
{
  int sensorValue;
};

void setup()
{
  Serial.begin(115200);
  if (!radio.begin())
  {
    Serial.println("NRF24L01 not detected!");
    while (1)
      ;
  }

  network.begin(90, thisNode); // Channel 90, Master node address
  Serial.println("Master node initialized.");
}

void loop()
{
  network.update();

  // Check for incoming messages
 while (network.available())
  {
    RF24NetworkHeader header;
    DataPacket receivedData;
    network.read(header, &receivedData, sizeof(receivedData));
    uint16_t nodeNumber = header.from_node;
    int data = receivedData.sensorValue;
    int zero = 0;
    Serial.write(zero);
    Serial.write(nodeNumber);
    //Serial.print(": ");
    Serial.write(data);
    //Serial.print(" ");
    // if(nodeNumber == 3){
    //   Serial.println(" ");
    // }
  }
}
