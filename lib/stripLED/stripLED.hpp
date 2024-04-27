#ifndef STRIP_LED_HPP
#define STRIP_LED_HPP

#include <stdint.h>
#include <atomic>
#include <memory>
#include <optional>
#include <FastLED.h>

namespace LED
{
    enum led_state_t
    {
        off = 0,
        white, 
        yellow,
        green,
        red, 
        purple, 
        blue, 
        cyan,
        ocean,
        cloud, 
        lava,
        forest,
        party, 
        rainbow,
        END_OF_STATES
    };

    //Set LED
    void FillLEDsFromPaletteColors( uint8_t colorIndex, CRGBPalette16 palette, TBlendType blend);

    //LED State
    void setState(led_state_t state); //explicitly set specific state
    led_state_t getState(void);
    void toggleState(void); //increment state by one
    void reverseToggleState(void); //revert to previous state

    //LED Brightness
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness(void);
    void increaseBrightness(std::optional<uint8_t> brightness_amount);
    void decreaseBrightness(std::optional<uint8_t> brightness_amount);   
    
    //LED Functionality 
    void updateColourPallette(void);
    void updateLEDs(void *pvParams);
    void startLEDs(uint16_t num_leds);

}; //namespace

#endif