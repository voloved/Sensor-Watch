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

#include <stdlib.h>
#include "simple_clock_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_private_display.h"

static watch_date_time date_time;

static void _update_alarm_indicator(bool settings_alarm_enabled, simple_clock_state_t *state) {
    state->alarm_enabled = settings_alarm_enabled;
    if (state->alarm_enabled) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
}

static void _update_face_low_energy(watch_date_time date_time, watch_date_time previous_date_time, char *buf) {
    if (date_time.unit.day != previous_date_time.unit.day) {
        sprintf(buf, "%s%2d%2d%02d  ", watch_utility_get_weekday(date_time), date_time.unit.day, date_time.unit.hour, date_time.unit.minute);
        watch_display_string(buf, 0);
    }
    else if (date_time.unit.hour != previous_date_time.unit.hour) {
        sprintf(buf, "%2d%02d  ", date_time.unit.hour, date_time.unit.minute);
        watch_display_string(buf, 4);
    }
    // If both digits of the minute need updating
    else if ((date_time.unit.minute / 10) != (previous_date_time.unit.minute / 10)) {
        sprintf( buf, "%02d  ", date_time.unit.minute);
        watch_display_string(buf, 6);
    }
    // If only the ones-place of the minute needs updating.
    else if (date_time.unit.minute != previous_date_time.unit.minute) {
        sprintf( buf, "%d  ", date_time.unit.minute % 10);
        watch_display_string(buf, 7);
    }
}

void simple_clock_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(simple_clock_state_t));
        simple_clock_state_t *state = (simple_clock_state_t *)*context_ptr;
        state->signal_enabled = true;
        state->watch_face_index = watch_face_index;
    }
}

void simple_clock_face_activate(movement_settings_t *settings, void *context) {
    simple_clock_state_t *state = (simple_clock_state_t *)context;

    if (watch_tick_animation_is_running()) watch_stop_tick_animation();

#ifdef CLOCK_FACE_24H_ONLY
    watch_set_indicator(WATCH_INDICATOR_24H);
#else
    if (settings->bit.clock_mode_24h) watch_set_indicator(WATCH_INDICATOR_24H);
#endif

    // handle chime indicator
    if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_BELL);
    else watch_clear_indicator(WATCH_INDICATOR_BELL);

    // show alarm indicator if there is an active alarm
    _update_alarm_indicator(settings->bit.alarm_enabled, state);

    watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->previous_date_time.reg = 0xFFFFFFFF;
    state->showing_logo = false;
    state->lat_long_set = watch_get_backup_data(1) != 0;
    state->birth_date.reg = watch_get_backup_data(2);
}

bool simple_clock_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    simple_clock_state_t *state = (simple_clock_state_t *)context;
    char buf[11];
    uint8_t pos;

    watch_date_time previous_date_time;

    if (state->showing_logo){
        if (!watch_get_pin_level(BTN_ALARM)){
            state->showing_logo = false;
            go_to_teriary_face();
        }
        return true;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            date_time = movement_get_local_date_time();
            previous_date_time = state->previous_date_time;
            state->previous_date_time.reg = date_time.reg;

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

            if ((date_time.reg >> 6) == (previous_date_time.reg >> 6) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                // everything before seconds is the same, don't waste cycles setting those segments.
                if (date_time.unit.second / 10 != previous_date_time.unit.second / 10)
                    watch_display_character_lp_seconds('0' + date_time.unit.second / 10, 8);
                watch_display_character_lp_seconds('0' + date_time.unit.second % 10, 9);
                break;
            } else if ((date_time.reg >> 12) == (previous_date_time.reg >> 12) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                // everything before minutes is the same.
                if (date_time.unit.minute / 10 != previous_date_time.unit.minute / 10) {
                    pos = 6;
                    sprintf(buf, "%02d%02d", date_time.unit.minute, date_time.unit.second);
                } else {
                     pos = 7;
                    sprintf(buf, "%1d%02d", date_time.unit.minute % 10, date_time.unit.second);                   
                }
            } else {
                // other stuff changed; let's do it all.
#ifndef CLOCK_FACE_24H_ONLY
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
#endif
                pos = 0;
                if (event.event_type == EVENT_LOW_ENERGY_UPDATE) {
                    _update_face_low_energy(date_time, previous_date_time, buf);
                    break;
                } else {
                    sprintf(buf, "%s%2d%2d%02d%02d", watch_utility_get_weekday(date_time), date_time.unit.day, date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
                }
            }
            watch_display_string(buf, pos);
            // handle alarm indicator
            if (state->alarm_enabled != settings->bit.alarm_enabled) _update_alarm_indicator(settings->bit.alarm_enabled, state);
            break;
        case EVENT_ALARM_LONGER_PRESS:
            state->showing_logo = true;
            state->signal_enabled = !state->signal_enabled;
            watch_clear_all_indicators();
            watch_clear_colon();
            watch_display_string("    CASIO ", 0);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->signal_enabled = !state->signal_enabled;
            if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_BELL);
            else watch_clear_indicator(WATCH_INDICATOR_BELL);
            break; 
        case EVENT_ALARM_BUTTON_UP:
            if (settings->bit.clock_mode_toggle) {
                settings->bit.clock_mode_24h = !settings->bit.clock_mode_24h;
                date_time = movement_get_local_date_time();
                if (settings->bit.clock_mode_24h) {
                    watch_set_indicator(WATCH_INDICATOR_24H);
                    watch_clear_indicator(WATCH_INDICATOR_PM);
                }
                else {
                    watch_clear_indicator(WATCH_INDICATOR_24H);
                    if (date_time.unit.hour >= 12) watch_set_indicator(WATCH_INDICATOR_PM);
                    date_time.unit.hour %= 12;
                    if (date_time.unit.hour == 0) date_time.unit.hour = 12;
                }
                sprintf(buf, "%2d", date_time.unit.hour);
                watch_display_string(buf, 4);
            }
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the clock face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            if (date_time.unit.month == state->birth_date.bit.month && date_time.unit.day == state->birth_date.bit.day)
                movement_play_signal(SIGNAL_TUNE_HAPPY_BIRTHDAY);
            else
                movement_play_signal(SIGNAL_TUNE_DEFAULT);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void simple_clock_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool simple_clock_face_wants_background_task(movement_settings_t *settings, void *context) {
    simple_clock_state_t *state = (simple_clock_state_t *)context;
    if (!state->signal_enabled) return false;

    date_time = movement_get_local_date_time();
    if (date_time.unit.minute != 0) return false;
    if (settings->bit.hourly_chime_always) return true;

    bool use_chime_start_time = date_time.unit.hour < Hourly_Chime_Middle;
    if (((use_chime_start_time && settings->bit.hourly_chime_start == 3) ||
        (!use_chime_start_time && settings->bit.hourly_chime_end == 3)) &&
        state->lat_long_set) {
        return settings->bit.is_daytime;
    }
    return use_chime_start_time
        ? (date_time.unit.hour >= Hourly_Chime_Start[settings->bit.hourly_chime_start])
        : (date_time.unit.hour < Hourly_Chime_End[settings->bit.hourly_chime_end]);
}
