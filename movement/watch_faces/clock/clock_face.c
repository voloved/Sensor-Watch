/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 David Keck <davidskeck@users.noreply.github.com>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Jeremy O'Brien <neutral@fastmail.com>
 * Copyright © 2023 Mikhail Svarichevsky <3@14.by>
 * Copyright © 2023 Wesley Aptekar-Cassels <me@wesleyac.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
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

#include <stdlib.h>
#include "clock_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_private_display.h"
#include "sunriset.h"

// 2.2 volts will happen when the battery has maybe 5-10% remaining?
// we can refine this later.
#ifndef CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD
#define CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD 2200
#endif

typedef struct {
    struct {
        watch_date_time previous;
    } date_time;
    uint8_t last_battery_check;
    uint8_t watch_face_index;
    bool time_signal_enabled;
    bool battery_low;
    bool showing_logo;
} clock_state_t;

static uint8_t _time_to_chime_hour(double time, double hours_from_utc, bool use_end_of_hour) {
    time += hours_from_utc;
    uint8_t hour_to_start = (uint8_t)time;
    double minutes = (time - hour_to_start) * 60;
    if (!use_end_of_hour) return hour_to_start;
    if (minutes >= 0.5)
        hour_to_start = (hour_to_start + 1) % 24;
    return hour_to_start;
}

static void _get_chime_times(watch_date_time date_time, movement_settings_t *settings, uint8_t *start_hour, uint8_t *end_hour) {
    uint8_t init_val = 0xFF;
    uint8_t hourly_chime_start = settings->bit.hourly_chime_start;
    uint8_t hourly_chime_end = settings->bit.hourly_chime_end;
    *start_hour = (hourly_chime_start == 3) ? init_val : Hourly_Chime_Start[hourly_chime_start];
    *end_hour = (hourly_chime_end == 3) ? init_val : Hourly_Chime_End[hourly_chime_end];
    if (hourly_chime_start != 3 && hourly_chime_end != 3) {
        return;
    }
    int32_t tz = movement_get_current_timezone_offset();
    watch_date_time utc_now = watch_utility_date_time_convert_zone(date_time, tz, 0); // the current date / time in UTC
    movement_location_t movement_location = (movement_location_t) watch_get_backup_data(1);
    if (movement_location.reg == 0) {
        return;
    }
    double rise, set;
    uint8_t rise_hour, set_hour;
    int16_t lat_centi = (int16_t)movement_location.bit.latitude;
    int16_t lon_centi = (int16_t)movement_location.bit.longitude;
    double lat = (double)lat_centi / 100.0;
    double lon = (double)lon_centi / 100.0;
    double hours_from_utc = ((double)tz) / 3600.0;
    uint8_t result = sun_rise_set(utc_now.unit.year + WATCH_RTC_REFERENCE_YEAR, utc_now.unit.month, utc_now.unit.day, lon, lat, &rise, &set);
    if (result != 0) {
        return;
    }
    rise_hour = _time_to_chime_hour(rise, hours_from_utc, true);
    set_hour = _time_to_chime_hour(set, hours_from_utc, false);
    if (*start_hour == init_val) *start_hour = rise_hour;
    if (*end_hour == init_val) *end_hour = set_hour;
    if (*start_hour == 0) *start_hour = 24;
    if (*end_hour == 0) *end_hour = 24;
}

static bool clock_is_in_24h_mode(movement_settings_t *settings) {
#ifdef CLOCK_FACE_24H_ONLY
    return true;
#else
    return settings->bit.clock_mode_24h;
#endif
}

static void clock_indicate(WatchIndicatorSegment indicator, bool on) {
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

static void clock_indicate_alarm(movement_settings_t *settings) {
    clock_indicate(WATCH_INDICATOR_SIGNAL, settings->bit.alarm_enabled);
}

static void clock_indicate_time_signal(clock_state_t *clock) {
    clock_indicate(WATCH_INDICATOR_BELL, clock->time_signal_enabled);
}

static void clock_indicate_24h(movement_settings_t *settings) {
    clock_indicate(WATCH_INDICATOR_24H, clock_is_in_24h_mode(settings));
}

static bool clock_is_pm(watch_date_time date_time) {
    return date_time.unit.hour >= 12;
}

static void clock_indicate_pm(movement_settings_t *settings, watch_date_time date_time) {
    if (settings->bit.clock_mode_24h) { return; }
    clock_indicate(WATCH_INDICATOR_PM, clock_is_pm(date_time));
}

static void clock_indicate_low_available_power(clock_state_t *clock) {
    // Set the LAP indicator if battery power is low
    clock_indicate(WATCH_INDICATOR_LAP, clock->battery_low);
}

static watch_date_time clock_24h_to_12h(watch_date_time date_time) {
    date_time.unit.hour %= 12;

    if (date_time.unit.hour == 0) {
        date_time.unit.hour = 12;
    }

    return date_time;
}

static void clock_toggle_24h_mode(movement_settings_t *settings, watch_date_time current) {
    char buf[2 + 1];
    settings->bit.clock_mode_24h = !settings->bit.clock_mode_24h;
    if (clock_is_in_24h_mode(settings)) {
        clock_indicate(WATCH_INDICATOR_PM, false);
    }
    else {
        clock_indicate_pm(settings, current);
        current = clock_24h_to_12h(current);
    }
    clock_indicate_24h(settings);
    sprintf(buf, "%2d", current.unit.hour);
    watch_display_string(buf, 4);
}

static void clock_check_battery_periodically(clock_state_t *clock, watch_date_time date_time) {
    // check the battery voltage once a day
    if (date_time.unit.day == clock->last_battery_check) { return; }

    clock->last_battery_check = date_time.unit.day;

    watch_enable_adc();
    uint16_t voltage = watch_get_vcc_voltage();
    watch_disable_adc();

    clock->battery_low = voltage < CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD;

    clock_indicate_low_available_power(clock);
}

static void clock_toggle_time_signal(clock_state_t *clock) {
    clock->time_signal_enabled = !clock->time_signal_enabled;
    clock_indicate_time_signal(clock);
}

static void clock_display_all(watch_date_time date_time) {
    char buf[10 + 1];

    snprintf(
        buf,
        sizeof(buf),
        "%s%2d%2d%02d%02d",
        watch_utility_get_weekday(date_time),
        date_time.unit.day,
        date_time.unit.hour,
        date_time.unit.minute,
        date_time.unit.second
    );

    watch_display_string(buf, 0);
}

static bool clock_display_some(watch_date_time current, watch_date_time previous) {
    if ((current.reg >> 6) == (previous.reg >> 6)) {
        // everything before seconds is the same, don't waste cycles setting those segments.

        watch_display_character_lp_seconds('0' + current.unit.second / 10, 8);
        watch_display_character_lp_seconds('0' + current.unit.second % 10, 9);

        return true;

    } else if ((current.reg >> 12) == (previous.reg >> 12)) {
        // everything before minutes is the same.

        char buf[4 + 1];

        snprintf(
            buf,
            sizeof(buf),
            "%02d%02d",
            current.unit.minute,
            current.unit.second
        );

        watch_display_string(buf, 6);

        return true;

    } else {
        // other stuff changed; let's do it all.
        return false;
    }
}

static void clock_display_clock(movement_settings_t *settings, watch_date_time current) {
    if (!clock_is_in_24h_mode(settings)) {
        // if we are in 12 hour mode, do some cleanup.
        clock_indicate_pm(settings, current);
        current = clock_24h_to_12h(current);
    }
    clock_display_all(current);
}

static void clock_display_logo(clock_state_t *clock) {
    clock->showing_logo = true;
    clock->time_signal_enabled = !clock->time_signal_enabled;
    watch_clear_all_indicators();
    watch_clear_colon();
    watch_display_string("    CASIO ", 0);
}

static void clock_stop_logo(movement_settings_t *settings, clock_state_t *clock, watch_date_time current)  {
    clock->showing_logo = false;

    clock_indicate_time_signal(clock);
    clock_indicate_alarm(settings);
    clock_indicate_24h(settings);
    watch_set_colon();

    clock_display_clock(settings, current);
    clock->date_time.previous = current;
}

static void clock_display_low_energy(watch_date_time date_time) {
    char buf[10 + 1];

    snprintf(
        buf,
        sizeof(buf),
        "%s%2d%2d%02d  ",
        watch_utility_get_weekday(date_time),
        date_time.unit.day,
        date_time.unit.hour,
        date_time.unit.minute
    );

    watch_display_string(buf, 0);
}

static void clock_stop_tick_tock_animation(void) {
    if (watch_tick_animation_is_running()) {
        watch_stop_tick_animation();
    }
}

void clock_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(clock_state_t));
        clock_state_t *state = (clock_state_t *) *context_ptr;
        state->time_signal_enabled = false;
        state->watch_face_index = watch_face_index;
    }
}

void clock_face_activate(movement_settings_t *settings, void *context) {
    clock_state_t *clock = (clock_state_t *) context;

    clock_stop_tick_tock_animation();

    clock_indicate_time_signal(clock);
    clock_indicate_alarm(settings);
    clock_indicate_24h(settings);

    watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    clock->date_time.previous.reg = 0xFFFFFFFF;
}

bool clock_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    clock_state_t *state = (clock_state_t *) context;
    watch_date_time current;

    if (state->showing_logo){
        if (!watch_get_pin_level(BTN_ALARM)){
            current = movement_get_local_date_time();
            clock_stop_logo(settings, state, current);
        }
        return true;
    }

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
            clock_display_low_energy(movement_get_local_date_time());
            break;
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            current = movement_get_local_date_time();
            if (!clock_display_some(current, state->date_time.previous)) {
                clock_display_clock(settings, current);
            }
            current = movement_get_local_date_time();

            clock_display_clock(settings, current);

            clock_check_battery_periodically(state, current);

            state->date_time.previous = current;

            break;
        case EVENT_ALARM_LONG_PRESS:
            clock_toggle_time_signal(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (settings->bit.clock_mode_toggle) {
                current = movement_get_local_date_time();
                clock_toggle_24h_mode(settings, current);
                state->date_time.previous = current;
            }
            break;
        case EVENT_ALARM_LONGER_PRESS:
            clock_display_logo(state);
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the clock face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            movement_play_signal(SIGNAL_TUNE_DEFAULT);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void clock_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool clock_face_wants_background_task(movement_settings_t *settings, void *context) {
    (void) settings;
    clock_state_t *state = (clock_state_t *) context;
    if (!state->time_signal_enabled) return false;

    watch_date_time date_time = movement_get_local_date_time();
    if (date_time.unit.minute != 0) return false;
    if (settings->bit.hourly_chime_always) return true;
    uint8_t chime_start, chime_end;
    _get_chime_times(date_time, settings, &chime_start, &chime_end);
    if ((24 >= chime_start && date_time.unit.hour < chime_start) || (24 >= chime_end && date_time.unit.hour >= chime_end)) return false;

    return true;
}