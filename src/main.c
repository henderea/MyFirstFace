#include <pebble.h>
 
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
  KEY_STEPS = 2,
  KEY_DISTANCE = 3,
  KEY_CHECK_PROGRESS = 4,
  KEY_BATTERY = 5,
  KEY_CONFIG = 6
};
 

static Window *s_main_window;
static TextLayer *s_time_layer_hour;
static TextLayer *s_time_layer_min;
static TextLayer *s_time_layer_sec;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_steps_layer;
static Layer *s_check_progress_layer;
static GFont s_time_font_hour;
static GFont s_time_font_min_sec;
static GFont s_date_font;
static GFont s_weather_font;
static GFont s_battery_font;
static GFont s_steps_font;
static int s_check_progress_width = 0;
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;
static int s_battery_level;
static char steps_buffer[20];
static int held_up = 1;
static int should_show_light = 0;
// static char distance_buffer[20];
// static int showing_steps = 1;

static void check_progress_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, s_check_progress_width, bounds.size.h), 0, GCornerNone);
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void update_battery() {
  static char battery_buffer[5];
  snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", s_battery_level);
  text_layer_set_text(s_battery_layer, battery_buffer);
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  update_battery();
}

// static void show_steps_or_distance() {
//   if(showing_steps) {
//     text_layer_set_text(s_steps_layer, steps_buffer);
//   } else {
//     text_layer_set_text(s_steps_layer, distance_buffer);
//   }
// }

// static void tap_handler(AccelAxisType axis, int32_t direction) {
//   showing_steps = 1 - showing_steps;
//   show_steps_or_distance();
// }

static void data_handler(AccelData *data, uint32_t num_samples) {
  int total_x = 0, total_y = 0, total_z = 0;
  unsigned int i;
  for(i = 0; i < num_samples; i++) {
    total_x += data[i].x;
    total_y += data[i].y;
    total_z += data[i].z;
  }
  float avg_x, avg_y, avg_z;
  avg_x = (float)total_x / (float)num_samples;
  avg_y = (float)total_y / (float)num_samples;
  avg_z = (float)total_z / (float)num_samples;
  if(abs(avg_x) < 500.0F && avg_y < -300.0F && avg_z > -800.0F) {
    held_up = 1;
  } else {
    held_up = 0;
  }
  if(abs(avg_x) < 300.0F && avg_y < -700.0F && avg_z > -300.0F) {
    if(should_show_light == 0) {
      light_enable_interaction();
    }
    should_show_light = 1;
  } else {
    should_show_light = 0;
  }
}

static void main_window_load(Window *window) {
  // HOUR
  // Create time TextLayer
  s_time_layer_hour = text_layer_create(GRect(0, 0, 90, 80));
  text_layer_set_background_color(s_time_layer_hour, GColorClear);
  text_layer_set_text_color(s_time_layer_hour, GColorCyan);
  text_layer_set_text(s_time_layer_hour, "00");

  // Improve the layout to be more like a watchface
  text_layer_set_text_alignment(s_time_layer_hour, GTextAlignmentRight);
  
  // Create GFont
  s_time_font_hour = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LIGHT_FONT_70));
  
  // Apply to TextLayer
  text_layer_set_font(s_time_layer_hour, s_time_font_hour);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer_hour));
  
  // MIN
  // Create time TextLayer
  s_time_layer_min = text_layer_create(GRect(100, 10, 60, 35));
  text_layer_set_background_color(s_time_layer_min, GColorClear);
  text_layer_set_text_color(s_time_layer_min, GColorCyan);
  text_layer_set_text(s_time_layer_min, "00");

  // Improve the layout to be more like a watchface
  text_layer_set_text_alignment(s_time_layer_min, GTextAlignmentLeft);
  
  // Create GFont
  s_time_font_min_sec = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LIGHT_FONT_25));
  
  // Apply to TextLayer
  text_layer_set_font(s_time_layer_min, s_time_font_min_sec);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer_min));
  
  // SEC
  // Create time TextLayer
  s_time_layer_sec = text_layer_create(GRect(100, 45, 60, 35));
  text_layer_set_background_color(s_time_layer_sec, GColorClear);
  text_layer_set_text_color(s_time_layer_sec, GColorWhite);
  text_layer_set_text(s_time_layer_sec, "00");

  // Improve the layout to be more like a watchface
  text_layer_set_text_alignment(s_time_layer_sec, GTextAlignmentLeft);
  
  // Apply to TextLayer
  text_layer_set_font(s_time_layer_sec, s_time_font_min_sec);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer_sec));
  
  // Create date Layer
  s_date_layer = text_layer_create(GRect(0, 90, 144, 20));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text(s_date_layer, "Wed, Sep 17");
  
  // Create second custom font, apply it and add to Window
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LIGHT_FONT_16));
  text_layer_set_font(s_date_layer, s_date_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(GRect(0, 110, 144, 25));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  
  // Create second custom font, apply it and add to Window
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LIGHT_FONT_20));
  text_layer_set_font(s_weather_layer, s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
  // Create battery Layer
  s_battery_layer = text_layer_create(GRect(0, 140, 50, 20));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorGreen);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  text_layer_set_text(s_battery_layer, "100%");
  
  // Create second custom font, apply it and add to Window
  s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LIGHT_FONT_16));
  text_layer_set_font(s_battery_layer, s_battery_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  
  // Create steps Layer
  s_steps_layer = text_layer_create(GRect(50, 142, 90, 20));
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_text_color(s_steps_layer, GColorOrange);
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentRight);
  text_layer_set_text(s_steps_layer, "10000 steps");
  
  // Create second custom font, apply it and add to Window
  s_steps_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LIGHT_FONT_14));
  text_layer_set_font(s_steps_layer, s_steps_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_steps_layer));
  
  // Create battery meter Layer
  s_check_progress_layer = layer_create(GRect(0, 0, 144, 2));
  layer_set_update_proc(s_check_progress_layer, check_progress_update_proc);
  
  // Add to Window
  layer_add_child(window_get_root_layer(window), s_check_progress_layer);
  
  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  
  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(57, 69, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(bluetooth_connection_service_peek());
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer_h[] = "00";
  static char buffer_m[] = "00";
  static char buffer_s[] = "00";
  static char buffer_d[] = "Wed, Sep 17";

  // Write the current hours into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer_h, sizeof("00"), "%H", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer_h, sizeof("00"), "%I", tick_time);
  }
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer_hour, buffer_h);
  
  // Write the current minutes into the buffer
  strftime(buffer_m, sizeof("00"), "%M", tick_time);
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer_min, buffer_m);
  
  if(held_up) {
    // Write the current seconds into the buffer
    strftime(buffer_s, sizeof("00"), "%S", tick_time);
    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer_sec, buffer_s);
    layer_set_hidden(text_layer_get_layer(s_time_layer_sec), 0);
  }
  
  // Write the current seconds into the buffer
  strftime(buffer_d, sizeof("Wed, Sep 16"), "%a, %b %e", tick_time);
  // Display this time on the TextLayer
  text_layer_set_text(s_date_layer, buffer_d);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if(held_up) {
    update_time();
  } else {
    if(tick_time->tm_sec % 60 == 0) {
      update_time();
    }
    layer_set_hidden(text_layer_get_layer(s_time_layer_sec), 1);
  }
  // Get weather update every 30 minutes
  if(tick_time->tm_sec % 60 == 0 && tick_time->tm_min % 5 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, KEY_BATTERY, s_battery_level);
  
    // Send the message!
    app_message_outbox_send();
    
    update_battery();
  }
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer_hour);
  // Unload GFont
  fonts_unload_custom_font(s_time_font_hour);
  // Destroy TextLayer
  text_layer_destroy(s_time_layer_min);
  // Destroy TextLayer
  text_layer_destroy(s_time_layer_sec);
  // Unload GFont
  fonts_unload_custom_font(s_time_font_min_sec);
  // Destroy date elements
  text_layer_destroy(s_date_layer);
  fonts_unload_custom_font(s_date_font);
  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
  // Destroy battery elements
  text_layer_destroy(s_battery_layer);
  fonts_unload_custom_font(s_battery_font);
  // Destroy steps elements
  text_layer_destroy(s_steps_layer);
  fonts_unload_custom_font(s_steps_font);
  // Destroy check progress elements
  layer_destroy(s_check_progress_layer);
  // Destron BT connection elements
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°F", (int)t->value->int32);
        break;
      case KEY_CONDITIONS:
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
        break;
      case KEY_STEPS:
        snprintf(steps_buffer, sizeof(steps_buffer), "%s steps", t->value->cstring);
        break;
//       case KEY_DISTANCE:
//         snprintf(distance_buffer, sizeof(distance_buffer), "%s", t->value->cstring);
//         break;
      case KEY_CHECK_PROGRESS:
        s_check_progress_width = t->value->int32;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
//   show_steps_or_distance();
  text_layer_set_text(s_steps_layer, steps_buffer);
  layer_mark_dirty(s_check_progress_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  window_set_background_color(s_main_window, GColorBlack);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  
  // Register for Bluetooth connection updates
  bluetooth_connection_service_subscribe(bluetooth_callback);
  
//   accel_tap_service_subscribe(tap_handler);
  // Subscribe to the accelerometer data service
  int num_samples = 3;
  accel_data_service_subscribe(num_samples, data_handler);

  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}