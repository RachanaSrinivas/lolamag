#include <Wire.h>
#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <ArduinoJson.h>
#include <QMC5883LCompass.h>

#define VERSION "2.0"
#define NODE_ID 3
#define SYNC_WORD 0xFF

#define MSG_DELIMITER ";"
#define CMD_DELIMITER ":"

#define RELAY_PIN 13
#define RELAY_TIMER_PIN 4
#define LORA_STATUS_OK 27       // Green
#define LORA_STATUS_FAIL 26     // Red
#define CS_PIN 5          // LoRa radio chip select
#define RST_PIN 14        // LoRa radio reset
#define IRQ_PIN 2         // change for your board; must be a hardware interrupt pin

#define CLEAR_INTERVAL 100
#define HEART_BEAT_INTERVAL 31
#define SENSOR_INTERVAL 5

#define SWITCH true
#define SENSOR false

const long FREQUENCY = 868E6;  // LoRa Frequency
