#include "pebble.h"

// POST variables

// Received variables
enum {
	BTC_KEY_CURRENCY = 1,
	BTC_KEY_EXCHANGE = 2,
	BTC_KEY_ASK = 3,
	BTC_KEY_BID = 4,
	BTC_KEY_LAST = 5,
	INVERT_COLOR_KEY = 6
};
	

static Window *window;
static AppTimer *timer;
static AppSync sync;
static uint8_t sync_buffer[124];

static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static TextLayer *text_price_layer;
static TextLayer *text_currency_layer;
static TextLayer *text_buy_label_layer;
static TextLayer *text_buy_price_layer;
static TextLayer *text_sell_label_layer;
static TextLayer *text_sell_price_layer;

static GFont font_last_price_small;
static GFont font_last_price_large;

static bool using_smaller_font = false;

static void set_timer();

//void failed(int32_t cookie, int http_status, void* context) {
//	if(cookie == 0 || cookie == BTC_HTTP_COOKIE) {
//		text_layer_set_text(&text_price_layer, "---");
//	}
//}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d, %d", app_message_error, dict_error);
}

static void set_invert_color(bool invert) {
	if (invert) {
    window_set_background_color(window, GColorWhite);
    text_layer_set_text_color(text_date_layer, GColorBlack);
    text_layer_set_text_color(text_time_layer, GColorBlack);
    text_layer_set_text_color(text_price_layer, GColorBlack);
    text_layer_set_text_color(text_currency_layer, GColorBlack);
    text_layer_set_text_color(text_buy_label_layer, GColorBlack);
    text_layer_set_text_color(text_buy_price_layer, GColorBlack);
    text_layer_set_text_color(text_sell_label_layer, GColorBlack);
    text_layer_set_text_color(text_sell_price_layer, GColorBlack);
	} else {
    window_set_background_color(window, GColorBlack);
    text_layer_set_text_color(text_date_layer, GColorWhite);
    text_layer_set_text_color(text_time_layer, GColorWhite);
    text_layer_set_text_color(text_price_layer, GColorWhite);
    text_layer_set_text_color(text_currency_layer, GColorWhite);
    text_layer_set_text_color(text_buy_label_layer, GColorWhite);
    text_layer_set_text_color(text_buy_price_layer, GColorWhite);
    text_layer_set_text_color(text_sell_label_layer, GColorWhite);
    text_layer_set_text_color(text_sell_price_layer, GColorWhite);
	}
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	
	static char str[35] = "";
	bool invert;
	switch (key) {
		case BTC_KEY_LAST:
			text_layer_set_text(text_price_layer, new_tuple->value->cstring);
			size_t len = strlen(new_tuple->value->cstring);
			if (len > 6 && !using_smaller_font) {
				text_layer_set_font(text_price_layer, font_last_price_small);
				using_smaller_font = true;
			} else if (len <= 6 && using_smaller_font) {
				text_layer_set_font(text_price_layer, font_last_price_large);
				using_smaller_font = false;
			}
			break;
		case BTC_KEY_BID:
			text_layer_set_text(text_buy_price_layer, new_tuple->value->cstring);
			break;
		case BTC_KEY_ASK:
			text_layer_set_text(text_sell_price_layer, new_tuple->value->cstring);
			break;
		case BTC_KEY_CURRENCY:
			snprintf(str, sizeof(str), "%s / BTC", new_tuple->value->cstring);
			text_layer_set_text(text_currency_layer, str);
			break;
		case INVERT_COLOR_KEY:
			invert = new_tuple->value->uint8 != 0;
			persist_write_bool(INVERT_COLOR_KEY, invert);
			set_invert_color(invert);
			break;
	}
}

static void send_cmd(void) {
	Tuplet value = TupletInteger(0, 1);
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &value);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

static void timer_callback(void *data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer tick");
	send_cmd();
	set_timer();
}

static void set_timer() {
	// Update again in 30 minutes.
	const uint32_t timeout_ms = 30 * 60 * 1000;
	timer = app_timer_register(timeout_ms, timer_callback, NULL);
}

static void update_time(struct tm *tick_time) {
	// Need to be static because they're used by the system later.
	static char time_text[] = "00:00";
	static char date_text[] = "Xxxxxxxxx 00";
	
	char *time_format;
	
	
	// TODO: Only update the date when it's changed.
	strftime(date_text, sizeof(date_text), "%B %e", tick_time);
	text_layer_set_text(text_date_layer, date_text);
	
	
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
	
	text_layer_set_text(text_time_layer, time_text);				
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	update_time(tick_time);
}

static void create_text_layer(TextLayer** text_layer, Layer* window_layer, GRect bounds, GColor color, uint32_t font_id) {
  *text_layer = text_layer_create(bounds);
	text_layer_set_text_color(*text_layer, color);
	text_layer_set_background_color(*text_layer, GColorClear);
	text_layer_set_font(*text_layer, fonts_load_custom_font(resource_get_handle(font_id)));
	layer_add_child(window_layer, text_layer_get_layer(*text_layer));
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	font_last_price_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_MEDIUM_29));
	font_last_price_large = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_MEDIUM_39));
	
  GRect text_bounds;
  
  // Price
  #ifdef PBL_RECT
  text_bounds = GRect(0, 0, bounds.size.w, 40);
  #elif PBL_ROUND
  text_bounds = GRect(0, 18, bounds.size.w, 40);
  #endif
  create_text_layer(&text_price_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_MEDIUM_39);
	text_layer_set_text_alignment(text_price_layer, GTextAlignmentCenter);
	
  // Currency
  #ifdef PBL_RECT
  text_bounds = GRect(8, 41, bounds.size.w, 18);
  #elif PBL_ROUND
  text_bounds = GRect(0, 7, bounds.size.w, 18);
  #endif
  create_text_layer(&text_currency_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_15);
  #ifdef PBL_ROUND
  text_layer_set_text_alignment(text_currency_layer, GTextAlignmentCenter);
  #endif
  
  // Buy label
  #ifdef PBL_RECT
  text_bounds = GRect(8, 60, bounds.size.w, 18);
  #elif PBL_ROUND
  text_bounds = GRect(0, 60, bounds.size.w / 2, 18);
  #endif
  create_text_layer(&text_buy_label_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_15);
  text_layer_set_text(text_buy_label_layer, "BUY");
  #if PBL_ROUND
  text_layer_set_text_alignment(text_buy_label_layer, GTextAlignmentCenter);
  #endif	
	
  // Buy price
  #ifdef PBL_RECT
  text_bounds = GRect(56, 60, bounds.size.w, 18);
  #elif PBL_ROUND
  text_bounds = GRect(0, 78, bounds.size.w / 2, 18);
  #endif
  create_text_layer(&text_buy_price_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_15);
  #if PBL_ROUND
  text_layer_set_text_alignment(text_buy_price_layer, GTextAlignmentCenter);
  #endif	
  
  // Sell label
  #ifdef PBL_RECT
  text_bounds = GRect(8, 78, bounds.size.w, 18);
  #elif PBL_ROUND
  text_bounds = GRect(bounds.size.w / 2, 60, bounds.size.w / 2, 18);
  #endif
  create_text_layer(&text_sell_label_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_15);
	text_layer_set_text(text_sell_label_layer, "SELL");
  #if PBL_ROUND
  text_layer_set_text_alignment(text_sell_label_layer, GTextAlignmentCenter);
  #endif	
	
  // Sell price
  #ifdef PBL_RECT
  text_bounds = GRect(56, 78, bounds.size.w, 18);
  #elif PBL_ROUND
  text_bounds = GRect(bounds.size.w / 2, 78, bounds.size.w / 2, 18);
  #endif
  create_text_layer(&text_sell_price_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_15);
  #if PBL_ROUND
  text_layer_set_text_alignment(text_sell_price_layer, GTextAlignmentCenter);
  #endif	

  // Date
  #ifdef PBL_RECT
  text_bounds = GRect(8, 96, bounds.size.w, 25);
  #elif PBL_ROUND
  text_bounds = GRect(0, 135, bounds.size.w, 25);
  #endif
  create_text_layer(&text_date_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_MEDIUM_19);
  #if PBL_ROUND
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
  #endif	
	
  
  // Time
  #ifdef PBL_RECT
  text_bounds = GRect(7, 114, bounds.size.w, 45);
  #elif PBL_ROUND
  text_bounds = GRect(0, 90, bounds.size.w, 45);
  #endif
  create_text_layer(&text_time_layer, window_layer, text_bounds,
                    GColorWhite, RESOURCE_ID_FONT_DIAVLO_HEAVY_44);
  #if PBL_ROUND
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  #endif	
  
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
	
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	update_time(t);
	
	Tuplet initial_values[] = {
		TupletCString(BTC_KEY_CURRENCY, "---"),
		TupletCString(BTC_KEY_ASK, "---"),
		TupletCString(BTC_KEY_BID, "---"),
		TupletCString(BTC_KEY_LAST, "---"),
		TupletCString(BTC_KEY_EXCHANGE, "---"),
		TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
	};
	
	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
				  sync_tuple_changed_callback, sync_error_callback, NULL);
	
	//send_cmd();
	timer = app_timer_register(2000, timer_callback, NULL);
	//set_timer();
}


static void window_unload(Window *window) {
	app_sync_deinit(&sync);
	
	tick_timer_service_unsubscribe();
	
	text_layer_destroy(text_date_layer);
	text_layer_destroy(text_time_layer);
	text_layer_destroy(text_price_layer);
	text_layer_destroy(text_currency_layer);
	text_layer_destroy(text_buy_label_layer);
	text_layer_destroy(text_buy_price_layer);
	text_layer_destroy(text_sell_label_layer);
	text_layer_destroy(text_sell_price_layer);
}

static void init(void) {
	window = window_create();
	window_set_background_color(window, GColorBlack);
	
	#ifdef PBL_SDK_2
	window_set_fullscreen(window, true);
	#endif
		
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	
	const int inbound_size = 124;
	const int outbound_size = 124;
	app_message_open(inbound_size, outbound_size);
	
	const bool animated = true;
	window_stack_push(window, animated);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void){
	init();
	app_event_loop();
	deinit();
}


