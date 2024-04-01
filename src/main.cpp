#include <Arduino.h> 
#include <atomic>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

std::atomic<uint8_t> state = 1;
std::atomic<uint8_t> brightness = 255;

const gpio_num_t BUTTON_NEXT = GPIO_NUM_10;
const gpio_num_t BUTTON_PREV = GPIO_NUM_11;
const gpio_num_t BUTTON_PLUS = GPIO_NUM_46;
const gpio_num_t BUTTON_MINUS = GPIO_NUM_9;

void nextISR(void)
{
  ESP_LOGI("ISR", "Next State");
  state = state + 1;
}

void prevISR(void)
{
  ESP_LOGI("ISR", "Prev State");
  state = state - 1;
}

void increaseISR()
{
  ESP_LOGI("ISR", "Increase Brightness");
  brightness += 51;
}

void decreaseISR()
{
  ESP_LOGI("ISR", "Decrease Brightness");
  brightness -= 51;
}

BLECharacteristic *pCharacteristic;

void setup() {
  Serial.begin(115200);

  vTaskDelay(1000); //to stop boot lock

  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_PREV, INPUT_PULLUP);

  attachInterrupt(BUTTON_NEXT, nextISR, FALLING);
  attachInterrupt(BUTTON_PREV, prevISR, FALLING);

  Serial.println("Starting BLE work!");

  BLEDevice::init("Long name works now");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue(std::to_string(state.load()));
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
  // put your main code here, to run repeatedly

  pCharacteristic->setValue(std::to_string(state.load()));


  delay(2000);
}