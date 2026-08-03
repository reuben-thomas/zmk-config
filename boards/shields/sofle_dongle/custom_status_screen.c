/*
 * Custom status screen for the dongle's 128x32 SSD1306 OLED.
 *
 *   +--------------------------------+
 *   | L  88%   R  92%                |  <- battery of each split half
 *   | (kbd) Base                     |  <- highest active layer
 *   +--------------------------------+
 *
 * The built in battery widget only knows about the battery of the device it
 * runs on, which on a dongle build is the dongle itself. The levels of the two
 * halves are only available on the central, via the peripheral battery events
 * raised when CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING is enabled,
 * so the per side display is done with a small custom widget here.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
#include <zmk/display/widgets/layer_status.h>

static struct zmk_widget_layer_status layer_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)

#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>

/* Peripheral sources, in the order the halves are paired to the dongle. */
#define SPLIT_SOURCE_LEFT 0
#define SPLIT_SOURCE_RIGHT 1
#define SPLIT_SOURCE_COUNT 2

struct split_battery_state {
    uint8_t levels[SPLIT_SOURCE_COUNT];
};

static lv_obj_t *split_battery_label;

/* A level of 0 means that half has not reported anything to us yet. */
static void format_level(char *buf, size_t len, uint8_t level) {
    if (level == 0) {
        snprintf(buf, len, "  --");
    } else {
        snprintf(buf, len, "%3u%%", level);
    }
}

static void set_split_battery_text(struct split_battery_state state) {
    char left[5] = {};
    char right[5] = {};
    char text[16] = {};

    format_level(left, sizeof(left), state.levels[SPLIT_SOURCE_LEFT]);
    format_level(right, sizeof(right), state.levels[SPLIT_SOURCE_RIGHT]);

    snprintf(text, sizeof(text), "L %s  R %s", left, right);

    lv_label_set_text(split_battery_label, text);
}

static struct split_battery_state split_battery_get_state(const zmk_event_t *eh) {
    struct split_battery_state state = {};

    for (uint8_t source = 0; source < SPLIT_SOURCE_COUNT; source++) {
        uint8_t level = 0;

        if (zmk_split_central_get_peripheral_battery_level(source, &level) == 0) {
            state.levels[source] = level;
        }
    }

    return state;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_split_battery, struct split_battery_state,
                            set_split_battery_text, split_battery_get_state)

ZMK_SUBSCRIPTION(widget_split_battery, zmk_peripheral_battery_state_changed);

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) */

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    split_battery_label = lv_label_create(screen);
    lv_obj_set_style_text_font(split_battery_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(split_battery_label, LV_ALIGN_TOP_LEFT, 0, 0);

    widget_split_battery_init();
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif

    return screen;
}
