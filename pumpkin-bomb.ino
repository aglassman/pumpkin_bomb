// A basic everyday NeoPixel strip test program.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>
#include <Bounce2.h> 
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    6

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 12

#define RINGTIMER 50;

struct AnimationState {
  bool idle;
  // The duration of the animation
  float duration;
  // The start time of the animation
  unsigned long start;
  // The end_time of the animation (start_time + duration_ms)
  unsigned long end;
  // The elapsed milliseconds of the animation.
  unsigned long elapsed_ms;
  // Normalized completion of animation (0,1)
  float elapsed_n;
};

// Animation Setup
enum animation_mode { A_IDLE, A_EXPLODE_1 };
int animation_mode_count = 2;
int current_animation_mode = A_IDLE;

struct AnimationState animation_state = {true, 0, 0, 0, 0, 0};

unsigned long last_print = 0;

// Declare our NeoPixel strip object:
Adafruit_NeoPixel top_ring = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

// Chase Ring
int out_pins[]={21,20,19,18,17,16,15,14};
unsigned long chase_step = 0;
unsigned long  chase_timer = 0;

#define BUTTON_PIN 9
Bounce2::Button button = Bounce2::Button();

// LED for chase ring 13-21

// setup() function -- runs once at startup --------------------------------

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  Serial.begin(9600);
  Serial.println("Hello world");

  button.attach ( BUTTON_PIN , INPUT_PULLUP );
  button.interval( 5 );
  button.setPressedState( LOW ); 

  top_ring.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  top_ring.show();            // Turn OFF all pixels ASAP
  top_ring.setBrightness(200); // Set BRIGHTNESS to about 1/5 (max = 255)

  for(int i = 14; i < 22; i++) {
    pinMode(i, OUTPUT);
  }
}

void loop() {
  unsigned long time = millis();
  button.update();

  if(button.pressed()) {
    cycleAnimation(time);
  }

  animation_update(time);

  if (!animation_state.idle) {
    animate(time);
  }

  if (time > (last_print + 500)) {
    print_animation_state();
    last_print = time;
  }

}

void cycleAnimation(unsigned long time) {
  Serial.println(button.pressed());
  Serial.println(current_animation_mode);
  animation_halt();
  if(++current_animation_mode >= animation_mode_count) {
    current_animation_mode = 0;
  }

  switch (current_animation_mode) {
    case A_IDLE:
      break;

    case A_EXPLODE_1:
      animation_init(time, 6000);
      break;
  }
}

bool animation_update(unsigned long time) {
  float elapsed_ms = time - animation_state.start;
  float elapsed_n = elapsed_ms / animation_state.duration;  
  if (elapsed_ms >= animation_state.duration) {
    animation_state.elapsed_n = 1;
    animation_state.elapsed_ms = animation_state.duration;
    animation_halt();
  } else {
    animation_state.elapsed_n = elapsed_n;
    animation_state.elapsed_ms = elapsed_ms;
  }
}

void print_animation_state() {
  Serial.println("AnimationState: idle: ");
  Serial.print(animation_state.idle);
  Serial.print(" duration: ");
  Serial.print(animation_state.duration);
  Serial.print(" start: ");
  Serial.print(animation_state.start);
  Serial.print(" end: ");
  Serial.print(animation_state.end);
  Serial.print(" elapsed_n: ");
  Serial.print(animation_state.elapsed_n);
  Serial.print(" elapsed_ms: ");
  Serial.print(animation_state.elapsed_ms);
  Serial.println("");
}

void animation_init(unsigned long time, unsigned long duration) {
  animation_state.idle = false;
  animation_state.duration = duration;
  animation_state.start = time;
  animation_state.end = time + duration;
  animation_state.elapsed_n = 0;
  animation_state.elapsed_ms = 0;
}

void animation_reset() {
  animation_state.idle = true;
  animation_state.duration = 0;
  animation_state.start = 0;
  animation_state.end = 0;
  animation_state.elapsed_n = 0;
  animation_state.elapsed_ms = 0;
}

void animation_halt() {
  chase_halt();
  ring_halt(&top_ring);
  animation_reset();
}

void animate(unsigned long time) {
  switch (current_animation_mode) {
    case A_IDLE:
      break;

    case A_EXPLODE_1:
      int blink_rate = 1000 - (250.0 * (animation_state.elapsed_n * 3));
      int chase_rate = 100 - (99 * (animation_state.elapsed_n * 1.2));
      if(animation_state.elapsed_ms < 4900) {
        ring_blink(&top_ring, time, blink_rate);
      } else {
        ring_solid(&top_ring);
      }

      if(animation_state.elapsed_ms < 4900) {
        chase_spin(time, chase_rate);
      } else {
        chase_solid();
      }

      break;
  }
}

void ring_blink(Adafruit_NeoPixel* ring, unsigned long time, int speed) {
  if((time % speed) <= (speed / 2)) {
    uint32_t color = ring->Color(0, 255,0);
    for(int i=0; i<ring->numPixels(); i++) {
      ring->setPixelColor(i, color);
    }
  } else {
    ring->clear();
  }
  ring->show();
}

void ring_solid(Adafruit_NeoPixel* ring) {
  uint32_t color = ring->Color(0, 255,0);
  for(int i=0; i < ring->numPixels(); i++) {
    ring->setPixelColor(i, color);
  }
  ring->show();
}

void ring_halt(Adafruit_NeoPixel* ring) {
  ring->clear();
  ring->show();
}

void chase_spin(unsigned long time, int speed_ms) {
  if (time > chase_timer) {
    chase_timer = time + speed_ms;
    chase_step++;
    for(int i = 0; i < 8; i++) {
      digitalWrite(out_pins[i], ((chase_step + i) % 8) <= 1);
    }
  }
}

void chase_solid() {
  for(int i = 0; i < 8; i++) {
    digitalWrite(out_pins[i], HIGH);
  }
}

void chase_halt() {
  for(int i = 0; i < 8; i++) {
    digitalWrite(out_pins[i], LOW);
  }
  chase_step = 0;
  chase_timer = 0;
}
