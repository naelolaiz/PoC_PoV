#pragma once
#include <array>

#include <atomic>

#include <FastLED.h>


#define LED_TYPE    WS2811

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;


// Move all implementation to .cpp file
template <size_t NUM_LEDS, size_t LED_PIN, EOrder COLOR_ORDER>
class LedController
{
    constexpr static size_t BRIGHTNESS{18u};
    constexpr static size_t UPDATES_PER_SECOND{100u};

public:
    LedController(size_t brightness = BRIGHTNESS)
    : mBrightness(brightness)
    {
    }

    void setup()
    {
        delay( 3000 ); // power-up safety delay
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(mLeds.data(), NUM_LEDS);
        FastLED.setCorrection( TypicalLEDStrip );
        FastLED.setBrightness(  BRIGHTNESS );
    }
public:
static void LoopTask(void* parameter)
{
            
    constexpr size_t MID_LEDS {NUM_LEDS/2};
    constexpr static double maxValue = 220.;
    constexpr static double threshold = 80.;
    auto t = static_cast<LedController*>(parameter);
    for(;;)
    {

        const double constantVelocity = t->mConstantVelocity;
        const double absConstantVelocity = abs(constantVelocity);
        const double multiplier = min(maxValue, absConstantVelocity) / maxValue;
        if (absConstantVelocity > threshold)
        {
            // Clear all LEDs
            FastLED.clear();
        }

        // Animate from ends to the center based on gyro movement
        for (int i = 0; i <= MID_LEDS; ++i) 
        {
            if (constantVelocity > threshold)
            {
                t->mLeds[i] = CRGB::Green; // Left half
                t->mLeds[NUM_LEDS - 1 - i] = CRGB::Green; // Right half
            }
            else if (constantVelocity < -threshold)
            {
                t->mLeds[i] = CRGB::Cyan; // Left half
                t->mLeds[NUM_LEDS - 1 - i] = CRGB::Cyan; // Right half
            }
            else
            {
                t->mLeds[i] = CRGB::Black; // Left half
                t->mLeds[NUM_LEDS - 1 - i] = CRGB::Black; // Right half
            }
      }
      FastLED.setBrightness(  BRIGHTNESS * multiplier );

      FastLED.show();
      vTaskDelay(pdMS_TO_TICKS(1000 / UPDATES_PER_SECOND));
    }
}


void setInstantVelocity(double velocity)
{
    static constexpr double lengthPipe = 500; // mm. Used to calculate tangent speed
    mConstantVelocity = velocity;
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
    std::array<CRGB, NUM_LEDS> mLeds;
    double mConstantVelocity{};
    TaskHandle_t * const mTaskHandle{};
};
