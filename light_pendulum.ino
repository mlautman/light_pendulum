/*
 * Board: Esp32 Arduino -> SparkFun ESP32 Thing Plus
 * 
 */

#include <FastLED.h>
#include <DoublyLinkedList.h>


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
#define RIPPLE_FADE_RATE (FRAMES_PER_SECOND/10)
#define GRAVITY -9.81  // Gravity constant, negative for downward acceleration
#define BALL_SIZE 7

CRGB leds[NUM_LEGS][TOTAL_LEDS_LEG_RING];
//CRGB ledsRing[TOTAL_LEDS_LEG_RING]; // Array for the leg + ring

uint frames = 0;
double time_per_animation_s = 5;
double frames_per_animation = time_per_animation_s * FRAMES_PER_SECOND;

struct Ball {
  float position;
  float velocity;
  CRGB color;
};

struct Ripple {
  int start_index;
  float radius;
  float speed;
  CRGB color;
  float fade_rate;

  Ripple(int start, float spd, CRGB clr, float fade) : start_index(start), radius(0), speed(spd), color(clr), fade_rate(fade) {}
};
// DoubleLinkedList<Ripple*> ripples; // Create a DoubleLinkedList of Ripples
DoublyLinkedList<Ripple*> ripples;


// Randomly between 2 and 6 balls per strip
Ball balls[NUM_LEGS][10];
int ballCount[NUM_LEGS];

CRGB random_color() {
    return CHSV(random(0, 256), 255, 255); // Hue varies from 0 to 255, saturation and value are maxed out for bright, vivid colors
}
void fade_leg(uint i, uint fadeVal){
  for (uint j=0; j<NUM_LEDS_LEG; j++)
  {
    leds[i][j].fadeToBlackBy(fadeVal); // Gradually fade the ring with a noticeable decrement.
  }  
}
void fade_legs(uint fadeVal){
  for (uint i=0; i<NUM_LEGS; i++)
  {
    fade_leg(i, fadeVal);
  }
}
void fade_ring(uint8_t fadeValue) {
  for (int i = 0; i < NUM_LEDS_RING; i++) {
    RING_STRIP[i].fadeToBlackBy(fadeValue); // Gradually fade the ring with a noticeable decrement.
  }
}

void clear_leds()
{
  for (uint i = 0; i< NUM_LEGS; i++)
    for (uint j = 0; j<TOTAL_LEDS_LEG_RING; j++)
      leds[i][j] = CRGB::Black;
}

void fill_ring(CRGB color) {
  for (int i = 0; i < NUM_LEDS_RING; i++) {
    RING_STRIP[i] = color;
  }
}

void init_balls()
{
  for (int i = 0; i < NUM_LEGS; i++) {
    ballCount[i] = 10; //random(8, 10); // Random number of balls between 1 and 3
    for (int j = 0; j < ballCount[i]; j++) {
      balls[i][j].position = random(10, NUM_LEDS_LEG); // Start at a random height
      balls[i][j].velocity = random(-200, 00) / 100.0; // Random velocity between -0.50 and 0.50 m/s
      balls[i][j].color = random_color(); // Assign a color or make it random
    }
  }
}
int sectionLength = NUM_LEDS_RING / NUM_LEGS;
uint ballFadeRate = 255. / FRAMES_PER_SECOND * 1;
void draw_balls() {
  // Apply a global fade to the LEDs to simulate a trailing effect
  fade_legs(ballFadeRate);

  // Update and display balls
  for (int i = 0; i < NUM_LEGS; i++) {
    for (int j = 0; j < ballCount[i]; j++) {
      // Physics update
      balls[i][j].velocity += GRAVITY * (1.0 / FRAMES_PER_SECOND);
      balls[i][j].position += balls[i][j].velocity * (1.0 / FRAMES_PER_SECOND);

      if (balls[i][j].position < 0) {
        balls[i][j].position = 0;
        balls[i][j].velocity *= -1; // Invert velocity
        // Serial.println("bouncing");
        int rippleStartIndex = i * sectionLength; // Calculate the ripple start index based on the leg

        auto ripple = new Ripple(rippleStartIndex, abs(balls[i][j].velocity), balls[i][j].color, RIPPLE_FADE_RATE);
        // Serial.println("Made a ripple");
        ripples.insert(ripple);
        // Serial.println("added a ripple");
      }

      // Draw the ball at the new position with fading effect
      int ledPos = (int)balls[i][j].position;
      if (ledPos >= 0 && ledPos < NUM_LEDS_LEG) {
        leds[i][NUM_LEDS_LEG-ledPos] = balls[i][j].color;
      }
    }
  }
}

void cleanupRipples() {
  Serial.println("cleaning up riples");

  int i = 0;
  while (i < ripples.size()) {
    Ripple* ripple = ripples[i];
    delete ripple;  // Free the memory of the ripple
    ripples.removeAt(i);  // Remove ripple from the list without incrementing index
  }
}

void updateAndRenderRipples() {
  // Serial.println("updateAndRenderRipples()");
  for (int i = 0; i < ripples.size(); i++) {
    Ripple* ripple = ripples[i];
    
    // Calculate new positions based on the ripple's speed
    int forwardIndex = int(ripple->start_index + ripple->radius) % NUM_LEDS_RING;
    int backwardIndex = int(ripple->start_index - ripple->radius) % NUM_LEDS_RING;
    if (backwardIndex < 0) 
      backwardIndex += NUM_LEDS_RING; // Wrap around the ring

    // Set the color at the new positions and fade
    RING_STRIP[forwardIndex] = ripple->color;
    RING_STRIP[backwardIndex] = ripple->color;


    // Increase radius for next frame
    // ripple->radius += ripple->speed;
    ripple->radius += ripple->speed / FRAMES_PER_SECOND;


    ripple->color.fadeToBlackBy(ripple->fade_rate);
    // Check if the ripple should be removed
    if (ripple->color.getLuma() < 10) { // Assuming fade_rate and color handling causes it to fade out
      delete ripple; // Free memory
      ripples.removeAt(i--);
    }
  }
  // Fade all LEDs slightly
  uint ringFadeRate = (uint)(255 / FRAMES_PER_SECOND * 0.5);
  fade_ring(ringFadeRate);
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
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(leds[0], TOTAL_LEDS_LEG_RING).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN3, COLOR_ORDER>(leds[1], NUM_LEDS_LEG).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds[2], NUM_LEDS_LEG).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}


void loop() {
if (frames < 1*frames_per_animation)
  {
    if (frames % (uint)frames_per_animation == 0)
    {
      // first balls
      init_balls();
      clear_leds();
    } else if (frames % (uint)frames_per_animation == (uint)frames_per_animation-1)
    {
      // Last balls
      cleanupRipples();
    } 
    else {
      draw_balls();
      updateAndRenderRipples();
    }
  }  
  // else if (frames < 2*frames_per_animation)
  // {
  //   rainbow_bottom();
  // }
  // else if (frames < 3*frames_per_animation)
  // {
  //   fade_ring(FADE_RATE);
  //   fade_legs(FADE_RATE);
  // }
  // else if (frames < 4*frames_per_animation)
  // {
  //   rainbows();
  // }
  else 
  {
    Serial.println("clearing and resetting loop");
    frames = 0;
    clear_leds();
  }
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  frames++;
}
