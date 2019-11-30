#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

/** 
 *Config 
 **/
const bool DEBUG = false; /// en-/disable debug output

const uint8_t DATA_PIN = 6; /// The pin the leds are connected to
const uint16_t NUM_FINS = 5; // The number of fins the start has
const uint16_t NUM_LEDS_BETWEEN_TIPS = 5; // The number of leds between tips (without tip leds)

// 60000 milliseconds = 1 minute
const uint16_t animationTime = 60000; // The time after which to change animation

const uint8_t maxMode = 6; // The mode count

/**
 * Internal variables 
 **/
const uint16_t NUM_LEDS = (NUM_FINS * 2 + NUM_LEDS_BETWEEN_TIPS * NUM_FINS * 2);
const uint16_t NUM_LEDS_IN_FIN = (2 * NUM_LEDS_BETWEEN_TIPS) + 1;
uint32_t lastAnimationTime = 0; // The last time we changed the animation
uint32_t lastAnimationStep = 0; // The last time we animated a step 1
uint32_t lastAnimationStep2 = 0; // The last time we animated a step 2
uint8_t currentMode = 0; // The current mode/animation we are in
uint8_t hue = 0; // The color of the animation

CRGBArray<NUM_LEDS> leds; // Our leds

void setup()
{
  if(DEBUG)
  {
    Serial.begin(115200); // Enable serial output when debugging
  }
  FastLED.addLeds<NEOPIXEL,DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,200); // Limit power to 2 Watts (5V * 0.2A)
  leds.fill_solid(0x000000); // Clear all leds
  FastLED.show();
}

void loop()
{
  unsigned long currentMillis =  millis();
  if(currentMillis - lastAnimationTime > animationTime) // Change the mode after the given animationTime
  {
    lastAnimationTime = currentMillis;
    ++currentMode;
    if(currentMode > maxMode) // Reset to first mode after last mode
    {
      currentMode = 0;
    }
  }

  logParameter("Time", currentMillis);
  
  switch(currentMode) // Run the animation associated with the mode
  {
    case 0:
      rainbow(currentMillis);
      break;
    case 1:
      randomTwinkle(currentMillis, 0x00B315, 0xB3000C);
      break;
    case 2:
      outsideRainbow(currentMillis);
      break;
    case 3:
      staticColor(CHSV(hue, 255, 255));
      break;
    case 4:
      outsideWoosh(currentMillis);
      break;
    case 5:
      simpleColorFade(currentMillis);
      break;
    case 6:
      staticTwinkle(currentMillis, CHSV(hue, 255, 255));
      break;
    default:
      currentMode = 0;
  }

  LEDS.show(); // Show the animation
}

void rainbow(uint32_t time)
{
  if(animate(time, 30)) // Every 30 ms update the color
  {
    ++hue;
    for(uint8_t i = 0; i < NUM_LEDS; ++i)
    {
      leds[i].setHue((hue-(255/NUM_LEDS)*i) % 256);
    }
  }
}

void randomTwinkle(uint32_t time, uint32_t color1, uint32_t color2)
{
  if(animate(time, 100)) // Every 100 ms fade by 20 and set random leds
  {
    leds.fadeToBlackBy(20);
    leds[random8(0, NUM_LEDS)] = color1;
    leds[random8(0, NUM_LEDS)] = color2;
  }
}

void weirdAnimation(uint32_t time)
{
  if(animate(time, 30)) // Every 30 ms update the color
  {
    ++hue;
    // Set tip and inner tip green
    leds[0] = leds[NUM_FINS + 1] = 0x00B315;

    // Fill rainbow between first tips
    for(uint8_t i = 0; i < NUM_LEDS_BETWEEN_TIPS; ++i)
    {
      leds[i + 1].setHue((hue-(255/NUM_LEDS_BETWEEN_TIPS)*i) % 256);
    }

    mirrorFirstFin();
    copyFirstFinToAllFins();
  }
}

void outsideWoosh(uint32_t time)
{
  static uint8_t animationStep = 0;
  if(animate(time, 100)) // Every 100 ms update the color
  {
    leds.fadeToBlackBy(convertPercent(300 / (NUM_LEDS_BETWEEN_TIPS + 2) ));
    
    if(animationStep < NUM_LEDS_BETWEEN_TIPS + 2) // Don't animate all the time to have time to fade to black
    {
      leds[animationStep] = CHSV(hue,255,255);
      leds(NUM_LEDS_IN_FIN, NUM_LEDS_BETWEEN_TIPS + 2) = leds(1, NUM_LEDS_BETWEEN_TIPS + 1);
      copyFirstFinToAllFins();
      ++hue;
    }

    ++animationStep;
    if(animationStep >= 2 * (NUM_LEDS_BETWEEN_TIPS + 2))
    {
      animationStep = 0;
    }
  }
}

void outsideRainbow(uint32_t time)
{
  if(animate(time, 30)) // Every 30 ms update the color
  {
    ++hue;
    // Fill rainbow between first tips
    for(uint8_t i = 0; i < NUM_LEDS_BETWEEN_TIPS+2; ++i)
    {
      leds[i].setHue((hue-(255/(NUM_LEDS_IN_FIN))*i) % 256);
    }

    mirrorFirstFin();
    copyFirstFinToAllFins();
  }
}

void simpleColorFade(uint32_t time)
{
  if(animate(time, 30)) // Every 30 ms update the color
  {
    ++hue;
    staticColor(CHSV(hue, 255, 255));
  }
}

void staticColor(const CHSV & color)
{
  leds.fill_solid(color);
}

void staticTwinkle(uint32_t time, const CHSV & color)
{
  if(animate2(time, 50))  // Every 50 ms fade to black by 12.5 %
  {
    lerp8(leds, color, convertPercent(12.5)); // 12.5%
  }
  if(animate(time, 150))
  {
    leds[random8(0, NUM_LEDS)] = CRGB::White;
  }
}

// Converts a given percentage to the FastLED percentage
uint8_t convertPercent(const double percentage)
{
  return percentage * 2.56;
}

// Mirrors the first half of the first fin to the second half
void mirrorFirstFin()
{
  leds(NUM_LEDS_IN_FIN, NUM_LEDS_BETWEEN_TIPS + 2) = leds(1, NUM_LEDS_BETWEEN_TIPS + 1);
}

// Copies the first fin to all other fins
void copyFirstFinToAllFins()
{
  for(uint16_t fin = 1; fin < NUM_FINS; ++fin)
  {
    const int finStart = (2 * fin * NUM_LEDS_BETWEEN_TIPS) + (fin * 2);
    leds(finStart, finStart + NUM_LEDS_IN_FIN) = leds(0, NUM_LEDS_IN_FIN);
  }
}

// Helper function that returns true after the given animationStepTime has passed
bool animate(const uint32_t currentTime, const uint16_t animationStepTime)
{
  if(currentTime - lastAnimationStep > animationStepTime)
  {
    lastAnimationStep = currentTime;
    return true;
  }
  return false;
}

// second Helper function does the same as above but on a second timer
bool animate2(const uint32_t currentTime, const uint16_t animationStepTime)
{
  if(currentTime - lastAnimationStep2 > animationStepTime)
  {
    lastAnimationStep2 = currentTime;
    return true;
  }
  return false;
}

// Do a linear interpolation between the current color of eacg led in ledArray and the given color with the given fraction
void lerp8(CPixelView<CRGB> & ledArray, const CHSV & color, const uint8_t fraction)
{
  CRGB rgbColor = CRGB(color);
  for(int i=0; i< ledArray.size(); ++i)
  {
    lerp8(ledArray[i], rgbColor, fraction);
  }
}

// Do a linear interpolation between the two given colors and store the result in the first color object
void lerp8(CRGB & color1, const CRGB & color2, const uint8_t fraction)
{
  color1.r = lerp8by8(color1.r, color2.r, fraction);
  color1.g = lerp8by8(color1.g, color2.g, fraction);
  color1.b = lerp8by8(color1.b, color2.b, fraction);
}

// Log the given parameter when logging is enabled
// Ecpected output: "<name>: <value>"
template<typename T>
void logParameter(String name, T value)
{
  if(DEBUG)
  {
    Serial.print(name);
    Serial.print(": ");
    Serial.println(value);
  }
}
