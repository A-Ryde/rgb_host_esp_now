#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <FunctionalInterrupt.h>

namespace button
{
class button

{
private:
    uint32_t m_last_trigger {0};
    gpio_num_t m_pin;


    void (*function_callback)(void);
    void button_isr(void);
    void call(void);  

public:

    void setCallback(void (*function)(void)) { this->function_callback = function; }
    
    button(gpio_num_t pin);
    ~button();
};

}; //namespace
