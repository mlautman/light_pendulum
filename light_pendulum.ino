/*
 * Board: Esp32 Arduino -> SparkFun ESP32 Thing Plus
 * 
 */

#include <FastLED.h>


#define NUM_LEDS_LEG 75
#define NUM_LEDS_RING 151
#define TOTAL_LEDS_LEG_RING (NUM_LEDS_LEG + NUM_LEDS_RING)
#define NUM_LEGS 3
#define DATA_PIN1 TX1 // 
#define DATA_PIN2 MISO // 
#define DATA_PIN3 MOSI// Sketchy bug fix
#define CLOCK_PIN 5  // SPI Clock
#define BRIGHTNESS 100
#define FRAMES_PER_SECOND 60
#define LED_TYPE WS2811
#define COLOR_ORDER BGR
#define RING_STRIP (&leds[0][NUM_LEDS_LEG])
#define FADE_RATE (FRAMES_PER_SECOND * 3) 
#define GRAVITY -9.81  // Gravity constant, negative for downward acceleration
#define BALL_SIZE 7

CRGB leds[NUM_LEGS][TOTAL_LEDS_LEG_RING];
//CRGB ledsRing[TOTAL_LEDS_LEG_RING]; // Array for the leg + ring


struct Ball {
  float position;
  float velocity;
  CRGB color;
};

// Randomly between 2 and 6 balls per strip
Ball balls[NUM_LEGS][6];
int ballCount[NUM_LEGS];

CRGB random_color() {
    return CHSV(random(0, 256), 255, 255); // Hue varies from 0 to 255, saturation and value are maxed out for bright, vivid colors
}
void fade_legs(uint fade_rate){
  for (uint i=0; i<NUM_LEGS; i++)
  {
    for (uint j=0; j<NUM_LEDS_LEG; j++)
    {
      leds[i][j].fadeToBlackBy(255 / fade_rate); // Gradually fade the ring     
    }
  }
}
void fade_ring(uint fade_rate) {
  for (int i = 0; i < NUM_LEDS_RING; i++) {
    RING_STRIP[i].fadeToBlackBy(255 / fade_rate); // Gradually fade the ring
  }
}
void clear_leds()
{
  for (uint i = 0; i< NUM_LEGS; i++)
    for (uint j = 0; j<TOTAL_LEDS_LEG_RING; j++)
      leds[i][j] = CRGB::Black;
}

void splash_ring(CRGB color) {
  for (int i = 0; i < NUM_LEDS_RING; i++) {
    RING_STRIP[i] = color;
  }
}

void init_balls()
{
  for (int i = 0; i < NUM_LEGS; i++) {
    ballCount[i] = random(1, 8); // Random number of balls between 1 and 3
    for (int j = 0; j < ballCount[i]; j++) {
      balls[i][j].position = random(30, NUM_LEDS_LEG - BALL_SIZE); // Start at a random height
      balls[i][j].velocity = 0; // Initially, balls are not moving
      balls[i][j].color = random_color(); // Assign a color or make it random
    }
  }
}

void draw_balls()
{
  // Update and display balls
  for (int i = 0; i < NUM_LEGS; i++) {
    // Clear the strip
    fill_solid(leds[i], NUM_LEDS_LEG, CRGB::Black);

    for (int j = 0; j < ballCount[i]; j++) {
      // Physics update
      balls[i][j].velocity += GRAVITY * (1.0 / FRAMES_PER_SECOND);
      balls[i][j].position += balls[i][j].velocity * (1.0 / FRAMES_PER_SECOND);

      // Bounce check
      if (balls[i][j].position < 0) {
        balls[i][j].position = 0; // Reset position to the start of the strip
        balls[i][j].velocity *= -1; //-0.8; // Invert velocity and reduce it by 20%
        splash_ring(balls[i][j].color); 
      }


      // Draw the ball
      // Draw the ball with a rounded effect and minimum brightness at the ends
      int ledPos = (int)balls[i][j].position;
      for (int k = max(0, ledPos - BALL_SIZE / 2); k <= min(NUM_LEDS_LEG - 1, ledPos + BALL_SIZE / 2); k++) {
        // Calculate distance from the center of the ball to the current LED
        float distanceFromCenter = abs(ledPos - k);
        float brightnessFactor = cos((distanceFromCenter / (BALL_SIZE / 2)) * PI / 2); // Normalize and apply cosine
        float minimumBrightness = 0.1; // Adjust to set the minimum brightness (0 for none, 1 for full)
        brightnessFactor = (brightnessFactor * (1 - minimumBrightness)) + minimumBrightness;
        leds[i][NUM_LEDS_LEG - 1 - k] = balls[i][j].color;
        leds[i][NUM_LEDS_LEG - 1 - k].fadeToBlackBy(255 * (1 - brightnessFactor)); // Adjust brightness
      }
      
//      int ledPos = (int)balls[i][j].position;
//      for (int k = max(0, ledPos - BALL_SIZE / 2); k <= min(NUM_LEDS_LEG - 1, ledPos + BALL_SIZE / 2); k++) {
//        // Calculate distance from the center of the ball to the current LED
//        float distanceFromCenter = abs(ledPos - k);
//        float brightnessFactor = cos((distanceFromCenter / (BALL_SIZE / 2)) * PI / 2); // Normalize and apply cosine
//        leds[i][NUM_LEDS_LEG - 1 - k] = balls[i][j].color;
//        leds[i][NUM_LEDS_LEG - 1 - k].fadeToBlackBy(255 * (1 - brightnessFactor)); // Adjust brightness based on the cosine factor
//      }
    }
  }
  fade_ring(FADE_RATE/3);
}

void rainbow_bottom()
{
  CRGB ring_rainbow[NUM_LEDS_RING];
  fill_rainbow(ring_rainbow, NUM_LEDS_RING, millis() / 5, 10);  // Rainbow for the ring
  
  for (uint i=0; i<NUM_LEGS; i++)
  {
    for (uint j=0; j<NUM_LEDS_LEG; j++)
    {
      leds[i][j] = ring_rainbow[i * (uint)(NUM_LEDS_RING / NUM_LEGS)];
    }
  }
  fill_rainbow(RING_STRIP, NUM_LEDS_RING, millis() / 5, 10); 
}

void rainbows(){
  // Update animation for each strip
  for (uint i=0; i<NUM_LEGS; i++)
  {
    fill_rainbow(leds[i], NUM_LEDS_LEG, millis() / 10, 10);
  }
  
  // Separate animation for the combined leg and ring
  fill_rainbow(RING_STRIP, NUM_LEDS_RING, millis() / 10, 10); // Rainbow for the ring
}


void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds[0], TOTAL_LEDS_LEG_RING).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds[1], NUM_LEDS_LEG).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN3, COLOR_ORDER>(leds[2], NUM_LEDS_LEG).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}


uint frames = 0;
double time_per_animation_s = 30;
double frames_per_animation = time_per_animation_s * FRAMES_PER_SECOND;
void loop() {
if (frames < 1*frames_per_animation)
  {
    if (frames % (uint)frames_per_animation == 0)
    {
      init_balls();
      clear_leds();
    }
    draw_balls();
  }  
  else if (frames < 2*frames_per_animation)
  {
    rainbow_bottom();
  }
  else if (frames < 3*frames_per_animation)
  {
    fade_ring(FADE_RATE);
    fade_legs(FADE_RATE);
  }
  else if (frames < 4*frames_per_animation)
  {
    rainbows();
  }
  else 
  {
    frames = 0;
  }
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  frames++;
}
