#include <Arduino.h>
#include "FastLED.h"


#define NUM_LEDS      16
#define LED_TYPE   WS2812B
#define COLOR_ORDER   GRB
#define DATA_PIN        3
#define VOLTS           5
#define MAX_MA          20



constexpr size_t FPS=140;
constexpr size_t FRAME_TIME_MS=1000/FPS;




CRGB leds[NUM_LEDS];

// Background color for 'unlit' pixels
// Can be set to CRGB::Black if desired.
CRGB gBackgroundColor = CRGB::Black; 
// Example of dim incandescent fairy light background color
// CRGB gBackgroundColor = CRGB(CRGB::FairyLight).nscale8_video(16);


void setup() {
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
      leds[dot] = gBackgroundColor;
  }
  delay( 3000 ); //safety startup delay

  FastLED.setMaxPowerInVoltsAndMilliamps( VOLTS, MAX_MA);
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip);
  FastLED.show();
}




  void loop() {
      const auto color = CRGB( random(10,255),random(10,255),random(10,255));
      for(int dot = 0; dot < NUM_LEDS; dot++) { 
          leds[dot] = color;
          FastLED.show();
          // clear this led for the next time around the loop
          leds[dot] = gBackgroundColor;
          delay(FRAME_TIME_MS);
      }
      for(int dot = NUM_LEDS-1; dot >=0; dot--) { 
          leds[dot] = color;
          FastLED.show();
          // clear this led for the next time around the loop
          leds[dot] = gBackgroundColor;
          delay(FRAME_TIME_MS);
      }

}
