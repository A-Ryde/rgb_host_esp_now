#include <Arduino.h> 
#include <atomic>
#include <esp_now.h>
#include <stripLED.hpp>
#include <WiFi.h>

/*
  *Firmware for independant LED controller - Control Side*

  Use case is to be used to control RGB Lights on a Yacht, with 2 switching devices
  each of the 2 sets of controllers (Front and Back) consists of 2 controlers, Left and Right.

  The Right controllers on the boat will be configured as slaves to the corresponding Left Controller
  as such they will always copy the state of the left controller.

  The left controller 'control side', will be sending the states to the right controller.
  States will be incremented when one of the switches is toggled (Front or Back)

  As the left controller will be off, it will handshake with the existing controller to increment the state when started. 

  In addition to this, there will be a master controller, separate with button controls for the whole network. 
*/

#define FRONT_CONTROLLER
const uint16_t num_leds = 300;

uint8_t master_mac_addr[] = {0xDC, 0x54, 0x75, 0xF1, 0xE1, 0xE0};

#ifdef FRONT_CONTROLLER 
  //host mac refers to host of rear controller
  uint8_t host_mac_addr[] = {0xDC, 0x54, 0x75, 0xF1, 0xE2, 0x38}; 
  uint8_t slave_mac_addr[] = {0xDC, 0x54, 0x75, 0xF1, 0xE1, 0x34};
#endif

#ifdef REAR_CONTROLLER
  //host mac refers to host of front controller
  uint8_t host_mac_addr[] = {0xDC, 0x54, 0x75, 0xF1, 0xE2, 0x20}; 
  uint8_t slave_mac_addr[] = {0xDC, 0x54, 0x75, 0xF1, 0xE1, 0x58};
#endif

LED::led_state_t DEFAULT_STARTUP_STATE = LED::led_state_t::white;

typedef struct led_data_t
{
  uint8_t led_state; 
  uint8_t led_brightness;
} led_data_t;

esp_now_peer_info_t slave_info;
esp_now_peer_info_t host_controller_info; 
esp_now_peer_info_t master_controller_info;

std::atomic<bool> has_previously_connected = false;
uint32_t connection_time;

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.printf("Last Packet Send Status: %s \n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

void on_startup_rx_state_update(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  if(memcmp(mac_addr, host_mac_addr, sizeof(host_mac_addr)) == 0)
  {
    if(data[0] == 255 && has_previously_connected)
    {    
      //Other host has been toggled in the last 5 seconds, move state up by 1, and send update to whole network
      //Note has_previously_connected bool has a timeout such that the network wont update state just from turning on other side
      LED::toggleState();

      led_data_t current_led_state_info; 
      current_led_state_info.led_state = LED::getState();
      current_led_state_info.led_brightness = LED::getBrightness();
      esp_now_send(slave_mac_addr, (uint8_t *) &current_led_state_info, sizeof(current_led_state_info));
      vTaskDelay(1);
      esp_now_send(host_mac_addr, (uint8_t *) &current_led_state_info, sizeof(current_led_state_info));
    }

    if(data[0] == 255 && !has_previously_connected)
    {
      //Other Host has just turned on, send it current LED state information
      has_previously_connected = true;
      connection_time = millis();

      led_data_t current_led_state_info; 
      current_led_state_info.led_state = LED::getState();
      current_led_state_info.led_brightness = LED::getBrightness();
      esp_now_send(host_mac_addr, (uint8_t *) &current_led_state_info, sizeof(current_led_state_info));
    }

    if(!has_previously_connected)
    {
      //You are recieving state update from other host
      has_previously_connected = true;
      connection_time = millis();

      led_data_t data_recieved;
      memcpy(&data_recieved, data, sizeof(data));
      LED::setState(static_cast<LED::led_state_t>(data_recieved.led_state));
      LED::setBrightness(data_recieved.led_brightness);

      led_data_t current_led_state_info; 
      current_led_state_info.led_state = LED::getState();
      current_led_state_info.led_brightness = LED::getBrightness();
      esp_now_send(slave_mac_addr, (uint8_t *) &current_led_state_info, sizeof(current_led_state_info));

      Serial.printf("State: %i, Brightness: %i \n", data_recieved.led_state, data_recieved.led_brightness);
    }
  }

  if (memcmp(mac_addr, master_mac_addr, 6) == 0)
  {
    led_data_t data_recieved;
    memcpy(&data_recieved, data, sizeof(data));
    LED::setState(static_cast<LED::led_state_t>(data_recieved.led_state));
    LED::setBrightness(data_recieved.led_brightness);

    Serial.printf("State: %i, Brightness: %i \n", data_recieved.led_state, data_recieved.led_brightness);
  }
}

void sync_controllers(void* pvParams);
void monitor_timeout(void* pvParams);

void setup() 
{
  Serial.begin(115200);

  LED::startLEDs(num_leds);
  LED::setState(DEFAULT_STARTUP_STATE);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW \n");
    return;
  }

  esp_now_register_send_cb(on_data_sent);
  esp_now_register_recv_cb(on_startup_rx_state_update);

  memcpy(slave_info.peer_addr, &slave_mac_addr, sizeof(slave_mac_addr));
  slave_info.channel = 0;
  slave_info.encrypt = false;
  esp_now_add_peer(&slave_info);

  memcpy(host_controller_info.peer_addr, &host_mac_addr, sizeof(host_mac_addr));
  host_controller_info.channel = 1;
  host_controller_info.encrypt = 0;
  esp_now_add_peer(&host_controller_info);

  memcpy(master_controller_info.peer_addr, &master_mac_addr, sizeof(host_mac_addr));
  master_controller_info.channel = 2;
  master_controller_info.encrypt = 0;

  uint8_t startup_code[1] = {255};

  vTaskDelay(1);
  esp_now_send(host_mac_addr, (uint8_t *) startup_code, sizeof(startup_code));

  xTaskCreate(sync_controllers, "sycncControllers", 2048, NULL, 1, NULL);
}

void loop() 
{
  vTaskSuspend(NULL);
}

void sync_controllers(void* pvParams)
{
  for(;;)
  //front controller syncs between rear and slave
  //rear contorller syncs to its slave device
  {
      led_data_t current_led_state_info; 
      current_led_state_info.led_state = LED::getState();
      current_led_state_info.led_brightness = LED::getBrightness();

    #ifdef FRONT_CONTROLLER
      esp_now_send(slave_mac_addr, (uint8_t *) &current_led_state_info, sizeof(current_led_state_info));
      vTaskDelay(10);
      esp_now_send(host_mac_addr, (uint8_t *) &current_led_state_info, sizeof(current_led_state_info));
    #endif

    #ifdef REAR_CONTROLLER
      esp_now_send(slave_mac_addr, (uint8_t *) &current_led_state_info, sizeof(current_led_state_info));
    #endif

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void monitor_timeout(void* pvParams)
{
  for(;;)
  {
    uint32_t connection_timeout = 5000;
    if(connection_timeout + connection_time < millis())
    {
      has_previously_connected = false;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}