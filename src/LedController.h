#pragma once
#include <array>

#include <FastLED.h>


#define LED_TYPE    WS2811

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;


// Move all implementation to .cpp file
template <size_t NUM_LEDS, size_t LED_PIN, EOrder COLOR_ORDER>
class LedController
{
    constexpr static size_t BRIGHTNESS{18u};
    constexpr static size_t UPDATES_PER_SECOND{10u};

public:
    LedController(size_t brightness = BRIGHTNESS, size_t updatesPerSecond = UPDATES_PER_SECOND)
    : mBrightness(brightness)
    , mUpdatesPerSecond(updatesPerSecond)
    {
    }

    void setup()
    {
        delay( 3000 ); // power-up safety delay
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(mLeds.data(), NUM_LEDS);
        FastLED.setCorrection( TypicalLEDStrip );
        FastLED.setBrightness(  BRIGHTNESS );
        mCurrentPalette = RainbowColors_p;
        mCurrentBlending = LINEARBLEND;
    }
private:
    void FillLEDsFromPaletteColors( uint8_t colorIndex)
    {
        uint8_t brightness = 255;
        
        for( int i = 0; i < NUM_LEDS; ++i) {
            mLeds[i] = ColorFromPalette( mCurrentPalette, colorIndex, brightness, mCurrentBlending);
            colorIndex += 3;
        }
    }
    // This function fills the palette with totally random colors.
    void SetupTotallyRandomPalette()
    {
        for( int i = 0; i < 16; ++i) {
            mCurrentPalette[i] = CHSV( random8(), 255, random8());
        }
    }

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( mCurrentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    mCurrentPalette[0] = CRGB::White;
    mCurrentPalette[4] = CRGB::White;
    mCurrentPalette[8] = CRGB::White;
    mCurrentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    mCurrentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}
// Additional notes on FastLED compact palettes:
//
// Normally, in computer graphics, the palette (or "color lookup table")
// has 256 entries, each containing a specific 24-bit RGB color.  You can then
// index into the color palette using a simple 8-bit (one byte) value.
// A 256-entry color palette takes up 768 bytes of RAM, which on Arduino
// is quite possibly "too many" bytes.
//
// FastLED does offer traditional 256-element palettes, for setups that
// can afford the 768-byte cost in RAM.
//
// However, FastLED also offers a compact alternative.  FastLED offers
// palettes that store 16 distinct entries, but can be accessed AS IF
// they actually have 256 entries; this is accomplished by interpolating
// between the 16 explicit entries to create fifteen intermediate palette
// entries between each pair.
//
// So for example, if you set the first two explicit entries of a compact 
// palette to Green (0,255,0) and Blue (0,0,255), and then retrieved 
// the first sixteen entries from the virtual palette (of 256), you'd get
// Green, followed by a smooth gradient from green-to-blue, and then Blue.
void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { mCurrentPalette = RainbowColors_p;         mCurrentBlending = LINEARBLEND; }
        if( secondHand == 10)  { mCurrentPalette = RainbowStripeColors_p;   mCurrentBlending = NOBLEND;  }
        if( secondHand == 15)  {mCurrentPalette = RainbowStripeColors_p;   mCurrentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             mCurrentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              mCurrentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       mCurrentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       mCurrentBlending = LINEARBLEND; }
        if( secondHand == 40)  { mCurrentPalette = CloudColors_p;           mCurrentBlending = LINEARBLEND; }
        if( secondHand == 45)  { mCurrentPalette = PartyColors_p;           mCurrentBlending = LINEARBLEND; }
        if( secondHand == 50)  { mCurrentPalette = myRedWhiteBluePalette_p; mCurrentBlending = NOBLEND;  }
        if( secondHand == 55)  { mCurrentPalette = myRedWhiteBluePalette_p; mCurrentBlending = LINEARBLEND; }
    }
}
public:
static void LoopTask(void* parameter)
{
    for(;;)
    {
        auto t = static_cast<LedController*>(parameter);
        t->ChangePalettePeriodically();
        
        static uint8_t startIndex = 0;
        startIndex = startIndex + 1; /* motion speed */
        
        t->FillLEDsFromPaletteColors( startIndex);
        
        FastLED.show();
        // FastLED.delay(1000 / UPDATES_PER_SECOND);

        vTaskDelay(pdMS_TO_TICKS(1000 / UPDATES_PER_SECOND)); 

    }
}

void startTask()
{
      xTaskCreate(
    LoopTask,      /* Task function */
    "LoopTask",    /* String with name of task */
    4096,         /* Stack size in words */
    this,         /* Parameter passed as input to the task */
    1,            /* Priority of the task */
    mTaskHandle   /* Task handle to keep track of created task */
  );
}

private:
    const size_t mBrightness;
    const size_t mUpdatesPerSecond;
    std::array<CRGB, NUM_LEDS> mLeds;
    CRGBPalette16 mCurrentPalette;
    TBlendType    mCurrentBlending;

    TaskHandle_t * const mTaskHandle{};

};

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};

