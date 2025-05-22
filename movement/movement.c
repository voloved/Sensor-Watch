/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define MOVEMENT_LONG_PRESS_TICKS 64
#define DEBOUNCE_TICKS_DOWN  3
#define DEBOUNCE_TICKS_UP   20
/*
DEBOUNCE_TICKS_DOWN and DEBOUNCE_TICKS_UP are in terms of fast_cb ticks after a button is pressed.
The logic is that pressed of a button are ignored until the cb_fast_tick function runs this variable amount of times.
Without modifying the code, the cb_fast_tick frequency is 128Hz, or 7.8125ms.
It is not suggested to set this value to one for debouncing, as the callback occurs asynchronously of the button's press,
meaning that if a button was pressed and 7ms passed since th elast time cb_fast_tick was called, then there will be only 812.5us
of debounce time.
*/

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "watch.h"
#include "watch_utility.h"
#include "filesystem.h"
#include "movement.h"
#include "shell.h"
#include "thermistor_driver.h"
#include "utz.h"
#include "zones.h"

#ifndef MOVEMENT_FIRMWARE
#include "movement_config.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_STANDARD
#include "movement_config.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_BACKER
#include "alt_fw/backer.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_ALT_TIME
#include "alt_fw/alt_time.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_FOCUS
#include "alt_fw/focus.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_THE_BACKPACKER
#include "alt_fw/the_backpacker.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_THE_ATHLETE
#include "alt_fw/the_athlete.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_THE_STARGAZER
#include "alt_fw/the_stargazer.h"
#elif MOVEMENT_FIRMWARE == MOVEMENT_FIRMWARE_DEEP_SPACE_NOW
#include "alt_fw/deep_space_now.h"
#endif

#include "movement_custom_signal_tunes.h"

// Default to no secondary face behaviour.
#ifndef MOVEMENT_SECONDARY_FACE_INDEX
#define MOVEMENT_SECONDARY_FACE_INDEX 0
#endif

// Set default LED colors if not set
#ifndef MOVEMENT_DEFAULT_RED_COLOR
#define MOVEMENT_DEFAULT_RED_COLOR 0x0
#endif
#ifndef MOVEMENT_DEFAULT_GREEN_COLOR
#define MOVEMENT_DEFAULT_GREEN_COLOR 0xF
#endif

// Default to 12h mode
#ifndef MOVEMENT_DEFAULT_24H_MODE
#define MOVEMENT_DEFAULT_24H_MODE false
#endif

// Default to mode button sounding on press
#ifndef MOVEMENT_DEFAULT_BUTTON_SOUND
#define MOVEMENT_DEFAULT_BUTTON_SOUND true
#endif

// Default to switch back to main watch face after 60 seconds
#ifndef MOVEMENT_DEFAULT_TIMEOUT_INTERVAL
#define MOVEMENT_DEFAULT_TIMEOUT_INTERVAL 0
#endif

// Default to switch to low energy mode after 2 hours
#ifndef MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL
#define MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL 2
#endif

// Default to 1 second led duration
#ifndef MOVEMENT_DEFAULT_LED_DURATION
#define MOVEMENT_DEFAULT_LED_DURATION 2
#endif

// Default to no set location latitude
#ifndef MOVEMENT_DEFAULT_LATITUDE
#define MOVEMENT_DEFAULT_LATITUDE 0
#endif

// Default to no set location longitude
#ifndef MOVEMENT_DEFAULT_LONGITUDE
#define MOVEMENT_DEFAULT_LONGITUDE 0
#endif

// Default to no set birthdate year
#ifndef MOVEMENT_DEFAULT_BIRTHDATE_YEAR
#define MOVEMENT_DEFAULT_BIRTHDATE_YEAR 0
#endif

// Default to no set birthdate month
#ifndef MOVEMENT_DEFAULT_BIRTHDATE_MONTH
#define MOVEMENT_DEFAULT_BIRTHDATE_MONTH 0
#endif

// Default to no set birthdate day
#ifndef MOVEMENT_DEFAULT_BIRTHDATE_DAY
#define MOVEMENT_DEFAULT_BIRTHDATE_DAY 0
#endif

// Default to not always chiming every hour
#ifndef MOVEMENT_DEFAULT_HOURLY_CHIME_ALWAYS
#define MOVEMENT_DEFAULT_HOURLY_CHIME_ALWAYS 0
#endif

// Default to beginning a chime at 7am
#ifndef MOVEMENT_DEFAULT_HOURLY_CHIME_START
#define MOVEMENT_DEFAULT_HOURLY_CHIME_START 1
#endif

// Default to beginning a chime at 9pm
#ifndef MOVEMENT_DEFAULT_HOURLY_CHIME_END
#define MOVEMENT_DEFAULT_HOURLY_CHIME_END 1
#endif

// Default to having deep sleep be on
#ifndef MOVEMENT_DEFAULT_LE_DEEP_SLEEP
#define MOVEMENT_DEFAULT_LE_DEEP_SLEEP true
#endif

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

movement_state_t movement_state;
void * watch_face_contexts[MOVEMENT_NUM_FACES];
watch_date_time scheduled_tasks[MOVEMENT_NUM_FACES];
const int32_t movement_le_inactivity_deadlines[8] = {INT_MAX, 600, 3600, 7200, 21600, 43200, 86400, 604800};
const int32_t movement_le_deep_sleep_deadline = 4 * 60; // In minutes (will trigger at the top of the minute, rounded up from the LE timeout tick)
const int16_t movement_timeout_inactivity_deadlines[4] = {60, 120, 300, 1800};
int8_t g_temperature_c = -128;
uint8_t g_force_sleep; // 0 = no sleep forced; 1 = normal sleep; 2 = deep sleep
static bool _woke_up_for_buzzer;
static watch_date_time _dst_last_cache;
movement_event_t event;

int8_t _movement_dst_offset_cache[NUM_ZONE_NAMES] = {0};
#define TIMEZONE_DOES_NOT_OBSERVE (-127)

const char movement_valid_position_0_chars[] = " AaBbCcDdEeFGgHhIiJKLMNnOoPQrSTtUuWXYZ-='+\\/0123456789";
const char movement_valid_position_1_chars[] = " ABCDEFHlJLNORTtUX-='01378";

void cb_mode_btn_interrupt(void);
void cb_light_btn_interrupt(void);
void cb_alarm_btn_interrupt(void);
void cb_alarm_btn_extwake(void);
void cb_alarm_fired(void);
void cb_fast_tick(void);
void cb_tick(void);

static udatetime_t _movement_convert_date_time_to_udate(watch_date_time date_time) {
    return (udatetime_t) {
        .date.dayofmonth = date_time.unit.day,
        .date.dayofweek = dayofweek(UYEAR_FROM_YEAR(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR), date_time.unit.month, date_time.unit.day),
        .date.month = date_time.unit.month,
        .date.year = UYEAR_FROM_YEAR(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR),
        .time.hour = date_time.unit.hour,
        .time.minute = date_time.unit.minute,
        .time.second = date_time.unit.second
    };
}

static watch_date_time _movement_convert_udate_to_date_time(udatetime_t date_time) {
    return (watch_date_time) {
        .unit.day = date_time.date.dayofmonth,
        .unit.month = date_time.date.month,
        .unit.year = date_time.date.year - (WATCH_RTC_REFERENCE_YEAR - UYEAR_OFFSET),
        .unit.hour = date_time.time.hour,
        .unit.minute = date_time.time.minute,
        .unit.second = date_time.time.second
    };
}

static bool _movement_update_dst_offset_cache(watch_date_time system_date_time) {
    uzone_t local_zone;
    udatetime_t udate_time;
    bool dst_changed = false;

    for (uint8_t i = 0; i < NUM_ZONE_NAMES; i++) {
        unpack_zone(&zone_defns[i], "", &local_zone);
        watch_date_time date_time = watch_utility_date_time_convert_zone(system_date_time, 0, local_zone.offset.hours * 3600 + local_zone.offset.minutes * 60);

        if (!!local_zone.rules_len) {
            // if local zone has DST rules, we need to see if DST applies.
            udate_time = _movement_convert_date_time_to_udate(date_time);
            uoffset_t offset;
            get_current_offset(&local_zone, &udate_time, &offset);
            int8_t new_offset = (offset.hours * 60 + offset.minutes) / OFFSET_INCREMENT;
            if (_movement_dst_offset_cache[i] != new_offset) {
                _movement_dst_offset_cache[i] = new_offset;
                dst_changed = true;
            }
        } else {
            // otherwise set the cache to a constant value that indicates no DST check needs to be performed.
            _movement_dst_offset_cache[i] = TIMEZONE_DOES_NOT_OBSERVE;
        }
    }
    _dst_last_cache.reg = system_date_time.reg;
    return dst_changed;
}

static bool _movement_check_dst_changeover_occurring_now(watch_date_time date_time) {
    static watch_date_time dst_occur_date[2];  // Same length as the maximum of zone_defns[].rules_len
    static uint8_t year_prev = 0;
    static uint8_t tz_idx_prev = 0;
    uint8_t tz_idx_curr = movement_get_timezone_index();
    uint8_t rules_idx = zone_defns[tz_idx_curr].rules_idx;
    uint8_t rules_len = zone_defns[tz_idx_curr].rules_len;
    if (rules_len == 0) return false;
    // Get the DST dates if we don't already have them or they're outdated
    if (dst_occur_date[0].reg == 0 || tz_idx_curr != tz_idx_prev || date_time.unit.year != year_prev) {
        uzone_t local_zone;
        year_prev = date_time.unit.year;
        tz_idx_prev = tz_idx_curr;
        unpack_zone(&zone_defns[tz_idx_curr], "", &local_zone);
        for(uint8_t i = 0; i < rules_len; i++) {
            urule_t unpacked_rule;
            uoffset_t offset;
            unpack_rule(&zone_rules[rules_idx + i], date_time.unit.year + (WATCH_RTC_REFERENCE_YEAR - 2000), &unpacked_rule);
            dst_occur_date[i] = _movement_convert_udate_to_date_time(unpacked_rule.datetime);
            get_current_offset(&local_zone, &unpacked_rule.datetime, &offset);
            int32_t sec_offset = (offset.hours * 60 + offset.minutes) * 60;
            if (unpacked_rule.is_local_time == 0) {
                int32_t offset_non_dst = zone_defns[tz_idx_curr].offset_inc_minutes * OFFSET_INCREMENT * 60;
                dst_occur_date[i] = watch_utility_date_time_convert_zone(dst_occur_date[i], 0, offset_non_dst);
            }
            dst_occur_date[i] = watch_utility_date_time_convert_zone(dst_occur_date[i], sec_offset, movement_get_current_timezone_offset());
        }
    }
    // See if the current time is during DST
    for(uint8_t i = 0; i < rules_len; i++) {
        if (date_time.unit.month != dst_occur_date[i].unit.month) continue;
        if (date_time.unit.day != dst_occur_date[i].unit.day) continue;
        if (date_time.unit.hour != dst_occur_date[i].unit.hour) continue;
        if (date_time.unit.minute != dst_occur_date[i].unit.minute) continue;
        return true;
    }
    return false;
}

static uint8_t _movement_get_rise_set_hour(double time, double hours_from_utc, bool use_end_of_hour) {
    time += hours_from_utc;
    uint8_t hour_to_start = (uint8_t)time;
    double minutes = (time - hour_to_start) * 60;
    if (use_end_of_hour && minutes >= 0.5)
        hour_to_start = (hour_to_start + 1) % 24;
    if (hour_to_start == 0) hour_to_start = 24;
    return hour_to_start;
}

static bool _movement_compare_rise_set_dates(rise_set_check_t curr, rise_set_check_t prev) {
    if (!prev.foundTime) return false;
    if (curr.year != prev.year) return false;
    if (curr.month != prev.month) return false;
    if (curr.day != prev.day) return false;
    if (curr.latitude_centi != prev.latitude_centi) return false;
    if (curr.longitude_centi != prev.longitude_centi) return false;
    if (curr.tz_idx != prev.tz_idx) return false;
    return true;    
}

static bool _movement_get_if_daytime(watch_date_time date_time, watch_date_time utc_now, rise_set_check_t *prev_rise_set_info) {
    const uint8_t rise_fallback =  5; // If time's not found, use this hour for the end of sunrise
    const uint8_t fall_fallback = 22; // If time's not found, use this hour for the end of sunset
    rise_set_check_t rise_set_info = {0};
    rise_set_info.tz_idx = movement_get_timezone_index();

    if (movement_state.location.reg == 0) {
        prev_rise_set_info->foundTime = false;
        return ((date_time.unit.hour >= rise_fallback) && (date_time.unit.hour < fall_fallback));
    }
    double rise, set;
    rise_set_info.year = date_time.unit.year;
    rise_set_info.month = date_time.unit.month;
    rise_set_info.day = date_time.unit.day;
    rise_set_info.latitude_centi = (int16_t)movement_state.location.bit.latitude;
    rise_set_info.longitude_centi = (int16_t)movement_state.location.bit.longitude;
    if (_movement_compare_rise_set_dates(rise_set_info, *prev_rise_set_info)) {
        return ((date_time.unit.hour >= rise_fallback) && (date_time.unit.hour < fall_fallback));
    }
    *prev_rise_set_info = rise_set_info;
    double lat = (double)rise_set_info.latitude_centi / 100.0;
    double lon = (double)rise_set_info.longitude_centi / 100.0;
    int32_t tz = movement_get_current_timezone_offset_for_zone(rise_set_info.tz_idx);
    double hours_from_utc = ((double)tz) / 3600.0;
    uint8_t result = sun_rise_set(utc_now.unit.year + WATCH_RTC_REFERENCE_YEAR, utc_now.unit.month, utc_now.unit.day, lon, lat, &rise, &set);
    if (result != 0) {
        prev_rise_set_info->foundTime = false;
        return ((date_time.unit.hour >= rise_fallback) && (date_time.unit.hour < fall_fallback));
    }
    uint8_t rise_hr = _movement_get_rise_set_hour(rise, hours_from_utc, true);
    uint8_t fall_hr =  _movement_get_rise_set_hour(set, hours_from_utc, false);
    prev_rise_set_info->foundTime = true;
    return ((date_time.unit.hour >= rise_hr) && (date_time.unit.hour < fall_hr));
}

static inline void _movement_reset_inactivity_countdown(void) {
    movement_state.le_mode_ticks = movement_le_inactivity_deadlines[movement_state.settings.bit.le_interval];
    movement_state.timeout_ticks = movement_timeout_inactivity_deadlines[movement_state.settings.bit.to_interval];
    movement_state.le_deep_sleeping_ticks = movement_le_deep_sleep_deadline;
    _woke_up_for_buzzer = false;
}

static inline void _reset_debounce_ticks(void) {
#if (DEBOUNCE_TICKS_DOWN || DEBOUNCE_TICKS_UP)
    movement_state.debounce_ticks_light = 0;
    movement_state.debounce_ticks_alarm = 0;
    movement_state.debounce_ticks_mode = 0;
#endif
}

static inline bool _debounce_ticks_all_zero(void) {
#if (DEBOUNCE_TICKS_DOWN || DEBOUNCE_TICKS_UP)
        return ((movement_state.debounce_ticks_light | movement_state.debounce_ticks_mode | movement_state.debounce_ticks_alarm) == 0);
#else
    return true;
#endif
}

static inline void _movement_enable_fast_tick_if_needed(void) {
    if (!movement_state.fast_tick_enabled) {
        movement_state.fast_ticks = 0;
        watch_rtc_register_periodic_callback(cb_fast_tick, 128);
        movement_state.fast_tick_enabled = true;
    }
}

static inline void _movement_disable_fast_tick_if_possible(void) {
    if ((movement_state.light_ticks == -1) &&
        (movement_state.alarm_ticks == -1) &&
        _debounce_ticks_all_zero() &&
        ((movement_state.light_down_timestamp + movement_state.mode_down_timestamp + movement_state.alarm_down_timestamp) == 0)) {
        movement_state.fast_tick_enabled = false;
        watch_rtc_disable_periodic_callback(128);
    }
}

static void _decrement_deep_sleep_counter(watch_date_time date_time){
    if(!movement_state.settings.bit.screen_off_after_le) return;
    if (movement_state.le_deep_sleeping_ticks == -1) return;
    // Don't turn off the display during hour where people are unlikely to wear it
    if(date_time.unit.hour >= 22 || date_time.unit.hour < 6) return;
    if(movement_state.le_mode_ticks != -1) {
        movement_state.le_deep_sleeping_ticks = movement_le_deep_sleep_deadline;
        return;
    }
    // Reset whenever the temperature is high enough, so that way, we need multiple hours in a row before going into deep sleep.    
    if (g_temperature_c >= TEMPERATURE_ASSUME_WEARING) movement_state.le_deep_sleeping_ticks = movement_le_deep_sleep_deadline;
    else if (movement_state.le_deep_sleeping_ticks > 0) movement_state.le_deep_sleeping_ticks--;
    else{
        if (g_temperature_c == -128) return; // Ignore when the temp is not first read without affecting the timer.
        // Re-check temperature in case the user wore the watch after the last reading.
        thermistor_driver_enable();
        g_temperature_c = thermistor_driver_get_temperature();
        thermistor_driver_disable();
        if (g_temperature_c < TEMPERATURE_ASSUME_WEARING){
            movement_state.le_deep_sleeping_ticks = -1;
            return;
        }
        movement_state.le_deep_sleeping_ticks = movement_le_deep_sleep_deadline;
    }
}

static void _movement_handle_background_tasks(void) {
    watch_date_time utc_now = movement_get_utc_date_time();
    watch_date_time date_time = watch_utility_date_time_convert_zone(utc_now, 0, movement_get_current_timezone_offset());

    // update the DST offset cache if the current time matches the DST minute, hour, and month
    if (_movement_check_dst_changeover_occurring_now(date_time)) {
        _movement_update_dst_offset_cache(utc_now);
    }
    
    if (date_time.unit.minute == 0) {
        movement_state.settings.bit.is_daytime = _movement_get_if_daytime(date_time, utc_now, &movement_state.prev_sun_info);
    }

    for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
        // For each face, if the watch face wants a background task...
        if (watch_faces[i].wants_background_task != NULL && watch_faces[i].wants_background_task(&movement_state.settings, watch_face_contexts[i])) {
            // ...we give it one. pretty straightforward!
            movement_event_t background_event = { EVENT_BACKGROUND_TASK, 0 };
            watch_faces[i].loop(background_event, &movement_state.settings, watch_face_contexts[i]);
        }
    }
    _decrement_deep_sleep_counter(date_time);
    movement_state.needs_background_tasks_handled = false;
}

static void _movement_handle_scheduled_tasks(void) {
    watch_date_time date_time = movement_get_utc_date_time();
    uint8_t num_active_tasks = 0;

    for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
        if (scheduled_tasks[i].reg) {
            if (scheduled_tasks[i].reg <= date_time.reg) {
                scheduled_tasks[i].reg = 0;
                movement_event_t background_event = { EVENT_BACKGROUND_TASK, 0 };
                watch_faces[i].loop(background_event, &movement_state.settings, watch_face_contexts[i]);
                // check if loop scheduled a new task
                if (scheduled_tasks[i].reg) {
                    num_active_tasks++;
                }
            } else {
                num_active_tasks++;
            }
        }
    }

    if (num_active_tasks == 0) {
        movement_state.has_scheduled_background_task = false;
    } else {
        _movement_reset_inactivity_countdown();
    }
}

void movement_request_tick_frequency(uint8_t freq) {
    // Movement uses the 128 Hz tick internally
    if (freq == 128) return;

    // Movement requires at least a 1 Hz tick.
    // If we are asked for an invalid frequency, default back to 1 Hz.
    if (freq == 0 || __builtin_popcount(freq) != 1) freq = 1;

    // disable all callbacks except the 128 Hz one
    watch_rtc_disable_matching_periodic_callbacks(0xFE);

    movement_state.subsecond = 0;
    movement_state.tick_frequency = freq;
    watch_rtc_register_periodic_callback(cb_tick, freq);
}

void movement_illuminate_led(void) {
    if (movement_state.settings.bit.led_duration) {
        watch_set_led_color(movement_state.settings.bit.led_red_color ? (0xF | movement_state.settings.bit.led_red_color << 4) : 0,
                            movement_state.settings.bit.led_green_color ? (0xF | movement_state.settings.bit.led_green_color << 4) : 0);
        switch (movement_state.settings.bit.led_duration)
        {
        case 1:
            movement_state.light_ticks = 32;
            break;
        case 2:
            movement_state.light_ticks = 128;
            break;
        case 3:
            movement_state.light_ticks = 384;
            break;
        }
        _movement_enable_fast_tick_if_needed();
    }
}

void go_to_teriary_face(void) {
    movement_move_to_face(MOVEMENT_TERIARY_FACE_INDEX);
}

static void display_time(watch_date_time date_time, bool clock_mode_24h) {
    char buf[8 + 1];
    if (clock_mode_24h) watch_set_indicator(WATCH_INDICATOR_24H);
    else {
        if (date_time.unit.hour >= 12) watch_set_indicator(WATCH_INDICATOR_PM);
        date_time.unit.hour %= 12;
        if (date_time.unit.hour == 0) date_time.unit.hour = 12;
    }
    watch_set_colon();
    sprintf(buf, "%2d%2d%02d  ", date_time.unit.day, date_time.unit.hour, date_time.unit.minute);
    watch_display_string(buf, 2);
}

bool movement_default_loop_handler(movement_event_t event, movement_settings_t *settings) {
    (void)settings;

    switch (event.event_type) {
        case EVENT_MODE_BUTTON_UP:
            if (movement_state.current_face_idx == MOVEMENT_TERIARY_FACE_INDEX - 1) {
                movement_move_to_face(0);
            } else if (movement_state.current_face_idx == MOVEMENT_NUM_FACES - 1) {
                go_to_teriary_face();
            } else {
                movement_move_to_next_face();
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;
        case EVENT_MODE_LONG_PRESS:
            if (MOVEMENT_SECONDARY_FACE_INDEX && movement_state.current_face_idx == 0) {
                movement_move_to_face(MOVEMENT_SECONDARY_FACE_INDEX);
            } else {
                movement_move_to_face(0);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (movement_state.current_face_idx >= (int16_t)MOVEMENT_TERIARY_FACE_INDEX)
                display_time(movement_get_local_date_time(), movement_state.settings.bit.clock_mode_24h);
            break;
        default:
            break;
    }

    return true;
}

void movement_move_to_face(uint8_t watch_face_index) {
    movement_state.watch_face_changed = true;
    movement_state.next_face_idx = watch_face_index;
}

void movement_move_to_next_face(void) {
    uint16_t face_max;
    if (MOVEMENT_TERIARY_FACE_INDEX != 0 && MOVEMENT_SECONDARY_FACE_INDEX != 0) {
        face_max = (movement_state.current_face_idx < (int16_t)MOVEMENT_SECONDARY_FACE_INDEX) ? MOVEMENT_SECONDARY_FACE_INDEX : 
                (movement_state.current_face_idx < (int16_t)MOVEMENT_TERIARY_FACE_INDEX) ? MOVEMENT_TERIARY_FACE_INDEX : MOVEMENT_NUM_FACES;
    } else if (MOVEMENT_TERIARY_FACE_INDEX) {
        face_max = (movement_state.current_face_idx < (int16_t)MOVEMENT_TERIARY_FACE_INDEX) ? MOVEMENT_TERIARY_FACE_INDEX : MOVEMENT_NUM_FACES;
    } else if (MOVEMENT_SECONDARY_FACE_INDEX) {
        face_max = (movement_state.current_face_idx < (int16_t)MOVEMENT_SECONDARY_FACE_INDEX) ? MOVEMENT_SECONDARY_FACE_INDEX : MOVEMENT_NUM_FACES;
    } else {
        face_max = MOVEMENT_NUM_FACES;
    }
    movement_move_to_face((movement_state.current_face_idx + 1) % face_max);
}

void movement_schedule_background_task(watch_date_time date_time) {
    movement_schedule_background_task_for_face(movement_state.current_face_idx, date_time);
}

void movement_cancel_background_task(void) {
    movement_cancel_background_task_for_face(movement_state.current_face_idx);
}

void movement_schedule_background_task_for_face(uint8_t watch_face_index, watch_date_time date_time) {
    watch_date_time now = movement_get_utc_date_time();
    if (date_time.reg > now.reg) {
        movement_state.has_scheduled_background_task = true;
        scheduled_tasks[watch_face_index].reg = date_time.reg;
    }
}

void movement_cancel_background_task_for_face(uint8_t watch_face_index) {
    scheduled_tasks[watch_face_index].reg = 0;
    bool other_tasks_scheduled = false;
    for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
        if (scheduled_tasks[i].reg != 0) {
            other_tasks_scheduled = true;
            break;
        }
    }
    movement_state.has_scheduled_background_task = other_tasks_scheduled;
}

void movement_request_wake() {
    movement_state.needs_wake = true;
    _movement_reset_inactivity_countdown();
}

static void end_buzzing() {
    movement_state.is_buzzing = false;
}

static void end_buzzing_and_disable_buzzer(void) {
    end_buzzing();
    watch_disable_buzzer();
}

static void set_initial_clock_mode(void) {
#ifdef CLOCK_FACE_24H_ONLY
    movement_state.settings.bit.clock_mode_24h = true;
#else
    if(MOVEMENT_DEFAULT_24H_MODE == 1)
        movement_state.settings.bit.clock_mode_24h = true;
    else
        movement_state.settings.bit.clock_mode_24h = false;
#endif
}

void movement_play_signal(uint8_t chime) {
    void *maybe_disable_buzzer = end_buzzing_and_disable_buzzer;
    if (movement_state.le_deep_sleeping_ticks == -1) return;
    if (watch_is_buzzer_or_led_enabled()) {
        maybe_disable_buzzer = end_buzzing;
    } else {
        watch_enable_buzzer();
    }
    movement_state.is_buzzing = true;
    watch_buzzer_play_sequence(signal_tune[chime], maybe_disable_buzzer);
    if (movement_state.le_mode_ticks == -1) {
        // the watch is asleep. wake it up for "1" round through the main loop.
        // the sleep_mode_app_loop will notice the is_buzzing and note that it
        // only woke up to beep and then it will spinlock until the callback
        // turns off the is_buzzing flag.
        movement_state.needs_wake = true;
        movement_state.le_mode_ticks = 1;
    }
}

void movement_play_alarm(void) {
    movement_play_alarm_beeps(5, BUZZER_NOTE_C8);
}

void movement_play_alarm_beeps(uint8_t rounds, BuzzerNote alarm_note) {
    if (rounds == 0) rounds = 1;
    if (rounds > 20) rounds = 20;
    movement_request_wake();
    movement_state.alarm_note = alarm_note;
    // our tone is 0.375 seconds of beep and 0.625 of silence, repeated as given.
    movement_state.alarm_ticks = 128 * rounds - 75;
    _movement_enable_fast_tick_if_needed();
}

uint8_t movement_claim_backup_register(void) {
    if (movement_state.next_available_backup_register >= 8) return 0;
    return movement_state.next_available_backup_register++;
}

int32_t movement_get_current_timezone_offset_for_zone(uint8_t zone_index) {
    int8_t cached_dst_offset = _movement_dst_offset_cache[zone_index];

    if (cached_dst_offset == TIMEZONE_DOES_NOT_OBSERVE) {
        // if time zone doesn't observe DST, we can just return the standard time offset from the zone definition.
        return (int32_t)zone_defns[zone_index].offset_inc_minutes * OFFSET_INCREMENT * 60;
    } else {
        // otherwise, we've precalculated the offset for this zone and can return it.
        return (int32_t)cached_dst_offset * OFFSET_INCREMENT * 60;
    }
}

int32_t movement_get_current_timezone_offset(void) {
    return movement_get_current_timezone_offset_for_zone(movement_state.settings.bit.time_zone);
}

int32_t movement_get_timezone_index(void) {
    return movement_state.settings.bit.time_zone;
}

void movement_set_timezone_index(uint8_t value) {
    movement_state.settings.bit.time_zone = value;
}

watch_date_time movement_get_utc_date_time(void) {
    return watch_rtc_get_date_time();
}

bool movement_update_dst_offset_cache(void) {
    return _movement_update_dst_offset_cache(movement_get_utc_date_time());
}

static bool dst_cache_may_be_stale(watch_date_time utc_now) {
    // If _dst_last_cache was never set, default to recalculating
    if (_dst_last_cache.reg == 0) return true;
    // If we time-travelled, assume it's stale
    if(_dst_last_cache.reg > utc_now.reg) return true;
    // Checks if the yr, mo, day, and hr are all the same and says the data may be stale if not.
    if(((utc_now.reg ^ _dst_last_cache.reg) >> 12) != 0) return true;
    const uint8_t min_to_trigger = 30;  // We want to check every half-hour, but no need to cache more than once in a half-hour.
    int8_t delta_actual = utc_now.unit.minute - _dst_last_cache.unit.minute;
    if (delta_actual == 0) return false;
    int8_t delta_min = min_to_trigger - (_dst_last_cache.unit.minute % min_to_trigger);
    if (delta_actual >= delta_min || delta_actual < 0) return true;
    return false;  
}

bool movement_update_dst_offset_cache_if_needed(watch_date_time utc_now) {
    if (dst_cache_may_be_stale(utc_now))
        return _movement_update_dst_offset_cache(utc_now);
    return false;
}

watch_date_time movement_get_date_time_in_zone(uint8_t zone_index) {
    watch_date_time date_time = movement_get_utc_date_time();
    int32_t offset = movement_get_current_timezone_offset_for_zone(zone_index);
    movement_update_dst_offset_cache_if_needed(date_time);
    return watch_utility_date_time_convert_zone(date_time, 0, offset);
}

watch_date_time movement_get_local_date_time(void) {
    watch_date_time date_time = watch_rtc_get_date_time();
    return watch_utility_date_time_convert_zone(date_time, 0, movement_get_current_timezone_offset());
}

void movement_set_local_date_time(watch_date_time date_time) {
    int32_t current_offset = movement_get_current_timezone_offset();
    watch_date_time utc_date_time = watch_utility_date_time_convert_zone(date_time, current_offset, 0);
    watch_rtc_set_date_time(utc_date_time);
}

void app_init(void) {
#if defined(NO_FREQCORR)
    watch_rtc_freqcorr_write(0, 0);
#elif defined(WATCH_IS_BLUE_BOARD)
    watch_rtc_freqcorr_write(11, 0);
#else
    watch_rtc_freqcorr_write(22, 0);
#endif

    memset(&movement_state, 0, sizeof(movement_state));
    if (MOVEMENT_DEFAULT_24H_MODE <= 1) {
        movement_state.settings.bit.clock_mode_24h = MOVEMENT_DEFAULT_24H_MODE;
        movement_state.settings.bit.clock_mode_toggle = false;  
    }
    else {
        movement_state.settings.bit.clock_mode_24h = false;
        movement_state.settings.bit.clock_mode_toggle = true;
    }
    set_initial_clock_mode();
    movement_state.settings.bit.led_red_color = MOVEMENT_DEFAULT_RED_COLOR;
    movement_state.settings.bit.led_green_color = MOVEMENT_DEFAULT_GREEN_COLOR;
    movement_state.settings.bit.button_should_sound = MOVEMENT_DEFAULT_BUTTON_SOUND;
    movement_state.settings.bit.to_interval = MOVEMENT_DEFAULT_TIMEOUT_INTERVAL;
    movement_state.settings.bit.le_interval = MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL;
    movement_state.settings.bit.led_duration = MOVEMENT_DEFAULT_LED_DURATION;
    movement_state.location.bit.latitude = MOVEMENT_DEFAULT_LATITUDE;
    movement_state.location.bit.longitude = MOVEMENT_DEFAULT_LONGITUDE;
    movement_state.birthdate.bit.year = MOVEMENT_DEFAULT_BIRTHDATE_YEAR;
    movement_state.birthdate.bit.month = MOVEMENT_DEFAULT_BIRTHDATE_MONTH;
    movement_state.birthdate.bit.day = MOVEMENT_DEFAULT_BIRTHDATE_DAY;
    movement_state.settings.bit.hourly_chime_always = MOVEMENT_DEFAULT_HOURLY_CHIME_ALWAYS;
    movement_state.settings.bit.hourly_chime_start = MOVEMENT_DEFAULT_HOURLY_CHIME_START;
    movement_state.settings.bit.hourly_chime_end = MOVEMENT_DEFAULT_HOURLY_CHIME_END;
    movement_state.settings.bit.screen_off_after_le = MOVEMENT_DEFAULT_LE_DEEP_SLEEP;

    movement_state.light_ticks = -1;
    movement_state.alarm_ticks = -1;
    movement_state.next_available_backup_register = 4;
    _movement_reset_inactivity_countdown();

    filesystem_init();
}

void app_wake_from_backup(void) {
    movement_state.settings.reg = watch_get_backup_data(0);
    movement_state.location.reg = watch_get_backup_data(1);
    movement_state.birthdate.reg = watch_get_backup_data(2);
}

void app_write_to_backup(void) {
    watch_store_backup_data(movement_state.settings.reg, 0);
    watch_store_backup_data(movement_state.location.reg, 1);
    watch_store_backup_data(movement_state.birthdate.reg, 2);
}

void app_setup(void) {
    static bool is_first_launch = true;

    if (is_first_launch) {
        #ifdef MOVEMENT_CUSTOM_BOOT_COMMANDS
        MOVEMENT_CUSTOM_BOOT_COMMANDS()
        #endif

        for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
            watch_face_contexts[i] = NULL;
            scheduled_tasks[i].reg = 0;
            is_first_launch = false;
        }

        watch_date_time utc_now = movement_get_utc_date_time();
        // populate the DST offset cache
        _movement_update_dst_offset_cache(utc_now);

#if __EMSCRIPTEN__
        int32_t time_zone_offset = EM_ASM_INT({
            return -new Date().getTimezoneOffset();
        });
        for (int i = 0; i < NUM_ZONE_NAMES; i++) {
            if (movement_get_current_timezone_offset_for_zone(i) == time_zone_offset * 60) {
                movement_state.settings.bit.time_zone = i;
                break;
            }
        }
#elif defined(MAKEFILE_TIMEZONE)
        for (int i = 0; i < NUM_ZONE_NAMES; i++) {
            if (movement_get_current_timezone_offset_for_zone(i) == MAKEFILE_TIMEZONE * 60) {
                movement_state.settings.bit.time_zone = i;
                break;
            }
        }
#else
        movement_state.settings.bit.time_zone = UTZ_NEW_YORK;  // Atlantic Time as default
#endif

        watch_date_time date_time = watch_utility_date_time_convert_zone(utc_now, 0, movement_get_current_timezone_offset());
        movement_state.settings.bit.is_daytime = _movement_get_if_daytime(date_time, utc_now, &movement_state.prev_sun_info);
        // set up the 1 minute alarm (for background tasks and low power updates)
        watch_date_time alarm_time;
        alarm_time.reg = 0;
        alarm_time.unit.second = 59; // after a match, the alarm fires at the next rising edge of CLK_RTC_CNT, so 59 seconds lets us update at :00
        watch_rtc_register_alarm_callback(cb_alarm_fired, alarm_time, ALARM_MATCH_SS);
    }
    if (movement_state.le_mode_ticks != -1 && movement_state.le_deep_sleeping_ticks != -1) {
        movement_update_dst_offset_cache_if_needed(movement_get_utc_date_time());  
        watch_disable_extwake_interrupt(BTN_ALARM);

        watch_enable_external_interrupts();
        watch_register_interrupt_callback(BTN_MODE, cb_mode_btn_interrupt, INTERRUPT_TRIGGER_BOTH);
        watch_register_interrupt_callback(BTN_LIGHT, cb_light_btn_interrupt, INTERRUPT_TRIGGER_BOTH);
        watch_register_interrupt_callback(BTN_ALARM, cb_alarm_btn_interrupt, INTERRUPT_TRIGGER_BOTH);

        watch_enable_buzzer();
        watch_enable_leds();
        movement_request_tick_frequency(1);
        if (_woke_up_for_buzzer) return;
        watch_enable_display();


        for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
            watch_faces[i].setup(&movement_state.settings, i, &watch_face_contexts[i]);
        }

        watch_faces[movement_state.current_face_idx].activate(&movement_state.settings, watch_face_contexts[movement_state.current_face_idx]);
        event.subsecond = 0;
        event.event_type = EVENT_ACTIVATE;
    }
}

void app_prepare_for_standby(void) {
}

void app_wake_from_standby(void) {
}

static void _sleep_mode_app_loop(void) {
    movement_state.needs_wake = false;
    movement_state.ignore_alarm_btn_after_sleep = true;
    // as long as le_mode_ticks is -1 (i.e. we are in low energy mode), we wake up here, update the screen, and go right back to sleep.
    while (movement_state.le_mode_ticks == -1) {
        // we also have to handle background tasks here in the mini-runloop
        if (movement_state.needs_background_tasks_handled) _movement_handle_background_tasks();

        event.event_type = EVENT_LOW_ENERGY_UPDATE;
        watch_faces[movement_state.current_face_idx].loop(event, &movement_state.settings, watch_face_contexts[movement_state.current_face_idx]);

        // if we need to wake immediately, do it!
        if (movement_state.needs_wake) return;
        // otherwise enter sleep mode, and when the extwake handler is called, it will reset le_mode_ticks and force us out at the next loop.
        if (movement_state.le_deep_sleeping_ticks == -1) watch_enter_deep_sleep_mode();
        else watch_enter_sleep_mode();
    }
}

bool app_loop(void) {
    const watch_face_t *wf = &watch_faces[movement_state.current_face_idx];
    if (movement_state.watch_face_changed) {
        if (movement_state.settings.bit.button_should_sound) {
            // low note for nonzero case, high note for return to watch_face 0
            watch_buzzer_play_note(movement_state.next_face_idx ? BUZZER_NOTE_C7 : BUZZER_NOTE_C8, 50);
        }
        wf->resign(&movement_state.settings, watch_face_contexts[movement_state.current_face_idx]);
        movement_state.current_face_idx = movement_state.next_face_idx;
        // we have just updated the face idx, so we must recache the watch face pointer.
        wf = &watch_faces[movement_state.current_face_idx];
        watch_clear_display();
        movement_request_tick_frequency(1);
        wf->activate(&movement_state.settings, watch_face_contexts[movement_state.current_face_idx]);
        event.subsecond = 0;
        event.event_type = EVENT_ACTIVATE;
        movement_state.watch_face_changed = false;
    }

    // if the LED should be off, turn it off
    if (movement_state.light_ticks == 0) {
        // unless the user is holding down the LIGHT button, in which case, give them more time.
        if (watch_get_pin_level(BTN_LIGHT)) {
            movement_state.light_ticks = 1;
        } else {
            watch_set_led_off();
            movement_state.light_ticks = -1;
            _movement_disable_fast_tick_if_possible();
        }
    }

    // handle background tasks, if the alarm handler told us we need to
    if (movement_state.needs_background_tasks_handled) _movement_handle_background_tasks();

    // if we have a scheduled background task, handle that here:
    if (event.event_type == EVENT_TICK && movement_state.has_scheduled_background_task) _movement_handle_scheduled_tasks();

    // if we have timed out of our low energy mode countdown, enter low energy mode.
    if (movement_state.le_mode_ticks == 0) {
        movement_state.le_mode_ticks = -1;
        watch_register_extwake_callback(BTN_ALARM, cb_alarm_btn_extwake, true);
        event.event_type = EVENT_NONE;
        event.subsecond = 0;
        _woke_up_for_buzzer = false;

        // _sleep_mode_app_loop takes over at this point and loops until le_mode_ticks is reset by the extwake handler,
        // or wake is requested using the movement_request_wake function.
        _sleep_mode_app_loop();
        // as soon as _sleep_mode_app_loop returns, we prepare to reactivate
        // ourselves, but first, we check to see if we woke up for the buzzer:
        _woke_up_for_buzzer = movement_state.is_buzzing;
        if (!_woke_up_for_buzzer) {
            event.event_type = EVENT_ACTIVATE;
        }
        _reset_debounce_ticks();  // Likely unneeded, but good to reset the debounce timers on wake.
        // this is a hack tho: waking from sleep mode, app_setup does get called, but it happens before we have reset our ticks.
        // need to figure out if there's a better heuristic for determining how we woke up.
        app_setup();
    }

    // default to being allowed to sleep by the face.
    bool can_sleep = true;

    if (event.event_type) {
        event.subsecond = movement_state.subsecond;
        // the first trip through the loop overrides the can_sleep state
        can_sleep = wf->loop(event, &movement_state.settings, watch_face_contexts[movement_state.current_face_idx]);
        event.event_type = EVENT_NONE;
    }

    // if we have timed out of our timeout countdown, give the app a hint that they can resign.
    if (movement_state.timeout_ticks == 0) {
        movement_state.timeout_ticks = -1;
        if (movement_state.settings.bit.to_always == false) {
            // if "timeout always" is false, give the current watch face a chance to exit gracefully...
            event.event_type = EVENT_TIMEOUT;
        }
        event.subsecond = movement_state.subsecond;
        // if we run through the loop again to time out, we need to reconsider whether or not we can sleep.
        // if the first trip said true, but this trip said false, we need the false to override, thus
        // we will be using boolean AND:
        //
        // first trip  | can sleep | cannot sleep | can sleep    | cannot sleep
        // second trip | can sleep | cannot sleep | cannot sleep | can sleep
        //          && | can sleep | cannot sleep | cannot sleep | cannot sleep
        bool can_sleep2 = wf->loop(event, &movement_state.settings, watch_face_contexts[movement_state.current_face_idx]);
        can_sleep = can_sleep && can_sleep2;
        event.event_type = EVENT_NONE;
        if (movement_state.settings.bit.to_always && movement_state.current_face_idx != 0) {
            // ...but if the user has "timeout always" set, give it the boot.
            movement_move_to_face(0);
        }
    }

    // Now that we've handled all display update tasks, handle the alarm.
    if (movement_state.alarm_ticks >= 0) {
        uint8_t buzzer_phase = (movement_state.alarm_ticks + 80) % 128;
        if(buzzer_phase == 127) {
            // failsafe: buzzer could have been disabled in the meantime
            if (!watch_is_buzzer_or_led_enabled()) watch_enable_buzzer();
            // play 4 beeps plus pause
            for(uint8_t i = 0; i < 4; i++) {
                // TODO: This method of playing the buzzer blocks the UI while it's beeping.
                // It might be better to time it with the fast tick.
                watch_buzzer_play_note(movement_state.alarm_note, (i != 3) ? 50 : 75);
                if (i != 3) watch_buzzer_play_note(BUZZER_NOTE_REST, 50);
            }
        }
        if (movement_state.alarm_ticks == 0) {
            movement_state.alarm_ticks = -1;
            _movement_disable_fast_tick_if_possible();
        }
    }

    // if we are plugged into USB, handle the serial shell
    if (watch_is_usb_enabled()) {
        shell_task();
    }

    event.subsecond = 0;

    // if the watch face changed, we can't sleep because we need to update the display.
    if (movement_state.watch_face_changed) can_sleep = false;

    // if we woke up for the buzzer, stay awake until it's finished.
    if (_woke_up_for_buzzer) {
        while(watch_is_buzzer_or_led_enabled());
    }

    // if the LED is on, we need to stay awake to keep the TCC running.
    if (movement_state.light_ticks != -1) can_sleep = false;
    // if we're checking debounce, stay awake
    if (!_debounce_ticks_all_zero()) can_sleep = false;

    return can_sleep;
}

static movement_event_type_t _figure_out_button_event(bool pin_level, movement_event_type_t button_down_event_type, uint16_t *down_timestamp) {
    // force alarm off if the user pressed a button.
    if (movement_state.alarm_ticks) movement_state.alarm_ticks = 0;

    if (pin_level) {
        // handle rising edge
        _movement_enable_fast_tick_if_needed();
        *down_timestamp = movement_state.fast_ticks + 1;
        return button_down_event_type;
    } else {
        // this line is hack but it handles the situation where the light button was held for more than 20 seconds.
        // fast tick is disabled by then, and the LED would get stuck on since there's no one left decrementing light_ticks.
        if (movement_state.light_ticks == 1) movement_state.light_ticks = 0;
        // now that that's out of the way, handle falling edge
        uint16_t diff = movement_state.fast_ticks - *down_timestamp;
        *down_timestamp = 0;
        // any press over a half second is considered a long press. Fire the long-up event
        if (diff > MOVEMENT_LONG_PRESS_TICKS) return button_down_event_type + 3;
        else return button_down_event_type + 1;
    }
}

static movement_event_type_t btn_action(bool pin_level, int code, uint16_t *timestamp) {
    _movement_reset_inactivity_countdown(); 
    return _figure_out_button_event(pin_level, code, timestamp);
}

static void light_btn_action(bool pin_level) {
    event.event_type = btn_action(pin_level, EVENT_LIGHT_BUTTON_DOWN, &movement_state.light_down_timestamp);
}

static void mode_btn_action(bool pin_level) {
    event.event_type = btn_action(pin_level, EVENT_MODE_BUTTON_DOWN, &movement_state.mode_down_timestamp);
}

static void alarm_btn_action(bool pin_level) {
    uint8_t event_type = btn_action(pin_level, EVENT_ALARM_BUTTON_DOWN, &movement_state.alarm_down_timestamp);
    if  (movement_state.ignore_alarm_btn_after_sleep){
        if (event_type == EVENT_ALARM_BUTTON_UP || event_type == EVENT_ALARM_LONG_UP) movement_state.ignore_alarm_btn_after_sleep = false;
        return;
    }
    event.event_type = event_type;
}

#if (DEBOUNCE_TICKS_DOWN || DEBOUNCE_TICKS_UP)
static void debounce_btn_press(uint8_t pin, uint8_t *debounce_ticks, uint16_t *down_timestamp, void (*function)(bool)) {
    if (*debounce_ticks == 0) {
        bool pin_level = watch_get_pin_level(pin);
        function(pin_level);
        _reset_debounce_ticks();  // Avoids adding the debounce time of one btn into this one becoming a long press
        *debounce_ticks = pin_level ? DEBOUNCE_TICKS_DOWN : DEBOUNCE_TICKS_UP;
        if (*debounce_ticks != 0) _movement_enable_fast_tick_if_needed();
    }
    else
        *down_timestamp = 0;
}

static void disable_if_needed(uint8_t *ticks) {
    if (*ticks > 0 && --*ticks == 0)
        _movement_disable_fast_tick_if_possible();
}

static void movement_disable_if_debounce_complete(void) {
    disable_if_needed(&movement_state.debounce_ticks_light);
    disable_if_needed(&movement_state.debounce_ticks_alarm);
    disable_if_needed(&movement_state.debounce_ticks_mode);
}
#endif

void cb_light_btn_interrupt(void) {
#if (DEBOUNCE_TICKS_DOWN || DEBOUNCE_TICKS_UP)
    debounce_btn_press(BTN_LIGHT, &movement_state.debounce_ticks_light, &movement_state.light_down_timestamp, light_btn_action);
#else
    bool pin_level = watch_get_pin_level(BTN_LIGHT);
    light_btn_action(pin_level);
#endif
}

void cb_mode_btn_interrupt(void) {
#if (DEBOUNCE_TICKS_DOWN || DEBOUNCE_TICKS_UP)
    debounce_btn_press(BTN_MODE, &movement_state.debounce_ticks_mode, &movement_state.mode_down_timestamp, mode_btn_action);
#else
    bool pin_level = watch_get_pin_level(BTN_MODE);
    mode_btn_action(pin_level);
#endif
}

void cb_alarm_btn_interrupt(void) {
#if (DEBOUNCE_TICKS_DOWN || DEBOUNCE_TICKS_UP)
    debounce_btn_press(BTN_ALARM, &movement_state.debounce_ticks_alarm, &movement_state.alarm_down_timestamp, alarm_btn_action);
#else
    bool pin_level = watch_get_pin_level(BTN_ALARM);
    alarm_btn_action(pin_level);
#endif
}

void cb_alarm_btn_extwake(void) {
    // wake up!
    _movement_reset_inactivity_countdown();
}

void cb_alarm_fired(void) {
    movement_state.needs_background_tasks_handled = true;
}

void cb_fast_tick(void) {
#if (DEBOUNCE_TICKS_DOWN || DEBOUNCE_TICKS_UP)
    movement_disable_if_debounce_complete();
    if (_debounce_ticks_all_zero())
        movement_state.fast_ticks++;
#else
    movement_state.fast_ticks++;
#endif
    if (movement_state.light_ticks > 0) movement_state.light_ticks--;
    if (movement_state.alarm_ticks > 0) movement_state.alarm_ticks--;
    // check timestamps and auto-fire the long-press events
    // Notice: is it possible that two or more buttons have an identical timestamp? In this case
    // only one of these buttons would receive the long press event. Don't bother for now...
    if (movement_state.light_down_timestamp > 0)
        if (movement_state.fast_ticks - movement_state.light_down_timestamp == MOVEMENT_LONG_PRESS_TICKS + 1)
            event.event_type = EVENT_LIGHT_LONG_PRESS;
    if (movement_state.mode_down_timestamp > 0)
        if (movement_state.fast_ticks - movement_state.mode_down_timestamp == MOVEMENT_LONG_PRESS_TICKS + 1)
            event.event_type = EVENT_MODE_LONG_PRESS;
    if (movement_state.alarm_down_timestamp > 0){
        if (movement_state.fast_ticks - movement_state.alarm_down_timestamp == (3 * MOVEMENT_LONG_PRESS_TICKS) + 1)
            event.event_type = EVENT_ALARM_LONGER_PRESS;
        else if (movement_state.fast_ticks - movement_state.alarm_down_timestamp == MOVEMENT_LONG_PRESS_TICKS + 1)
            event.event_type = EVENT_ALARM_LONG_PRESS;
    }
    // this is just a fail-safe; fast tick should be disabled as soon as the button is up, the LED times out, and/or the alarm finishes.
    // but if for whatever reason it isn't, this forces the fast tick off after 20 seconds.
    if (movement_state.fast_ticks >= 128 * 20) {
        watch_rtc_disable_periodic_callback(128);
        movement_state.fast_tick_enabled = false;
        _reset_debounce_ticks();
    }
}

void cb_tick(void) {
    if (!_woke_up_for_buzzer) event.event_type = EVENT_TICK;
    watch_date_time date_time = movement_get_utc_date_time();
    if (date_time.unit.second != movement_state.last_second) {
        // TODO: can we consolidate these two ticks?
        if (g_force_sleep){
            switch (g_force_sleep)
            {
            case 2:
                movement_state.le_deep_sleeping_ticks = -1;
                // fall through
            case 1:
                movement_state.le_mode_ticks = 0;
                g_force_sleep = 0;
                break;
            }
        }
        else if (movement_state.settings.bit.le_interval && movement_state.le_mode_ticks > 0) movement_state.le_mode_ticks--;
        if (movement_state.timeout_ticks > 0) movement_state.timeout_ticks--;

        movement_state.last_second = date_time.unit.second;
        movement_state.subsecond = 0;
    } else {
        movement_state.subsecond++;
    }
}
