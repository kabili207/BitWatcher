#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"

#define IOS false
 
// iOS and it's stupid walled garden aproach forces us to use a shared UUID, as defined in http.h,
// for httpebble access. Android does not have this limitation, however the first four UUID pairs,
// 9141B628-BC89-498E-B147, must match to be recognized by httpebble.
#if !IOS
#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0xFD, 0x3F, 0x10, 0x3E, 0x4F, 0xAE }
#else
#define MY_UUID HTTP_UUID
#endif

PBL_APP_INFO(MY_UUID,
             "BitWatcher", "Andrew Nagle",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

// POST variables
/* None yet */
// Received variables
#define BTC_KEY_CURRENCY 1
#define BTC_KEY_AVERAGE 2
#define BTC_KEY_ASK 3
#define BTC_KEY_BID 4
#define BTC_KEY_LAST 5
#define BTC_KEY_VOLUME 6

#define BTC_HTTP_COOKIE 1648231298


Window window;

TextLayer text_date_layer;
TextLayer text_time_layer;
TextLayer text_price_layer;
TextLayer text_currency_layer;
TextLayer text_buy_label_layer;
TextLayer text_buy_price_layer;
TextLayer text_sell_label_layer;
TextLayer text_sell_price_layer;

void request_btc();
void handle_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie);

void failed(int32_t cookie, int http_status, void* context) {
	if(cookie == 0 || cookie == BTC_HTTP_COOKIE) {
		text_layer_set_text(&text_price_layer, "---");
	}
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
	if(cookie != BTC_HTTP_COOKIE) return;
	
	Tuple* currency_tuple = dict_find(received, BTC_KEY_CURRENCY);
	if(currency_tuple) {
		// Must be static so the system can use it later
		static char str[10];
		char *curr = currency_tuple->value->cstring;
		strcpy(str, curr);
		strcat(str, " / BTC");
		text_layer_set_text(&text_currency_layer, str);
	}
	
	Tuple* price_tuple = dict_find(received, BTC_KEY_LAST);
	if(price_tuple) {
		text_layer_set_text(&text_price_layer, price_tuple->value->cstring);
	}
	
	Tuple* buy_tuple = dict_find(received, BTC_KEY_BID);
	if(buy_tuple) {
		text_layer_set_text(&text_buy_price_layer, buy_tuple->value->cstring);
	}
	
	Tuple* sell_tuple = dict_find(received, BTC_KEY_ASK);
	if(sell_tuple) {
		text_layer_set_text(&text_sell_price_layer, sell_tuple->value->cstring);
	}
}

void reconnect(void* context) {
	request_btc();
}

void set_timer(AppContextRef ctx) {
	app_timer_send_event(ctx, 1740000, 1);
}

void handle_init(AppContextRef ctx) {
	(void)ctx;

	resource_init_current_app(&APP_RESOURCES);
	window_init(&window, "BitWacher");
	window_stack_push(&window, true /* Animated */);
	window_set_background_color(&window, GColorBlack);
	
	text_layer_init(&text_price_layer, window.layer.frame);
	text_layer_set_text_color(&text_price_layer, GColorWhite);
	text_layer_set_background_color(&text_price_layer, GColorClear);
	layer_set_frame(&text_price_layer.layer, GRect(8, 0, 144-8, 168-0));
	text_layer_set_font(&text_price_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_MEDIUM_39)));
	text_layer_set_text(&text_price_layer, "---");
	layer_add_child(&window.layer, &text_price_layer.layer);

	text_layer_init(&text_currency_layer, window.layer.frame);
	text_layer_set_text_color(&text_currency_layer, GColorWhite);
 	text_layer_set_background_color(&text_currency_layer, GColorClear);
 	layer_set_frame(&text_currency_layer.layer, GRect(8, 41, 144-8, 168-41));
	text_layer_set_font(&text_currency_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_LIGHT_15)));
	text_layer_set_text(&text_currency_layer, "--- / BTC");
	layer_add_child(&window.layer, &text_currency_layer.layer);

	text_layer_init(&text_buy_label_layer, window.layer.frame);
	text_layer_set_text_color(&text_buy_label_layer, GColorWhite);
 	text_layer_set_background_color(&text_buy_label_layer, GColorClear);
 	layer_set_frame(&text_buy_label_layer.layer, GRect(8, 60, 144-8, 168-60));
	text_layer_set_font(&text_buy_label_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_LIGHT_15)));
	text_layer_set_text(&text_buy_label_layer, "BUY");
	layer_add_child(&window.layer, &text_buy_label_layer.layer);

	text_layer_init(&text_buy_price_layer, window.layer.frame);
	text_layer_set_text_color(&text_buy_price_layer, GColorWhite);
 	text_layer_set_background_color(&text_buy_price_layer, GColorClear);
 	layer_set_frame(&text_buy_price_layer.layer, GRect(56, 60, 144-56, 168-60));
	text_layer_set_font(&text_buy_price_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_LIGHT_15)));
	text_layer_set_text(&text_buy_price_layer, "---");
	layer_add_child(&window.layer, &text_buy_price_layer.layer);

	text_layer_init(&text_sell_label_layer, window.layer.frame);
	text_layer_set_text_color(&text_sell_label_layer, GColorWhite);
 	text_layer_set_background_color(&text_sell_label_layer, GColorClear);
 	layer_set_frame(&text_sell_label_layer.layer, GRect(8, 78, 144-8, 168-78));
	text_layer_set_font(&text_sell_label_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_LIGHT_15)));
	text_layer_set_text(&text_sell_label_layer, "SELL");
	layer_add_child(&window.layer, &text_sell_label_layer.layer);

	text_layer_init(&text_sell_price_layer, window.layer.frame);
	text_layer_set_text_color(&text_sell_price_layer, GColorWhite);
 	text_layer_set_background_color(&text_sell_price_layer, GColorClear);
 	layer_set_frame(&text_sell_price_layer.layer, GRect(56, 78, 144-56, 168-78));
	text_layer_set_font(&text_sell_price_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_LIGHT_15)));
	text_layer_set_text(&text_sell_price_layer, "---");
	layer_add_child(&window.layer, &text_sell_price_layer.layer);
	
	text_layer_init(&text_date_layer, window.layer.frame);
	text_layer_set_text_color(&text_date_layer, GColorWhite);
	text_layer_set_background_color(&text_date_layer, GColorClear);
	layer_set_frame(&text_date_layer.layer, GRect(8, 96, 144-8, 168-96));
	text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_MEDIUM_21)));
	layer_add_child(&window.layer, &text_date_layer.layer);


	text_layer_init(&text_time_layer, window.layer.frame);
	text_layer_set_text_color(&text_time_layer, GColorWhite);
 	text_layer_set_background_color(&text_time_layer, GColorClear);
 	layer_set_frame(&text_time_layer.layer, GRect(7, 114, 144-7, 168-114));
	text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIAVLO_HEAVY_45)));
	layer_add_child(&window.layer, &text_time_layer.layer);

	
	http_register_callbacks((HTTPCallbacks){
		.failure=failed,
		.success=success,
		.reconnect=reconnect
	}, (void*)ctx);
	
	request_btc();
}


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {

	// Need to be static because they're used by the system later.
	static char time_text[] = "00:00";
	static char date_text[] = "Xxxxxxxxx 00";

	char *time_format;


	// TODO: Only update the date when it's changed.
	string_format_time(date_text, sizeof(date_text), "%B %e", t->tick_time);
	text_layer_set_text(&text_date_layer, date_text);


	if (clock_is_24h_style()) {
		time_format = "%R";
	} else {
		time_format = "%I:%M";
	}

	string_format_time(time_text, sizeof(time_text), time_format, t->tick_time);

	// Kludge to handle lack of non-padded hour format string
	// for twelve hour clock.
	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}

	text_layer_set_text(&text_time_layer, time_text);

}

void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.timer_handler = handle_timer,
		.tick_info = {
			.tick_handler = &handle_minute_tick,
			.tick_units = MINUTE_UNIT
		},
		.messaging_info = {
			.buffer_sizes = {
				.inbound = 124,
				.outbound = 256,
			}
		}
	};
	app_event_loop(params, &handlers);
}
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
	request_btc();
	// Update again in fifteen minutes.
	if(cookie)
		set_timer(ctx);
}

void request_btc() {
	
	// Build the HTTP request
	DictionaryIterator *body;
	HTTPResult result = http_out_get("http://pebble.zyrenth.com/btc_price.json", BTC_HTTP_COOKIE, &body);
	if(result != HTTP_OK) {
		text_layer_set_text(&text_price_layer, "---");
		return;
	}
	
	// Send it.
	if(http_out_send() != HTTP_OK) {
		text_layer_set_text(&text_price_layer, "---");
		return;
	}
	
}
