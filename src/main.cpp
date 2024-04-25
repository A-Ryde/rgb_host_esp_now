#include <Arduino.h> 
#include <atomic>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <button.hpp>
#include <stripLED.hpp>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define DEVICE1

#define SERVICE_UUID      "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

#ifdef DEVICE1
  #define SERVER_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
  #define CLIENT_CHAR_UUID  "99e4ab0b-4186-45d5-9022-082f7acfc2cd"
#endif

#ifdef DEVICE2
  #define SERVER_CHAR_UUID  "99e4ab0b-4186-45d5-9022-082f7acfc2cd"
  #define CLIENT_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#endif

std::atomic<uint8_t> brightness = 255;

const gpio_num_t BUTTON_NEXT = GPIO_NUM_10;
const gpio_num_t BUTTON_PREV = GPIO_NUM_11;
const gpio_num_t BUTTON_PLUS = GPIO_NUM_46;
const gpio_num_t BUTTON_MINUS = GPIO_NUM_9;

button::button next(BUTTON_NEXT);
button::button prev(BUTTON_PREV);

BLECharacteristic *p_led_state_characteristic; //server
BLECharacteristic *p_recieved_characteristic; //client

void setup() 
{
  vTaskDelay(1000); //to stop boot lock

  Serial.begin(115200);

  next.setCallback(LED::toggleState);
  prev.setCallback(LED::reverseToggleState);

  Serial.println("Starting BLE work!");

  BLEDevice::init("sus ble device");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  p_led_state_characteristic = pService->createCharacteristic(
                                         SERVER_CHAR_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  BLEClient *pClient = BLEDevice::createClient();  

  p_led_state_characteristic->setValue(std::to_string(static_cast<uint8_t>(LED::getState())));
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  xTaskCreate(updateState, "update LED state", NULL, 1, NULL);
}

void updateState(void *pvParam)
{
  const ticktype_t xStateDelay = pdMS_TO_TICKS(100);
  
  for(;;)
  {
    p_led_state_characteristic->setValue(std::to_string(LED::getState()));

    vTaskDelay(xStateDelay);
  }

}

void loop() 
{
  vTaskSuspend(NULL);
}