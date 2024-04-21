#include <FastLED.h>

#define DATA_PIN 26 //live
//#define DATA_PIN 5 //ESP32 Firebeetle
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

#define LEDGreen 0x008000
#define LEDRed  0xFF0000
#define LEDBlue 0x0000FF
#define LEDOrange 0xFFA500
#define LEDYellow 0xFFFF00
#define LEDLightSteelBlue 0xB0C4DE
#define LEDBlack 0x000000
unsigned long ledTimer1 = 0, ledTimer2 = 0, wait = 0, ledMillis = 0;

void ledSetup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
}


void LedBlinker(bool ledEnable, unsigned long delayLEDTime1, int colour1, unsigned long delayLEDTime2, int colour2, int repeats)
{
  if (ledEnable)
  {
    for (int i = 0; i < repeats; i++)
    {

      leds[0] = colour1;
      FastLED.show();
      delay(delayLEDTime1);
      leds[0] = CRGB::Black;
      FastLED.show();

      leds[0] = colour2;
      FastLED.show();
      delay(delayLEDTime2);
      leds[0] = CRGB::Black;
      FastLED.show();


    }
    leds[0] = CRGB::Black;
    FastLED.show();
  }
}
