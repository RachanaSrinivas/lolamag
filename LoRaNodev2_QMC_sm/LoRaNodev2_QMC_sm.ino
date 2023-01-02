#include "config.h"
#include "MessageQueue.h"
#include "Conversion.h"
#include <Ticker.h>

#if SENSOR
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  5

int SensorMsgId;
QMC5883LCompass compass;
#endif

volatile bool ReadReceivedData;
MessageQueue Mq;
Conversion Conv;
Ticker ClearMsgIdQueue;
Ticker HeartBeatSignal;


void loRaSetLedStatus(bool state) {
  if (state) {
    digitalWrite(LORA_STATUS_FAIL, LOW);
    digitalWrite(LORA_STATUS_OK, HIGH);
  }
  else {
    digitalWrite(LORA_STATUS_OK, LOW);
    digitalWrite(LORA_STATUS_FAIL, HIGH);
  }
}

#if SENSOR
String getSensorsReadings(String type) {
  float x, y, z;
  compass.read();
  x = compass.getX() / 10;
  y = compass.getY() / 10;
  z = compass.getZ() / 10;

  char magString[50];

  sprintf(magString, ":%0.4f,%0.4f,%0.4f", x, y, z);

  return String(type) + String(magString);
}
#endif

#if SENSOR
String getSensorData() {
  SensorMsgId = SensorMsgId + 1;
  return String(NODE_ID) + ";" + String(0) + ";" + String(SensorMsgId) + ";" + String(getSensorsReadings("m"));
}
#endif

#if SWITCH
Message parseLoRaMessage(String msg) {
  return Conv.toObject(msg);
}

String getAckMessage(Message rxData) {
  return String(rxData.NodeTo) + ";" + String(rxData.NodeFrom) + ";" + String(rxData.Id) + ";a:ACK";
}

String getErrorMessage(Message rxData) {
  return String(rxData.NodeTo) + ";" + String(rxData.NodeFrom) + ";" + String(rxData.Id) + ";e:INVALID COMMAND";
}

//TODO: add battery percentage in alive status
// 200 powered directly
// <100 battery powered
String getHeartBeatMessage() {
  return String(NODE_ID) + ";" + String(0) + ";-100;h:99";
}

bool executeCommand(String content) {
  if (strcmp(content.c_str(), "ON") == 0) {
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Turn on relay");
  }
  else if (strcmp(content.c_str(), "OFF") == 0) {
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Turn off relay");
  }
  else {
    return false;
  }

  return true;
}
#endif

void loRaRxMode() {
  loRaSetLedStatus(false);
  initLoRa();
  loRaSetLedStatus(true);

  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
  Serial.println("################# RX Mode #################");
}

void loRaTxMode() {
  loRaSetLedStatus(false);
  initLoRa();
  loRaSetLedStatus(true);

  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();               // normal mode
  Serial.println("################# TX Mode #################");
}

void LoRaSendMessage(String message) {
  loRaTxMode();
  delay(100);

  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it

  Serial.println("Msg Sent");
}

#if SWITCH
void processReceivedData() {
  String message = "";

  while (LoRa.available()) {
    message += (char)LoRa.read();
  }         // have to implement encryption and Decryption algo for security

  Serial.print("Received msg to Node: ");
  Serial.println(message);    // NodeFrom;NodeTo;Id;Content -> 0;1;1;"m:ON"

  Message rxData = parseLoRaMessage(message);

  vector<int> processedIds;

  if (rxData.NodeTo == NODE_ID ) {
    switch (rxData.Type[0]) {
      case 'm':
        processedIds = Mq.getProcessedIds();

        if (count(processedIds.begin(), processedIds.end(), rxData.Id) > 0) {
          Serial.println("Message already processed");
          return;
        }

        Serial.printf("Parsed String in Callback - %d, %d, %d, %s, %s\n", rxData.NodeFrom, rxData.NodeTo, rxData.Id, rxData.Type, rxData.Content);

        if (executeCommand(rxData.Content)) {
          delay(500);
          LoRaSendMessage(getAckMessage(rxData));

          Mq.addProcessedMsgId(rxData.Id);
        }
        else {
          LoRaSendMessage(getErrorMessage(rxData));
        }
        break;
      case 'a':
        int index;
        index = Mq.isIdPresent(rxData.Id);

        if (index == -1) {
          break;
        }

        Mq.ackMessage(index);
        Serial.println("Sensor Ack Received");
        break;
      default:
        break;
    }
  }
}
#endif
// #################### CALLBACKS ####################

void onReceive(int packetSize) {
  ReadReceivedData = true;
}

void onTxDone() {
  Serial.println("TxDone");
  loRaRxMode();
}

void clearQueue() {
  Mq.clearProcessedMsgs();
  Serial.println("Msg ID Queue cleared");
}

#if SWITCH
void heartBeatSignal() {
  String heartBeatMsg = getHeartBeatMessage();
  LoRaSendMessage(heartBeatMsg);
  Serial.println(heartBeatMsg);
}
#endif

#if SENSOR
void sensorData() {
  String sensorData = getSensorData();
  LoRaSendMessage(sensorData);
  //Serial.println("loop");
  Serial.println(sensorData);
}
#endif

void initLoRa() {
  digitalWrite(LORA_STATUS_OK, LOW);
  digitalWrite(LORA_STATUS_FAIL, LOW);

  LoRa.setPins(CS_PIN, RST_PIN, IRQ_PIN);

  if (!LoRa.begin(FREQUENCY)) {
    Serial.println("LoRa init failed. Check your connections.");
    loRaSetLedStatus(false);
    while (true);                       // if failed, do nothing
  }

  LoRa.setSyncWord(SYNC_WORD);
  Serial.printf("LoRa Node-%d init succeeded.\n", NODE_ID);

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
}

void setup() {
  Serial.begin(115200);
  Serial.println(VERSION);

#if SWITCH
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  HeartBeatSignal.attach(HEART_BEAT_INTERVAL, heartBeatSignal);
#endif

  pinMode(LORA_STATUS_OK, OUTPUT);
  pinMode(LORA_STATUS_FAIL, OUTPUT);

  loRaSetLedStatus(false);
  loRaRxMode();
  loRaSetLedStatus(true);

  ClearMsgIdQueue.attach(CLEAR_INTERVAL, clearQueue);

#if SENSOR
  compass.init();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
#endif
}

void loop() {
#if SWITCH
  if (ReadReceivedData) {
    processReceivedData();

    ReadReceivedData = false;
  }
#endif

#if SENSOR
  sensorData();

  delay(500);
  Serial.flush();
  esp_light_sleep_start();
#endif

  delay(100);
}
