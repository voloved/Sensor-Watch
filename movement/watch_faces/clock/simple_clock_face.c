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

static void _update_alarm_indicator(bool settings_alarm_enabled, simple_clock_state_t *state) {
    state->alarm_enabled = settings_alarm_enabled;
    if (state->alarm_enabled) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
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

    if (settings->bit.clock_mode_24h) watch_set_indicator(WATCH_INDICATOR_24H);

    // handle chime indicator
    if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_BELL);
    else watch_clear_indicator(WATCH_INDICATOR_BELL);

    // show alarm indicator if there is an active alarm
    _update_alarm_indicator(settings->bit.alarm_enabled, state);

    watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->previous_date_time = 0xFFFFFFFF;
    state->showingLogo = false;
}

bool simple_clock_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    simple_clock_state_t *state = (simple_clock_state_t *)context;
    char buf[11];
    uint8_t pos;

    watch_date_time date_time;
    uint32_t previous_date_time;

    if (state->showingLogo){
        if (!watch_get_pin_level(BTN_LIGHT)){
            state->showingLogo = false;
            state->signal_enabled = !state->signal_enabled;
            date_time = watch_rtc_get_date_time();
            state->previous_date_time = date_time.reg;
            if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_BELL);
            if (settings->bit.clock_mode_24h) watch_set_indicator(WATCH_INDICATOR_24H);
            else {
                if (date_time.unit.hour >= 12) watch_set_indicator(WATCH_INDICATOR_PM);
                date_time.unit.hour %= 12;
                if (date_time.unit.hour == 0) date_time.unit.hour = 12;
            }
            watch_set_colon();
            sprintf(buf, "%s%2d%2d%02d%02d", watch_utility_get_weekday(date_time), date_time.unit.day, date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
            watch_display_string(buf, 0);
        }
        return true;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            date_time = watch_rtc_get_date_time();
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
                state->battery_low = (voltage < 2200);
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
            break;
        case EVENT_ALARM_LONGER_PRESS:
            state->showingLogo = true;
            watch_clear_all_indicators();
            watch_clear_colon();
            watch_display_string("     CHUFF", 0);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->signal_enabled = !state->signal_enabled;
            if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_BELL);
            else watch_clear_indicator(WATCH_INDICATOR_BELL);
            break; 
        case EVENT_ALARM_BUTTON_UP:
            settings->bit.clock_mode_24h = !settings->bit.clock_mode_24h;
            if (settings->bit.clock_mode_24h) watch_set_indicator(WATCH_INDICATOR_24H);
            else watch_clear_indicator(WATCH_INDICATOR_24H);
            date_time = watch_rtc_get_date_time();
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
            else watch_clear_indicator(WATCH_INDICATOR_PM);
            sprintf(buf, "%2d", date_time.unit.hour);
            watch_display_string(buf, 4);
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the clock face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            movement_play_signal();
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
    (void) settings;
    simple_clock_state_t *state = (simple_clock_state_t *)context;
    if (!state->signal_enabled) return false;

    watch_date_time date_time = watch_rtc_get_date_time();
    uint8_t chime_start = Hourly_Chime_Start[settings->bit.hourly_chime_start];
    uint8_t chime_end = Hourly_Chime_End[settings->bit.hourly_chime_end];
    if (chime_end == 0) chime_end = 24;
    if (!settings->bit.hourly_chime_always && (date_time.unit.hour < chime_start || date_time.unit.hour >= chime_end)) return false;

    return date_time.unit.minute == 0;
}
