/*
 * MIT License
 *
 * Copyright (c) 2023 Andreas Nebinger, based on Joey Castillo's simple clock face
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
#include <string.h>
#include "simple_clock_bin_led_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_private_display.h"
#include "sunriset.h"

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
    int16_t tz = movement_get_current_timezone_offset();
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

static void _update_alarm_indicator(bool settings_alarm_enabled, simple_clock_bin_led_state_t *state) {
    state->alarm_enabled = settings_alarm_enabled;
    if (state->alarm_enabled) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
}

static void _display_left_aligned(uint8_t value) {
    if (value >= 10) {
        watch_display_character('0' + value / 10, 4);
        watch_display_character('0' + value % 10, 5);
    } else {
        watch_display_character('0' + value, 4);
        watch_display_character(' ', 5);
    }
}

void simple_clock_bin_led_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(simple_clock_bin_led_state_t));
        memset(*context_ptr, 0, sizeof(simple_clock_bin_led_state_t));
        simple_clock_bin_led_state_t *state = (simple_clock_bin_led_state_t *)*context_ptr;
        state->watch_face_index = watch_face_index;
    }
}

void simple_clock_bin_led_face_activate(movement_settings_t *settings, void *context) {
    simple_clock_bin_led_state_t *state = (simple_clock_bin_led_state_t *)context;

    if (watch_tick_animation_is_running()) watch_stop_tick_animation();

    if (settings->bit.clock_mode_24h) watch_set_indicator(WATCH_INDICATOR_24H);

    // handle chime indicator
    if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_BELL);
    else watch_clear_indicator(WATCH_INDICATOR_BELL);

    // show alarm indicator if there is an active alarm
    _update_alarm_indicator(settings->bit.alarm_enabled, state);

    watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->previous_date_time = 0xFFFFFFFF;
}

bool simple_clock_bin_led_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    simple_clock_bin_led_state_t *state = (simple_clock_bin_led_state_t *)context;
    char buf[11];
    uint8_t pos;

    watch_date_time date_time;
    uint32_t previous_date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            date_time = watch_rtc_get_date_time();
            if (state->flashing_state > 0) {
                if (state->ticks) {
                    state->ticks--;
                } else {
                    
                    if (state->flashing_state & 64) {
                        // start led on for current bit
                        state->flashing_state &= 63;
                        state->ticks = (state->flashing_value & 1 ? 7 : 1);
                        movement_illuminate_led();
                    } else {
                        // indicate first or switch to next bit
                        watch_set_led_off();
                        if ((state->flashing_state & 128) == 0) state->flashing_value = state->flashing_value >> 1;
                        if (state->flashing_value || (state->flashing_state & 128)) {
                            state->flashing_state &= 127;
                            state->flashing_state |= 64;
                            state->ticks = 6;
                        } else if (state->flashing_state & 1) {
                            // transition to minutes
                            state->flashing_state = 2 + 128;
                            state->flashing_value = date_time.unit.minute;
                            _display_left_aligned(state->flashing_value);
                            state->ticks = 9;
                        } else {
                            // end flashing
                            state->flashing_state = 0;
                            state->previous_date_time = 0xFFFFFFFF;
                            movement_request_tick_frequency(1);
                            watch_set_colon();
                        }
                    }
                }
            } else {
                previous_date_time = state->previous_date_time;
                state->previous_date_time = date_time.reg;

                // check the battery voltage once a day...
                if (date_time.unit.day != state->last_battery_check) {
                    state->last_battery_check = date_time.unit.day;
                    watch_enable_adc();
                    uint16_t voltage = watch_get_vcc_voltage();
                    watch_disable_adc();
                    // 2.2 volts will happen when the battery has maybe 5-10% remaining?
                    // we can refine this later.
                    state->battery_low = (voltage < 2400);
                }

                // ...and set the LAP indicator if low.
                if (state->battery_low) watch_set_indicator(WATCH_INDICATOR_LAP);

                if ((date_time.reg >> 6) == (previous_date_time >> 6) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                    // everything before seconds is the same, don't waste cycles setting those segments.
                    watch_display_character_lp_seconds('0' + date_time.unit.second / 10, 8);
                    watch_display_character_lp_seconds('0' + date_time.unit.second % 10, 9);
                    break;
                } else if ((date_time.reg >> 12) == (previous_date_time >> 12) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                    // everything before minutes is the same.
                    pos = 6;
                    sprintf(buf, "%02d%02d", date_time.unit.minute, date_time.unit.second);
                } else {
                    // other stuff changed; let's do it all.
                    if (!settings->bit.clock_mode_24h) {
                        // if we are in 12 hour mode, do some cleanup.
                        if (date_time.unit.hour < 12) {
                            watch_clear_indicator(WATCH_INDICATOR_PM);
                        } else {
                            watch_set_indicator(WATCH_INDICATOR_PM);
                        }
                        date_time.unit.hour %= 12;
                        if (date_time.unit.hour == 0) date_time.unit.hour = 12;
                    }
                    pos = 0;
                    if (event.event_type == EVENT_LOW_ENERGY_UPDATE) {
                        sprintf(buf, "%s%2d%2d%02d  ", watch_utility_get_weekday(date_time), date_time.unit.day, date_time.unit.hour, date_time.unit.minute);
                    } else {
                        sprintf(buf, "%s%2d%2d%02d%02d", watch_utility_get_weekday(date_time), date_time.unit.day, date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
                    }
                }
                watch_display_string(buf, pos);
                // handle alarm indicator
                if (state->alarm_enabled != settings->bit.alarm_enabled) _update_alarm_indicator(settings->bit.alarm_enabled, state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->signal_enabled = !state->signal_enabled;
            if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_BELL);
            else watch_clear_indicator(WATCH_INDICATOR_BELL);
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the clock face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            movement_play_signal(SIGNAL_TUNE_DEFAULT);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (state->flashing_state == 0) {
                date_time = watch_rtc_get_date_time();
                state->flashing_state = 1 + 128;
                state->ticks = 4;
                if (!settings->bit.clock_mode_24h) {
                    date_time.unit.hour %= 12;
                    if (date_time.unit.hour == 0) date_time.unit.hour = 12;
                }
                watch_display_string("      ", 4);
                _display_left_aligned(date_time.unit.hour);
                state->flashing_value = date_time.unit.hour > 12 ? date_time.unit.hour - 12 : date_time.unit.hour;
                watch_set_led_off();
                watch_clear_colon();
                movement_request_tick_frequency(8);
            }
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void simple_clock_bin_led_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool simple_clock_bin_led_face_wants_background_task(movement_settings_t *settings, void *context) {
    (void) settings;
    simple_clock_bin_led_state_t *state = (simple_clock_bin_led_state_t *)context;
    if (!state->signal_enabled) return false;

    watch_date_time date_time = movement_get_local_date_time();
    if (date_time.unit.minute != 0) return false;
    if (settings->bit.hourly_chime_always) return true;
    uint8_t chime_start, chime_end;
    _get_chime_times(date_time, settings, &chime_start, &chime_end);
    if ((24 >= chime_start && date_time.unit.hour < chime_start) || (24 >= chime_end && date_time.unit.hour >= chime_end)) return false;

    return true;
}
