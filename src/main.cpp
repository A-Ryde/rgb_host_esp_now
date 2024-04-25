#include <Arduino.h> 
#include <atomic>
#include <esp_now.h>
#include <stripLED.hpp>
#include <WiFi.h>

typedef struct led_data_t
{
  uint8_t led_state; 
  uint8_t led_brightness;
} led_data_t;

led_data_t current_state; 

esp_now_peer_info_t peer_info;

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.printf("Last Packet Send Status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

void on_receive_callback(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  led_data_t data_recieved;
  memcpy(&data_recieved, data, sizeof(data));
  LED::g_led_state.store(static_cast<LED::led_state_t>(data_recieved.led_state));
  LED::setBrightness(data_recieved.led_brightness);
}

void setup() 
{
  vTaskDelay(1000); //to stop boot lock

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(on_data_sent);
  esp_now_register_recv_cb(on_receive_callback);

  Serial.println(WiFi.macAddress());
}

void loop() 
{
  current_state.led_state = static_cast<uint8_t>(LED::g_led_state.load());
}