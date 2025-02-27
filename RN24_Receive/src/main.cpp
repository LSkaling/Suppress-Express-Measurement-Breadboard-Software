#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>

#define CE_PIN 2
#define CSN_PIN 21

const uint16_t thisNode = 00; // Master node address

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

char serial_buf[9];

struct DataPacket
{
  uint16_t node_id;
  int sensorValue;
};

char mask_shift(int in, int shift)
{
  shift = shift * 8;
  return (in >> shift) & 0xff;
}

// Pack a buffer of exactly 9 bytes for one sensor reading
void pack_serial_packet(char *buf, int id, int data)
{
  buf[0] = 0;
  for (int i = 0; i < 4; i++)
  {
    buf[i + 1] = mask_shift(id, i);
  }
  for (int i = 0; i < 4; i++)
  {
    buf[i + 5] = mask_shift(data, i); // <-- Fix: Now writes to correct location
  }
}

void setup()
{
  for(int i = 0; i < 9; i++) {
    serial_buf[i] = 0;
  }
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
    uint16_t node_number_full = receivedData.node_id;
    int data = receivedData.sensorValue;
    pack_serial_packet(serial_buf, nodeNumber, data);


    Serial.print("Received data from node ");
    Serial.print(node_number_full);
    Serial.print(" (");
    Serial.print(nodeNumber);
    Serial.print("): ");
    Serial.println(data);

  }
}
