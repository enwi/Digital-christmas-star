#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

/**
 *Config
 **/
constexpr static const bool DEBUG = true; /// en-/disable debug output

constexpr static const uint8_t DATA_PIN = 0; /// The pin the leds are connected to
constexpr static const uint16_t NUM_FINS = 5; /// The number of fins the start has
constexpr static const uint16_t NUM_LEDS_BETWEEN_TIPS = 9; /// The number of leds between tips (without tip leds)
constexpr static const uint16_t LED_OFFSET = 0; /// Offset for first LED
//#define NO_TIP_AND_PIT_LEDS /// Uncomment this if you do not have LEDS in the tip or pit

// 60000 milliseconds = 1 minute
constexpr static const uint16_t animationTime = 60000; /// The time after which to change animation

constexpr static const uint8_t maxMode = 8; /// The mode count

/**
 * Internal variables
 **/
#if defined(NO_TIP_AND_PIT_LEDS)
constexpr static const uint16_t NUM_LEDS = (NUM_LEDS_BETWEEN_TIPS * NUM_FINS * 2);
constexpr static const uint16_t NUM_LEDS_IN_FIN = (2 * NUM_LEDS_BETWEEN_TIPS);
#else
constexpr static const uint16_t NUM_LEDS = (NUM_FINS * 2 + NUM_LEDS_BETWEEN_TIPS * NUM_FINS * 2);
constexpr static const uint16_t NUM_LEDS_IN_FIN = (2 * NUM_LEDS_BETWEEN_TIPS) + 1;
#endif
uint32_t lastAnimationTime = 0; /// The last time we changed the animation
uint32_t lastAnimationStep = 0; /// The last time we animated a step 1
uint32_t lastAnimationStep2 = 0; /// The last time we animated a step 2
uint8_t currentMode = 0; /// The current mode/animation we are in
uint8_t hue = 0; /// The color of the animation

CRGBArray<NUM_LEDS> leds; // Our leds

// forward declaration of function
bool animate(const uint32_t currentTime, const uint16_t animationStepTime);
bool animate2(const uint32_t currentTime, const uint16_t animationStepTime);
void lerp8(CPixelView<CRGB>& ledArray, const CHSV& color, const uint8_t fraction);
void lerp8(CRGB& color1, const CRGB& color2, const uint8_t fraction);
constexpr uint8_t convertPercent(const double percentage);
template <typename T>
void logParameter(const String& name, const T& value);
void mirrorFirstFin();
void copyFirstFinToAllFins();

bool circleRainbowAnimation(const uint32_t time);
bool rainbow(const uint32_t time);
bool randomTwinkle(const uint32_t time, const uint32_t color1, const uint32_t color2);
bool outsideRainbow(const uint32_t time);
bool circleAnimation(const uint32_t time, const CHSV& color);
bool staticColor(const CHSV& color);
bool staticTwinkle(const uint32_t time, const CHSV& color);
bool outsideWoosh(const uint32_t time);
bool simpleColorFade(const uint32_t time);

void setup()
{
    if (DEBUG)
    {
        Serial.begin(115200); // Enable serial output when debugging
    }
    // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LEDS);
    // FastLED.setMaxPowerInVoltsAndMilliamps(5, 400); // Limit power to 2 Watts (5V * 0.4A)
    FastLED.setBrightness(128);
    leds.fill_solid(0x000000); // Clear all leds
    FastLED.show();
}

void loop()
{
    unsigned long currentMillis = millis();
    if (currentMillis - lastAnimationTime > animationTime) // Change the mode after the given animationTime
    {
        lastAnimationTime = currentMillis;
        ++currentMode;
        hue += 30;
        if (currentMode > maxMode) // Reset to first mode after last mode
        {
            currentMode = 0;
        }
    }

    logParameter("Time", currentMillis);

    bool show = false;
    switch (currentMode) // Run the animation associated with the mode
    {
    case 0:
        show = circleRainbowAnimation(currentMillis);
        break;
    case 1:
        show = rainbow(currentMillis);
        break;
    case 2:
        show = randomTwinkle(currentMillis, 0x00B315, 0xB3000C);
        break;
    case 3:
        show = outsideRainbow(currentMillis);
        break;
    case 4:
        show = circleAnimation(currentMillis, CHSV(hue, 255, 255));
        break;
    case 5:
        show = staticColor(CHSV(hue, 255, 255));
        break;
    case 6:
        show = outsideWoosh(currentMillis);
        break;
    case 7:
        show = simpleColorFade(currentMillis);
        break;
    case 8:
        show = staticTwinkle(currentMillis, CHSV(hue, 255, 255));
        break;
    default:
        currentMode = 0;
    }

    if (show)
    {
        LEDS.show(); // Show the animation
    }
}

bool rainbow(const uint32_t time)
{
    if (!animate(time, 30)) // Every 30 ms update the color
    {
        return false;
    }

    ++hue;
    for (uint8_t i = 0; i < NUM_LEDS; ++i)
    {
        leds[i].setHue((hue - (255 / NUM_LEDS) * i) % 256);
    }
    return true;
}

bool randomTwinkle(const uint32_t time, const uint32_t color1, const uint32_t color2)
{
    if (!animate(time, 100)) // Every 100 ms fade by 20 and set random leds
    {
        return false;
    }

    leds.fadeToBlackBy(20);
    leds[random8(0, NUM_LEDS)] = color1;
    leds[random8(0, NUM_LEDS)] = color2;
    return true;
}

bool weirdAnimation(const uint32_t time)
{
    if (!animate(time, 30)) // Every 30 ms update the color
    {
        return false;
    }

    ++hue;

#if defined(NO_TIP_AND_PIT_LEDS)
    // Fill rainbow between first tips
    for (uint8_t i = 0; i < NUM_LEDS_BETWEEN_TIPS; ++i)
    {
        leds[i].setHue((hue - (255 / NUM_LEDS_BETWEEN_TIPS) * i) % 256);
    }
#else
    // Set tip and inner tip green
    leds[0] = leds[NUM_FINS + 1] = 0x00B315;

    // Fill rainbow between first tips
    for (uint8_t i = 0; i < NUM_LEDS_BETWEEN_TIPS; ++i)
    {
        leds[i + 1].setHue((hue - (255 / NUM_LEDS_BETWEEN_TIPS) * i) % 256);
    }
#endif
    mirrorFirstFin();
    copyFirstFinToAllFins();
    return true;
}

bool outsideWoosh(const uint32_t time)
{
#if defined(NO_TIP_AND_PIT_LEDS)
    constexpr static const uint16_t LED_COUNT = NUM_LEDS_BETWEEN_TIPS;
#else
    constexpr static const uint16_t LED_COUNT = NUM_LEDS_BETWEEN_TIPS + 2;
#endif
    static uint8_t animationStep = 0;

    if (!animate(time, 100)) // Every 100 ms update the color
    {
        return false;
    }

    leds.fadeToBlackBy(convertPercent(300.0 / LED_COUNT));

    if (animationStep < LED_COUNT) // Don't animate all the time to have time to fade to black
    {
        leds[(animationStep + LED_OFFSET) % NUM_LEDS] = CHSV(hue, 255, 255);
        mirrorFirstFin();
        copyFirstFinToAllFins();
        ++hue;
    }

    ++animationStep;
    if (animationStep >= 2 * LED_COUNT)
    {
        animationStep = 0;
    }
    return true;
}

bool outsideRainbow(const uint32_t time)
{
#if defined(NO_TIP_AND_PIT_LEDS)
    constexpr static const uint16_t LED_COUNT = NUM_LEDS_BETWEEN_TIPS;
#else
    constexpr static const uint16_t LED_COUNT = NUM_LEDS_BETWEEN_TIPS + 2;
#endif

    if (!animate(time, 30)) // Every 30 ms update the color
    {
        return false;
    }

    ++hue;
    // Fill rainbow between first tips
    for (uint8_t i = 0; i < LED_COUNT; ++i)
    {
        leds[i].setHue(((hue - (255 / (NUM_LEDS_IN_FIN)) * i) + LED_OFFSET) % 256);
    }

    mirrorFirstFin();
    copyFirstFinToAllFins();
    return true;
}

bool simpleColorFade(const uint32_t time)
{
    if (!animate(time, 30)) // Every 30 ms update the color
    {
        return false;
    }

    ++hue;
    staticColor(CHSV(hue, 255, 255));
    return true;
}

bool staticColor(const CHSV& color)
{
    leds.fill_solid(color);
    return true;
}

bool staticTwinkle(const uint32_t time, const CHSV& color)
{
    bool update = false;
    if (animate2(time, 50)) // Every 50 ms fade to black by 12.5 %
    {
        lerp8(leds, color, convertPercent(12.5)); // 12.5%
        update = true;
    }
    if (animate(time, 150))
    {
        leds[random8(0, NUM_LEDS)] = CRGB::White;
        update = true;
    }
    return update;
}

uint8_t finIndex = 0;
uint8_t animationStep = 0;

bool circleAnimation(const uint32_t time, const CHSV& color)
{
    if (!animate(time, 100)) // Every 100 ms update the color
    {
        return false;
    }

#if defined(NO_TIP_AND_PIT_LEDS)
    // TODO implement
#else
    const uint16_t ledInFinIndex = animationStep % ((2 * NUM_LEDS_BETWEEN_TIPS) + 3);
    // 5 fins -> 0 -> 2 -> 4 -> 1 -> 3 -> 0 -> 2 -> 4 -> 1 -> 3 -> 0
    if (ledInFinIndex == 0)
    {
        finIndex = (finIndex + NUM_FINS / 2) % NUM_FINS;
    }
    const uint16_t ledIndex = (ledInFinIndex + finIndex * ((2 * NUM_LEDS_BETWEEN_TIPS) + 2)) % NUM_LEDS;

    leds.fadeToBlackBy(convertPercent(200.0 / (NUM_LEDS_BETWEEN_TIPS + 2)));
    leds[ledIndex] = color;

    ++animationStep;
    if (animationStep >= ((2 * NUM_LEDS_BETWEEN_TIPS) + 3) * NUM_FINS)
    {
        animationStep = 0;
    }
#endif
    return true;
}

bool circleRainbowAnimation(const uint32_t time)
{
    if (!animate(time, 100)) // Every 100 ms update the color
    {
        return false;
    }

#if defined(NO_TIP_AND_PIT_LEDS)
    // TODO implement
#else
    const uint16_t ledInFinIndex = animationStep % ((2 * NUM_LEDS_BETWEEN_TIPS) + 3);
    // 5 fins -> 0 -> 2 -> 4 -> 1 -> 3 -> 0 -> 2 -> 4 -> 1 -> 3 -> 0
    if (ledInFinIndex == 0)
    {
        finIndex = (finIndex + NUM_FINS / 2) % NUM_FINS;
    }
    const uint16_t ledIndex = ((ledInFinIndex + finIndex * ((2 * NUM_LEDS_BETWEEN_TIPS) + 2)) + LED_OFFSET) % NUM_LEDS;

    leds.fadeToBlackBy(convertPercent(200.0 / (NUM_LEDS_BETWEEN_TIPS + 2)));
    leds[ledIndex] = CHSV(hue, 255, 255);
    ++hue;

    ++animationStep;
    if (animationStep >= ((2 * NUM_LEDS_BETWEEN_TIPS) + 3) * NUM_FINS)
    {
        animationStep = 0;
    }
#endif
    return true;
}

// Converts a given percentage to the FastLED percentage
constexpr uint8_t convertPercent(const double percentage)
{
    return percentage * 2.56;
}

// Mirrors the first half of the first fin to the second half
void mirrorFirstFin()
{
#if defined(NO_TIP_AND_PIT_LEDS)
    leds(NUM_LEDS_IN_FIN, NUM_LEDS_BETWEEN_TIPS) = leds(0, NUM_LEDS_BETWEEN_TIPS);
#else
    leds(NUM_LEDS_IN_FIN, NUM_LEDS_BETWEEN_TIPS + 2) = leds(1, NUM_LEDS_BETWEEN_TIPS + 1);
#endif
}

// Copies the first fin to all other fins
void copyFirstFinToAllFins()
{
    for (uint16_t fin = 1; fin < NUM_FINS; ++fin)
    {
#if defined(NO_TIP_AND_PIT_LEDS)
        const int finStart = (2 * fin * NUM_LEDS_BETWEEN_TIPS);
        leds(finStart, finStart + NUM_LEDS_IN_FIN) = leds(0, NUM_LEDS_IN_FIN);
#else
        const int finStart = (2 * fin * NUM_LEDS_BETWEEN_TIPS) + (fin * 2);
        leds(finStart, finStart + NUM_LEDS_IN_FIN) = leds(0, NUM_LEDS_IN_FIN);
#endif
    }
}

// Helper function that returns true after the given animationStepTime has passed
bool animate(const uint32_t currentTime, const uint16_t animationStepTime)
{
    if (currentTime - lastAnimationStep > animationStepTime)
    {
        lastAnimationStep = currentTime;
        return true;
    }
    return false;
}

// second Helper function does the same as above but on a second timer
bool animate2(const uint32_t currentTime, const uint16_t animationStepTime)
{
    if (currentTime - lastAnimationStep2 > animationStepTime)
    {
        lastAnimationStep2 = currentTime;
        return true;
    }
    return false;
}

// Do a linear interpolation between the current color of eacg led in ledArray and the given color with the
// given fraction
void lerp8(CPixelView<CRGB>& ledArray, const CHSV& color, const uint8_t fraction)
{
    const CRGB rgbColor = CRGB(color);
    for (int i = 0; i < ledArray.size(); ++i)
    {
        lerp8(ledArray[i], rgbColor, fraction);
    }
}

// Do a linear interpolation between the two given colors and store the result in the first color object
void lerp8(CRGB& color1, const CRGB& color2, const uint8_t fraction)
{
    color1.r = lerp8by8(color1.r, color2.r, fraction);
    color1.g = lerp8by8(color1.g, color2.g, fraction);
    color1.b = lerp8by8(color1.b, color2.b, fraction);
}

// Log the given parameter when logging is enabled
// Ecpected output: "<name>: <value>"
template <typename T>
void logParameter(const String& name, const T& value)
{
    if (DEBUG)
    {
        Serial.print(name);
        Serial.print(": ");
        Serial.println(value);
    }
}
