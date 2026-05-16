#include <FastLED.h>

#define DATA_PIN 26 //live
//#define DATA_PIN 5 //ESP32 Firebeetle
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

#define LEDGreen 0x008000
#define LEDRed  0xFF0000
#define LEDBlue 0x0000FF
#define LEDCyan 0x00FFFF
#define LEDOrange 0xFFA500
#define LEDYellow 0xFFFF00
#define LEDLightSteelBlue 0xB0C4DE
#define LEDBlack 0x000000

// --- Non-blocking LED state variables ---
enum LedState { IDLE, STATE_1, STATE_2, STATE_CYCLE_END };
LedState currentLedState = IDLE;
unsigned long ledStateTimer = 0;
unsigned long state1_delay = 0;
unsigned long state2_delay = 0;
int state1_color = 0;
int state2_color = 0;
int repeat_count = 0;
int max_repeats = 0;
bool led_enabled_pattern = false;

void ledSetup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  leds[0] = CRGB::Black;
  FastLED.show();
}

// --- New Non-Blocking LED Functions ---

// Call this function from your main loop()
void updateLed() {
    if (!led_enabled_pattern || currentLedState == IDLE) {
        return;
    }

    unsigned long currentMillis = millis();

    switch (currentLedState) {
        case STATE_1:
            if (currentMillis - ledStateTimer >= state1_delay) {
                leds[0] = state2_color;
                FastLED.show();
                ledStateTimer = currentMillis;
                currentLedState = STATE_2;
            }
            break;

        case STATE_2:
            if (currentMillis - ledStateTimer >= state2_delay) {
                // End of one full cycle (color1 -> color2)
                currentLedState = STATE_CYCLE_END;
            }
            break;
        
        case STATE_CYCLE_END:
            repeat_count++;
            if (repeat_count >= max_repeats) {
                // Pattern finished
                leds[0] = CRGB::Black;
                FastLED.show();
                currentLedState = IDLE;
                led_enabled_pattern = false; // Stop processing until a new pattern is started
            } else {
                // Start next repeat
                leds[0] = state1_color;
                FastLED.show();
                ledStateTimer = currentMillis;
                currentLedState = STATE_1;
            }
            break;

        case IDLE:
            // Do nothing
            break;
    }
}

// This function replaces the old LedBlinker. It just sets up the pattern.
void startLedPattern(bool enable, unsigned long delay1, int color1, unsigned long delay2, int color2, int repeats) {
    if (!enable) {
        currentLedState = IDLE;
        leds[0] = CRGB::Black;
        FastLED.show();
        led_enabled_pattern = false;
        return;
    }

    // Allow a new pattern to interrupt an old one.
    led_enabled_pattern = true;
    state1_delay = delay1;
    state2_delay = delay2;
    state1_color = color1;
    state2_color = color2;
    max_repeats = repeats;
    repeat_count = 0;

    // Start the pattern immediately
    leds[0] = state1_color;
    FastLED.show();
    ledStateTimer = millis();
    currentLedState = STATE_1;
}

// --- Old blocking function (to be replaced) ---
void LedBlinker(bool ledEnable, unsigned long delayLEDTime1, int colour1, unsigned long delayLEDTime2, int colour2, int repeats)
{
    startLedPattern(ledEnable, delayLEDTime1, colour1, delayLEDTime2, colour2, repeats);
}
