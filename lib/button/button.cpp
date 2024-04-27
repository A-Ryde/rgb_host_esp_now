#include "button.hpp"

namespace button
{
    button::button(gpio_num_t pin)
    : m_pin(pin)
    {
        pinMode((uint8_t)m_pin, INPUT_PULLUP);
        attachInterrupt((uint8_t)m_pin, std::bind(&button::button_isr, this), FALLING);
    }

    button::~button()
    {
        detachInterrupt(m_pin);
    }

    void button::button_isr()
    {
        uint16_t button_debounce_timeout = 50; //ms
        int64_t current_time = esp_timer_get_time();
        if(current_time > button_debounce_timeout + m_last_trigger)
        {
            call();
            m_last_trigger = current_time;
        }
    }

    void button::call(void)
    { 
    if(function_callback != NULL) 
    {
        function_callback();
    }
    else
    {
        ESP_LOGI("BUTTON", "Button Pressed, No Callback Set...");
    }
}

}; //namespace