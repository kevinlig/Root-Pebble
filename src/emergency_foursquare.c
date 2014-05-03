#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *addr_layer;
Layer *line_layer;

static GBitmap *action_icon_refresh;
static ActionBarLayer *action_bar;

static AppSync sync;
static uint8_t sync_buffer[64];
char displayAddress[] = "";

int increment_count = 0;
float stored_lat;
float stored_long;


void update_location(int forceUpdate) {
  // request JS get the user's location
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "update location");
  // send command to JS
  Tuplet value = TupletInteger(0, forceUpdate);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
  increment_count = 0;
}


static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  update_location(12);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  // watch-phone communication error
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App message sync error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
  if (key == 1) {
      text_layer_set_text(addr_layer, new_tuple->value->cstring);
  }
}




void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {

  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";

  char *time_format;

  if (!tick_time) {
    time_t now = time(NULL);
    tick_time = localtime(&now);
  }


  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_layer, time_text);

  increment_count++;
  if (increment_count >= 30) {
    // check location every 30 seconds
    update_location(10);
  }
}

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);

  // draw the line
  // GRect line_frame = GRect(8, 63, 139, 2);
  // line_layer = layer_create(line_frame);
  // layer_set_update_proc(line_layer, line_layer_update_callback);
  // layer_add_child(window_layer, line_layer);

  // create the action bar
  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, window);
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);

  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, action_icon_refresh);

  // draw the time text layer
  text_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  // draw the address text layer
  addr_layer = text_layer_create(GRect(8, 10, 144-8, 168-68));
  text_layer_set_text_color(addr_layer, GColorWhite);
  text_layer_set_background_color(addr_layer, GColorClear);
  text_layer_set_font(addr_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(addr_layer));


  // start message monitoring
  Tuplet initial_values[] = {
    TupletInteger(0,10),
    TupletCString(1,""),
    TupletCString(2,"")
  };
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, sync_error_callback, NULL);


  // set up the time service
  tick_timer_service_subscribe(SECOND_UNIT, handle_minute_tick);
  handle_minute_tick(NULL, SECOND_UNIT);

  increment_count = 28;

}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {

  action_icon_refresh = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_REFRESH);

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
