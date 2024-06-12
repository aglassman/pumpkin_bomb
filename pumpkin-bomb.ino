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
const int RING_LED_COUNT = 12;

const int TOP_RING_PIN = 5;
const int BOTTOM_RING_PIN = 6;

// How many NeoPixels are attached to the Arduino?

struct AnimationState {
  bool idle;
  int animation;
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

// Program State Setup
enum program_mode { IDLE, ANIMATE, SETTINGS };
program_mode current_program_mode = IDLE;

unsigned long button_hold_start_time = 0;

enum animations {A_EXPLODE_1};
int animation_count = 1;


struct AnimationState animation_state = {true, A_EXPLODE_1, 0, 0, 0, 0, 0};

unsigned long last_debug_print = 0;

int ring_brightness = 10;

uint32_t GREEN = Adafruit_NeoPixel::Color(0, 255,0);
uint32_t ORANGE = Adafruit_NeoPixel::Color(255, 55,0);

// Declare our NeoPixel strip object:
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

enum ring_state {RING_OFF, RING_SOLID};

struct RingState {
  Adafruit_NeoPixel ring;
  ring_state state;
  unsigned long last_state_change;
};

struct RingState top = {Adafruit_NeoPixel(RING_LED_COUNT, TOP_RING_PIN, NEO_GRB + NEO_KHZ800), RING_OFF, 0};
struct RingState bottom = {Adafruit_NeoPixel(RING_LED_COUNT, BOTTOM_RING_PIN, NEO_GRB + NEO_KHZ800), RING_OFF, 0};

// Chase Ring
enum chase_state {CHASE_OFF, CHASE_ON};

struct ChaseState {
  int led_count;
  unsigned long chase_step;
  unsigned long chase_timer;
  chase_state state;
  int pins[];
};

struct ChaseState chase = {8, 0, 0, CHASE_OFF, {21,20,19,18,17,16,15,14}};

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

  top.ring.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  top.ring.show();            // Turn OFF all pixels ASAP
  top.ring.setBrightness(ring_brightness); // Set BRIGHTNESS to about 1/5 (max = 255)

  bottom.ring.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  bottom.ring.show();            // Turn OFF all pixels ASAP
  bottom.ring.setBrightness(ring_brightness); 

  chase_init(&chase);
}

void loop() {
  unsigned long time = millis() + 100000;
  button.update();

  //Serial.println(button.getPressedState());

  if(button.pressed()) {
    button_hold_start_time = time;
  }

  if(button.released()) {
    int delta_hold_time = time - button_hold_start_time;
    button_hold_start_time = 0;

    switch (current_program_mode) {
      case IDLE: {
        if (delta_hold_time <= 3000) {
          animation_init(time, 3000 + (delta_hold_time * 2));
        } else {
          settings_init(time);
        }
      } break;

      case SETTINGS: {
        if (delta_hold_time > 3000) {
          idle_init(time);
        } else {
          ring_brightness += 10;
          if (ring_brightness >= 200) {
            ring_brightness = 10;
          }
          Serial.println("Ring Brightness: ");
          Serial.print(ring_brightness);
          Serial.println("");
          top.ring.setBrightness(ring_brightness); 
          bottom.ring.setBrightness(ring_brightness);
          top.ring.show();
          bottom.ring.show();
        }
      }  break;
      
      case ANIMATE: {
      }  break;      
    }
  }

  animation_update(time);
  animate(time);

  if (time > last_debug_print + 1000) {
    debug_state();
    last_debug_print = time;
  }

}

void debug_state() {
  Serial.println("------------------");
  Serial.println("Program Mode: ");
  Serial.print(current_program_mode);
  Serial.println("");
  Serial.print("AnimationState: idle: ");
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

void idle_init(unsigned long time) {
  current_program_mode = IDLE;
  chase_off(&chase);
  ring_off(&top, time);
  ring_off(&bottom, time);                       
}

void settings_init(unsigned long time) {
  current_program_mode = SETTINGS;
  chase_off(&chase);
  ring_solid(&top, time, ORANGE);
  ring_solid(&bottom, time, ORANGE);
}

void cycleAnimation(unsigned long time) {
  animation_halt(time);
  if(++animation_state.animation >= animation_count) {
    animation_state.animation = 0;
  }
}

void animation_update(unsigned long time) {
  if (!animation_state.idle) {
    float elapsed_ms = time - animation_state.start;
    float elapsed_n = elapsed_ms / animation_state.duration;  
    if (elapsed_ms >= animation_state.duration) {
      animation_state.elapsed_n = 1;
      animation_state.elapsed_ms = animation_state.duration;
      animation_halt(time);
    } else {
      animation_state.elapsed_n = elapsed_n;
      animation_state.elapsed_ms = elapsed_ms;
    }
  }
}

void animation_init(unsigned long time, unsigned long duration) {
  current_program_mode = ANIMATE;
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

void animation_halt(unsigned long time) {
  chase_off(&chase);
  ring_off(&top, time);
  ring_off(&bottom, time);
  animation_reset();
  current_program_mode = IDLE;
}

void animate(unsigned long time) {
  switch (current_program_mode) {
    case IDLE: {
      int delta_hold_time = time - button_hold_start_time;
      if (button_hold_start_time == 0) {
        chase_off(&chase);
      } else if (delta_hold_time < 3000) {
        chase_single(&chase, ((time - button_hold_start_time) / 500) % 8);
      } else {
        ring_blink(&top, time, 200, ORANGE);
      }
      
    } break;

    case SETTINGS: {
      int delta_hold_time = time - button_hold_start_time;
      if (button_hold_start_time == 0) {
        // nothing for now
      } else if (delta_hold_time > 3000) {
         ring_blink(&top, time, 200, ORANGE);
      }
    } break;

    case ANIMATE: {
      unsigned long blink_rate = 100 - (50 * animation_state.elapsed_n);
      int chase_rate = 100 - (99 * (animation_state.elapsed_n * 1.2));
      if(animation_state.elapsed_n < 1) {
        ring_blink(&top, time, blink_rate, GREEN);
        ring_blink(&bottom, time, blink_rate, GREEN);
      } else {
        ring_solid(&top, time, GREEN);
        ring_solid(&bottom, time, GREEN);
      }

      if(animation_state.elapsed_n < 0.8) {
        chase_spin(&chase, time, chase_rate);
      } else {
        chase_solid(&chase);
      }
    } break;
  }
}

void ring_blink(RingState* ring_state, unsigned long time, unsigned long blink_rate, uint32_t color) {
  if (time >= (ring_state->last_state_change + blink_rate)) {
    if(ring_state->state == RING_OFF) {
      ring_solid(ring_state, time, color);
    } else if (ring_state->state == RING_SOLID) {
      ring_off(ring_state, time);
    }
  }
}

void ring_solid(RingState* ring_state, unsigned long time, uint32_t color) {
  for(int i=0; i < ring_state->ring.numPixels(); i++) {
    ring_state->ring.setPixelColor(i, color);
  }
  ring_state->ring.show();
  ring_state->state = RING_SOLID;
  ring_state->last_state_change = time;
}

void ring_off(RingState* ring_state, unsigned long time) {
  ring_state->ring.clear();
  ring_state->ring.show();
  ring_state->state = RING_OFF;
  ring_state->last_state_change = time;
}

void chase_spin(ChaseState* chase, unsigned long time, int speed_ms) {
  if (time > chase->chase_timer) {
    chase->chase_timer = time + speed_ms;
    chase->chase_step++;
    for(int i = 0; i < 8; i++) {
      digitalWrite(chase->pins[i], ((chase->chase_step + i) % chase->led_count) <= 1);
    }
  }
  chase->state = CHASE_ON;
}

void chase_init(ChaseState* chase) {
  for(int i = 0; i < chase->led_count; i++) {
    pinMode(chase->pins[i], OUTPUT);
  }
}

void chase_single(ChaseState* chase, int x) {
  for(int i = 0; i < chase->led_count; i++) {
    digitalWrite(chase->pins[i], x == i);
  }
  chase->state = CHASE_ON;
}

void chase_solid(ChaseState* chase) {
  for(int i = 0; i < chase->led_count; i++) {
    digitalWrite(chase->pins[i], HIGH);
  }
  chase->state = CHASE_ON;
}

void chase_off(ChaseState* chase) {
  for(int i = 0; i < chase->led_count; i++) {
    digitalWrite(chase->pins[i], LOW);
  }
  chase->chase_step = 0;
  chase->chase_timer = 0;
  chase->state = CHASE_OFF;
}
