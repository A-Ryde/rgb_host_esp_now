#include <stripLED.hpp>

#define CHIPSET WS2811
#define COLOR_ORDER RGB
#define LED_PIN 48
const uint8_t BRIGHTNESS = 255;

const uint16_t MAX_LEDS = 500;
uint16_t g_num_leds = 0;

namespace LED
{ 
    std::atomic<led_state_t> g_led_state;
    std::atomic<uint8_t> g_led_brightness;

    CRGB leds[MAX_LEDS];

    void FillLEDsFromPaletteColors(uint8_t colorIndex, CRGBPalette16 palette, TBlendType blend)
    {
        uint8_t brightness = 255;
        
        for( int i = 0; i < g_num_leds; ++i) {
            leds[i] = ColorFromPalette( palette, colorIndex, brightness, blend);
            colorIndex += 3;
        }
    }

    void setState(led_state_t led_state)
    {
        g_led_state.store(led_state);
    }

    led_state_t getState(void)
    {
        return g_led_state.load();
    }

    void toggleState(void)
    {
        ESP_LOGD("LED", "Toggle State ISR Triggered");
        uint8_t current_state = static_cast<uint8_t>(g_led_state.load());   
        current_state ++; 
        if(current_state == led_state_t::END_OF_STATES)
        {
            //LED state "off" will always be first state, increment by 1 to roll around
            g_led_state.store(static_cast<led_state_t>(static_cast<uint8_t>(led_state_t::off) + 1));
        }
        else
        {
            g_led_state.store(static_cast<led_state_t>(current_state));
        }
    }

    void reverseToggleState(void)
    {
        ESP_LOGD("LED", "Reverse Toggle State ISR Triggered");
        uint8_t current_state = static_cast<uint8_t>(g_led_state.load());   
        current_state --; 
        if(current_state == led_state_t::off)
        {
            //LED state "END OF STATES" will alwasy be final state, simply decrement by 1 to roll around;
            g_led_state.store(static_cast<led_state_t>(static_cast<uint8_t>(led_state_t::END_OF_STATES) - 1));
        }
        else
        {
            g_led_state.store(static_cast<led_state_t>(current_state));
        }
    }

    void setBrightness(uint8_t brightness)
    {
        g_led_brightness = constrain(brightness, 0, 255);
        FastLED.setBrightness(brightness);
    }

    uint8_t getBrightness(void)
    {
        return g_led_brightness.load();
    }

    void increaseBrightness(std::optional<uint8_t> brightness_amount)
    {
        uint8_t current_brightness = FastLED.getBrightness();
        
        if(brightness_amount.has_value())
        {
            constrain(g_led_brightness += brightness_amount.value(), 0, 255);
            FastLED.setBrightness(g_led_brightness);
        }
        else
        {
            const uint8_t brightness_adjust_amount = 51; //20% increment (of 255)
            constrain(g_led_brightness += brightness_adjust_amount, 0, 255);
            FastLED.setBrightness(g_led_brightness);
        }
    }

    void decreaseBrightness(std::optional<uint8_t> brightness_amount)
    {
        uint8_t current_brightness = FastLED.getBrightness();
        
        if(brightness_amount.has_value())
        {
            constrain(g_led_brightness -= brightness_amount.value(), 0, 255);
            FastLED.setBrightness(g_led_brightness);
        }
        else
        {
            const uint8_t brightness_adjust_amount = 51; //20% decrement (of 255)
            constrain(g_led_brightness -= brightness_adjust_amount, 0, 255);
            FastLED.setBrightness(g_led_brightness);
        }
    }

    void updateColourPalette(void)
    {
        CRGBPalette16 palette;
        TBlendType blend = LINEARBLEND;

        switch(g_led_state.load())
        {
            case led_state_t::off:
                fill_solid(palette, 16, CRGB::Black);
                break;
            case led_state_t::white:
                fill_solid(palette, 16, CRGB::White);
                break;
            case led_state_t::yellow:
                fill_solid(palette, 16, CRGB::Yellow);
                break;
            case led_state_t::green:
                fill_solid(palette, 16, CRGB::Green);
                break;  
            case led_state_t::red:
                fill_solid(palette, 16, CRGB::Red);
                break;
            case led_state_t::purple:
                fill_solid(palette, 16, CRGB::Purple);
                break;
            case led_state_t::blue:
                fill_solid(palette, 16, CRGB::Blue);
                break;
            case led_state_t::cyan:
                fill_solid(palette, 16, CRGB::Cyan);
                break;
            case led_state_t::ocean:
                palette = OceanColors_p;
                break;
            case led_state_t::cloud:
                palette = CloudColors_p;
                break;
            case led_state_t::lava:
                palette = LavaColors_p;
                break;
            case led_state_t::forest:
                palette = ForestColors_p;
                break;
            case led_state_t::party:
                palette = PartyColors_p;
                break;
             case led_state_t::rainbow:
                palette = RainbowColors_p;
                break;
            case led_state_t::END_OF_STATES:
                //Should be impossible 
                break;
            default: 
                 fill_solid(palette, 16, CRGB::Black);
                break;
            }

        static uint8_t startIndex = 0;
        startIndex++;
        FillLEDsFromPaletteColors(startIndex, palette, blend);
        FastLED.show();
    }
    
    void updateLEDs(void *pvParams)
    {
        const TickType_t xDelayLED = pdMS_TO_TICKS(20);

        for (;;)
        {
            updateColourPalette();

            vTaskDelay(xDelayLED);
        }
    }

    void startLEDs(uint16_t num_leds)
    {
        g_num_leds = num_leds;
        FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, num_leds).setCorrection(TypicalLEDStrip);
        g_led_brightness = BRIGHTNESS;
        FastLED.setBrightness(g_led_brightness);
        xTaskCreate(updateLEDs, "updateLED", 8192, NULL, 1, NULL);
    }

}; //namespace