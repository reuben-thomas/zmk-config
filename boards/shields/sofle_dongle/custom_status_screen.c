/*
 * 128x32 SSD1306 OLED.
 *
 * +--------------------------------+
 * | L  88%   R  92%                |
 * | default                 42 WPM |
 * +--------------------------------+
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>

#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

static lv_obj_t *layer_label;

struct layer_state {
  zmk_keymap_layer_index_t index;
  const char *name;
};

static void set_layer_text(struct layer_state state) {
  if (state.name == NULL || strlen(state.name) == 0) {
    char text[8] = {};

    snprintf(text, sizeof(text), "%u", state.index);

    lv_label_set_text(layer_label, text);
  } else {
    lv_label_set_text(layer_label, state.name);
  }
}

static struct layer_state layer_get_state(const zmk_event_t *eh) {
  zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();

  return (struct layer_state){
      .index = index,
      .name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
  };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer, struct layer_state, set_layer_text,
                            layer_get_state)

ZMK_SUBSCRIPTION(widget_layer, zmk_layer_state_changed);

#if IS_ENABLED(CONFIG_ZMK_WPM)

#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

static lv_obj_t *wpm_label;

struct wpm_state {
  uint8_t wpm;
};

static void set_wpm_text(struct wpm_state state) {
  char text[12] = {};

  snprintf(text, sizeof(text), "%u WPM", state.wpm);

  lv_label_set_text(wpm_label, text);
}

static struct wpm_state wpm_get_state(const zmk_event_t *eh) {
  return (struct wpm_state){.wpm = zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm, struct wpm_state, set_wpm_text,
                            wpm_get_state)

ZMK_SUBSCRIPTION(widget_wpm, zmk_wpm_state_changed);

#endif /* IS_ENABLED(CONFIG_ZMK_WPM) */

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)

#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>

/*
 * Peripheral sources, in the order the halves are paired to the dongle. The
 * right half claims slot 0 and the left half slot 1 on this keyboard, which is
 * the opposite of what the naming suggests.
 */
#define SPLIT_SOURCE_RIGHT 0
#define SPLIT_SOURCE_LEFT 1
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

static struct split_battery_state
split_battery_get_state(const zmk_event_t *eh) {
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
  lv_obj_set_style_text_font(split_battery_label, &lv_font_montserrat_12,
                             LV_PART_MAIN);
  lv_obj_align(split_battery_label, LV_ALIGN_TOP_LEFT, 0, 0);

  widget_split_battery_init();
#endif

  layer_label = lv_label_create(screen);
  lv_obj_set_style_text_font(layer_label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align(layer_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  widget_layer_init();

#if IS_ENABLED(CONFIG_ZMK_WPM)
  wpm_label = lv_label_create(screen);
  lv_obj_set_style_text_font(wpm_label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align(wpm_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  widget_wpm_init();
#endif

  return screen;
}
