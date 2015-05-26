#include <pebble.h>

#include "bitmap.h"
#include "Upright.h"

#define TRIG_HALF_MAX_ANGLE 0x8000
#define TRIG_QUARTER_MAX_ANGLE 0x4000
#define TRIG_THREE_QUARTER_MAX_ANGLE 0xC000

static Window *window;
static Layer *layer;
static GBitmap *source;
static GBitmap *digit[11];
static int num[4] = { 0, 0, 0, 0 };

static GPoint pos[4] = { {1, 1}, {73, 1}, {1, 73}, {73, 73} };
AppTimer *timer;

const int IMAGE_RESOURCE_IDS[11] = {
  RESOURCE_ID_IMAGE_T0,
  RESOURCE_ID_IMAGE_T1, RESOURCE_ID_IMAGE_T2, RESOURCE_ID_IMAGE_T3,
  RESOURCE_ID_IMAGE_T4, RESOURCE_ID_IMAGE_T5, RESOURCE_ID_IMAGE_T6,
  RESOURCE_ID_IMAGE_T7, RESOURCE_ID_IMAGE_T8, RESOURCE_ID_IMAGE_T9,
  RESOURCE_ID_IMAGE_T1_2
};

uint16_t squareRoot(uint16_t x) {
  uint16_t a, b;

  b = x;
  a = x = 0x3f;
  x = b/x;
  a = x = (x+a)>>1;
  x = b/x;
  a = x = (x+a)>>1;
  x = b/x;
  x = (x+a)>>1;

  return x;
}

void updateLayer(Layer *layer, GContext *ctx) {
  graphics_draw_bitmap_in_rect(ctx, &bitmap, GRect(0, 0, 144, 168));
}

static void handle_tick(struct tm *now, TimeUnits units_changed) {
  int i;
  int h = now->tm_hour;
  if (!clock_is_24h_style()) {
    h = (h>12)?h-12:((h==0)?12:h);
  }
  
  num[0] = h/10;
  num[1] = h%10;
  num[2] = now->tm_min/10;
  num[3] = now->tm_min%10;

  if (num[0] == 1) num[0] = 10;
  if (num[2] == 1) num[2] = 10;

  for (i=0; i<4; i++) {
    bmpCopyTo(digit[num[i]], source, pos[i]);
  }
}

static void handle_timer(void *data) {
  static int32_t angle = TRIG_MAX_ANGLE/4;
  static AccelData ad;
  int32_t a;

	accel_service_peek(&ad);
  a = atan2_lookup(ad.y, ad.x);

  if ( (angle < TRIG_QUARTER_MAX_ANGLE) && (a > TRIG_THREE_QUARTER_MAX_ANGLE) ) {
    angle = (2*angle + a - TRIG_MAX_ANGLE) / 3;
    if (angle < 0) {
      angle += TRIG_MAX_ANGLE;
    }
  } else if ( (angle > TRIG_THREE_QUARTER_MAX_ANGLE) && (a < TRIG_QUARTER_MAX_ANGLE)) {
    angle = (2*angle + a + TRIG_MAX_ANGLE) / 3;
    if (angle > TRIG_MAX_ANGLE) {
      angle -= TRIG_MAX_ANGLE;
    }
  } else {
    angle = (2*angle + a) / 3;
  }


  bmpFillColor(&bitmap, GColorBlack);
  bmpRotate(source, &bitmap, 270-(angle*360)/TRIG_MAX_ANGLE, NULL, GPoint(72, 72), GPoint(0, 0));
  layer_mark_dirty(layer);
  timer = app_timer_register(50, handle_timer, NULL);
}

static void handleAccel(AccelData *data, uint32_t num_samples) {
	// return;
}

static void init(void) {
  Layer *rootLayer;
  time_t now;
  
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_stack_push(window, true);

  source = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SQUARE);
  for (int i=0; i<11; i++) {
    digit[i] = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS[i]);
  }

  handle_tick(localtime(&now), 0);

  rootLayer = window_get_root_layer(window);
  
  layer = layer_create(layer_get_frame(rootLayer));
  layer_set_update_proc(layer, updateLayer);
  layer_add_child(rootLayer, layer);

  accel_data_service_subscribe(0, handleAccel);

  timer = app_timer_register(0, handle_timer, NULL);
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  app_timer_cancel(timer);
  tick_timer_service_unsubscribe();
  layer_destroy(layer);
  for (int i=0; i<11; i++) {
    gbitmap_destroy(digit[i]);
  }
  gbitmap_destroy(source);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
