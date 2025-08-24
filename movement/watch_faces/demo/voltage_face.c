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
#include <string.h>
#include "voltage_face.h"
#include "watch.h"

static bool _alarm_held;
static bool _displaying_curr;

static void _voltage_face_update_display(void) {
    char buf[14];
    _displaying_curr = true;

    watch_enable_adc();
    float voltage = (float)watch_get_vcc_voltage() / 1000.0;
    watch_disable_adc();

    sprintf(buf, "BA  %4.2f V", voltage);
    watch_display_string(buf, 0);
}

static void _voltage_face_blink_display(void) {
    watch_date_time date_time = movement_get_utc_date_time();  // Only cares about the seconds.
    if (date_time.unit.second % 5 == 0 || !_displaying_curr) {
        _voltage_face_update_display();
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }
    else if (date_time.unit.second % 5 == 4) {
        // Not 100% on this, but I like the idea of using the signal indicator to indicate that we're sensing data.
        // In this case we turn the indicator on a second before the reading is taken, and clear it when we're done.
        // In reality the measurement takes a fraction of a second, but this is just to show something is happening.
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
        return;
    }
}

static void _voltage_face_log_data(voltage_face_state_t *logger_state) {
    watch_enable_adc();
    watch_date_time date_time = movement_get_local_date_time();
    size_t pos = logger_state->data_points % VOLTAGE_LOGGING_CYC;

    logger_state->data[pos].timestamp.reg = date_time.reg;
    logger_state->data[pos].voltage = (float)watch_get_vcc_voltage() / 1000.0;
    logger_state->data_points++;

    watch_disable_adc();
}

static void _voltage_face_log_update_display(voltage_face_state_t *logger_state, bool clock_mode_24h) {
    int8_t pos = (logger_state->data_points - 1 - logger_state->display_index) % VOLTAGE_NUM_DATA_POINTS;
    char buf[14];

    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_colon();

    if (logger_state->display_index == VOLTAGE_NUM_DATA_POINTS){
        _voltage_face_blink_display();
        return;
    }
    if (_displaying_curr){
        _displaying_curr = false;
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }
    if (pos < 0) {
        sprintf(buf, "BA%2dno dat", logger_state->display_index);
    } else if (logger_state->ts_ticks) {
        watch_date_time date_time = logger_state->data[pos].timestamp;
        watch_set_colon();
        if (clock_mode_24h) {
            watch_set_indicator(WATCH_INDICATOR_24H);
        } else {
            if (date_time.unit.hour > 11) watch_set_indicator(WATCH_INDICATOR_PM);
            date_time.unit.hour %= 12;
            if (date_time.unit.hour == 0) date_time.unit.hour = 12;
        }
        sprintf(buf, "AT%2d%2d%02d%02d", date_time.unit.day, date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
    } else {
        sprintf(buf, "BA%2d%4.2f V", logger_state->display_index, logger_state->data[pos].voltage);
        watch_display_string(buf, 0);
    }

    watch_display_string(buf, 0);
}

void voltage_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(voltage_face_state_t));
        memset(*context_ptr, 0, sizeof(voltage_face_state_t));
    }
}

void voltage_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    voltage_face_state_t *logger_state = (voltage_face_state_t *)context;
    logger_state->display_index = VOLTAGE_NUM_DATA_POINTS;
    logger_state->ts_ticks = 0;
}

bool voltage_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    voltage_face_state_t *logger_state = (voltage_face_state_t *)context;
    if (_alarm_held){
        if (!watch_get_pin_level(BTN_ALARM)){
            _alarm_held = false;
        }
        else return true;
    }
    switch (event.event_type) {
        case EVENT_LIGHT_LONG_PRESS:
            // light button shows the timestamp, but if you need the light, long press it.
            movement_illuminate_led();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            logger_state->display_index = (logger_state->display_index + VOLTAGE_LOGGING_CYC - 1) % VOLTAGE_LOGGING_CYC;
            logger_state->ts_ticks = 0;
            _voltage_face_log_update_display(logger_state, settings->bit.clock_mode_24h);
            break;
        case EVENT_ALARM_BUTTON_UP:
            logger_state->display_index = (logger_state->display_index + 1) % VOLTAGE_LOGGING_CYC;
            logger_state->ts_ticks = 0;
            _voltage_face_log_update_display(logger_state, settings->bit.clock_mode_24h);
            break;
        case EVENT_ACTIVATE:
            _voltage_face_update_display();
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (!_displaying_curr) {
                _alarm_held = true;
                logger_state->ts_ticks = 2;
                _voltage_face_log_update_display(logger_state, settings->bit.clock_mode_24h);
            }
            break;
        case EVENT_TICK:
            if (_displaying_curr || (logger_state->ts_ticks && --logger_state->ts_ticks == 0)) {
                _voltage_face_log_update_display(logger_state, settings->bit.clock_mode_24h);
            }
            break;
        case EVENT_BACKGROUND_TASK:
            _voltage_face_log_data(logger_state);
            break;
        default:
            movement_default_loop_handler(event, settings);
            break;
    }

    return true;
}

void voltage_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool voltage_face_wants_background_task(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    // this will get called at the top of each minute, so all we check is if we're at the top of the hour as well.
    // if we are, we ask for a background task.
    return movement_get_utc_date_time().unit.minute == 0;
}
