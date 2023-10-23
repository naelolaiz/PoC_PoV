#pragma once
#include <array>
#include <atomic>
//#include <SPIFFS.h>
#include <FastLED.h>
#include "helloworld.h"

struct RGB {
  byte r;
  byte g;
  byte b;
};

#define LED_TYPE    WS2811

// Move all implementation to .cpp file
template <size_t NUM_LEDS, size_t LED_PIN, EOrder COLOR_ORDER>
class LedController
{
    constexpr static size_t BRIGHTNESS{18u};
    constexpr static double UPDATES_PER_SECOND {170u};
    constexpr static size_t imageHeight = 77u;
    constexpr static size_t imageWidth = 360u;

public:
    LedController(size_t brightness = BRIGHTNESS)
    : mBrightness(brightness)
    {
    }

    void setup()
    {

        // SPIFFS.begin();
        // mBmpFile = SPIFFS.open("/helloworld.bmp", "r");
        // if (!mBmpFile)
        // {
        //     Serial.println("Failed to open file");
        //     return;
        // }
        delay( 3000 ); // power-up safety delay
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(mLeds.data(), NUM_LEDS);
        FastLED.setCorrection( TypicalLEDStrip );
        FastLED.setBrightness(  BRIGHTNESS );
    }
private:
    // void readRGBBitmapColumn(int col)
    // {
    //     constexpr size_t BYTES_PER_PIXEL{3u};
    //     constexpr size_t BMP_HEADER_SIZE{54u};
    //     constexpr size_t ROW_PADDING{4u};

    //     // Calculate the position of the first pixel in the column
    //     size_t pixel_pos = BMP_HEADER_SIZE + col * BYTES_PER_PIXEL;
    //     size_t row_padding = (ROW_PADDING - ((imageWidth * BYTES_PER_PIXEL) % ROW_PADDING)) % ROW_PADDING;

    //     // Read the RGB values of each pixel in the column
    //     for (size_t y = 0; y < imageHeight; ++y)
    //     {
    //         // Calculate the position of the pixel in the file
    //         size_t pos = pixel_pos + (imageHeight - y - 1) * (imageWidth * BYTES_PER_PIXEL + row_padding);

    //         // Read the RGB values of the pixel
    //         char rgb[BYTES_PER_PIXEL];
    //         mBmpFile.seek(pos);
    //         mBmpFile.readBytes(rgb, BYTES_PER_PIXEL);

    //         // Set the corresponding LEDs to the RGB values
    //         mLeds[y] = CRGB(rgb[2], rgb[1], rgb[0]);
    //         mLeds[NUM_LEDS - 1 - y] = mLeds[y];
    //     }

    // }

    void readRGBBitmapColumn(size_t col) 
    {
        col*=3;
        std::vector<std::vector<uint8_t>> columnRGB;

        if (col >= imageWidth) 
        {
            Serial.println("readRGBBitmapColumn requested column out of bounds");
            return; // Column out of bounds
        }

        for (size_t row = 0; row < imageHeight; ++row) 
        {

            int index = (row * imageWidth + col) * 3;  // Each pixel takes up 3 bytes

            const uint8_t red = hello_world_bitmap.pixel_data[index];
            const uint8_t green = hello_world_bitmap.pixel_data[index + 1];
            const uint8_t blue = hello_world_bitmap.pixel_data[index + 2];
            // Set the corresponding LEDs to the RGB values
            mLeds[row] = CRGB(red, green, blue);
            mLeds[NUM_LEDS - 1 - row] = mLeds[row];
        }
    }
public:
static void LoopTask(void* parameter)
{
            
    constexpr size_t MID_LEDS {NUM_LEDS/2};
    constexpr static double maxValue = 220.;
    constexpr static double threshold = 35.;
    auto t = static_cast<LedController*>(parameter);
    for(;;)
    {

        const double constantVelocity = t->mConstantVelocity;
        const double absConstantVelocity = abs(constantVelocity);
        if (absConstantVelocity > threshold)
        {
            // Clear all LEDs
            FastLED.clear();
        }

        if (constantVelocity > threshold)
        {
            //t->mLeds[i] = CRGB::Green; // Left half
            //t->mLeds[NUM_LEDS - 1 - i] = CRGB::Green; // Right half
           if(!t->mDisplayingBmp)
           {
              t->mDisplayingBmp = true;
              t->mCurrentColumn = 0;
           }
           t->readRGBBitmapColumn(t->mCurrentColumn);
//           t->mCurrentColumn = (t->mCurrentColumn+1) % imageWidth;
           t->mCurrentColumn = (t->mCurrentColumn+1) % int(imageWidth/3);
        }
        else 
        {

            t->mDisplayingBmp = false;
            t->mCurrentColumn = 0;
            const double multiplier = min(maxValue, absConstantVelocity) / maxValue;
            // Animate from ends to the center based on gyro movement
            for (int i = 0; i <= MID_LEDS; ++i) 
            {
                if (constantVelocity < -threshold)
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
        }
      
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
    //File mBmpFile{};
    std::atomic<bool> mDisplayingBmp{false};
    std::atomic<size_t> mCurrentColumn{};
};


