#ifndef F_CPU
#define F_CPU 8000000UL // 8MHz internal RC oscillator
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>

#define BAUD 9600
#define MYUBRR (F_CPU / 16 / BAUD - 1)

// I2C 7-Bit Addresses (Shifted left by 1 for write mode)
#define DS3231_ADDR   0xD0 // 0x68 << 1
#define HT16K33_ADDR  0xE0 // 0x70 << 1

// HT16K33 Commands
#define HT16K33_CMD_OSC_ON       0x21
#define HT16K33_CMD_OSC_OFF      0x20
#define HT16K33_CMD_DISPLAY_ON   0x81
#define HT16K33_CMD_DISPLAY_OFF  0x80
#define HT16K33_CMD_BRIGHTNESS   0xEF

// Button Pins (Port B)
#define BTN_PORT     PORTB
#define BTN_PIN      PINB
#define BTN_DDR      DDRB
#define BTN_SELECT   PB1 // PCINT1
#define BTN_SET      PB2 // PCINT2

// Passive Buzzer Pin (Port C)
#define BUZZER_PORT  PORTC
#define BUZZER_DDR   DDRC
#define BUZZER_PIN   PC3

// ============================================================================
// GLOBAL CONFIGURATION FLAGS
// ============================================================================

static bool g_uart_debug_enabled = false; 

// ============================================================================
// ENUMS & STRUCTURES
// ============================================================================

typedef enum {
    PAGE_TIME = 0,        // Page 1: HH:MM
    PAGE_DATE,            // Page 2: MM.DD
    PAGE_YEAR,            // Page 3: YYYY
    PAGE_DOW,             // Page 4: 3-Letter Weekday
    PAGE_ALARM1,          // Page 5: A1 HH:MM (Time)
    PAGE_ALARM1_TOGGLE,   // Page 6: A on / A OFF
    PAGE_ALARM2,          // Page 7: A2 HH:MM (Time)
    PAGE_ALARM2_TOGGLE    // Page 8: b on / b OFF
} DisplayPage_t;

#define TOTAL_PAGES 8

typedef enum {
    SET_NONE = 0,
    SET_TIME_HH,
    SET_TIME_MM,
    SET_DATE_MM,
    SET_DATE_DD,
    SET_YEAR_YY,
    SET_DOW,
    SET_A1_HH,
    SET_A1_MM,
    SET_A2_HH,
    SET_A2_MM
} SetField_t;

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t dow;
    uint8_t date;
    uint8_t month;
    uint16_t year;
    uint8_t a1_hour;
    uint8_t a1_min;
    bool a1_enabled;
    uint8_t a2_hour;
    uint8_t a2_min;
    bool a2_enabled;
} WatchData_t;

// ============================================================================
// GLOBAL STATE & FONTS
// ============================================================================

static WatchData_t g_watch = {0, 0, 12, 1, 17, 10, 2026, 7, 0, false, 8, 0, false};
static DisplayPage_t g_current_page = PAGE_TIME;
static SetField_t g_set_field = SET_NONE;

// PCINT Debounce Counters and Event Flags
volatile uint8_t SELECTOR_DEBOUNCE = 0;
volatile uint8_t SETTER_DEBOUNCE   = 0;
volatile uint8_t SELECTOR_PRESS    = 0;
volatile uint8_t SETTER_PRESS      = 0;

// Timing flags
volatile uint16_t g_blink_ticks = 0;
volatile bool     g_blink_state = true;
volatile uint8_t  g_display_ticks = 0;
volatile bool     g_update_display_flag = true;
volatile uint8_t  g_rtc_ticks = 0;
volatile bool     g_read_rtc_flag = false;

// Active Alarm Tracking
volatile uint8_t g_alarm_active = 0;      // 0 = None, 1 = Alarm 1, 2 = Alarm 2
volatile uint8_t g_alarm_cycle_count = 0; // Counts up to 20 cycles
static int8_t g_last_alarm1_min = -1;
static int8_t g_last_alarm2_min = -1;

static const uint8_t segment_font[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

#define SEG_BLANK 0b00000000
#define SEG_A     0b01110111
#define SEG_b     0b01111100
#define SEG_d     0b01011110
#define SEG_E     0b01111001
#define SEG_F     0b01110001
#define SEG_h     0b01110100
#define SEG_i     0b00000100
#define SEG_n     0b01010100
#define SEG_o     0b01011100
#define SEG_r     0b01010000
#define SEG_S     0b01101101
#define SEG_t     0b01111000
#define SEG_u     0b00011100

static const uint8_t dow_font[8][4] = {
    {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK}, 
    {SEG_BLANK, SEG_n,     SEG_o,     SEG_n    }, 
    {SEG_BLANK, SEG_t,     SEG_u,     SEG_E    }, 
    {SEG_BLANK, SEG_u,     SEG_E,     SEG_d    }, 
    {SEG_BLANK, SEG_t,     SEG_h,     SEG_u    }, 
    {SEG_BLANK, SEG_F,     SEG_r,     SEG_i    }, 
    {SEG_BLANK, SEG_S,     SEG_A,     SEG_t    }, 
    {SEG_BLANK, SEG_S,     SEG_u,     SEG_n    }  
};

// ============================================================================
// HARDWARE PERIPHERAL DRIVERS (UART & I2C)
// ============================================================================

void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_print(const char* str) {
    if (!g_uart_debug_enabled) return;
    while (*str) uart_transmit(*str++);
}

void i2c_init(void) {
    TWSR = 0x00;
    TWBR = 32; // 100kHz @ 8MHz
}

void i2c_start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void i2c_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void i2c_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

uint8_t i2c_read_ack(void) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t i2c_read_nack(void) {
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t bcd_to_dec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }
uint8_t dec_to_bcd(uint8_t val) { return ((val / 10 * 16) + (val % 10)); }

// ============================================================================
// BUZZER & ALARM PATTERNS (PC3)
// ============================================================================

void buzzer_init(void) {
    BUZZER_DDR  |= (1 << BUZZER_PIN);   
    BUZZER_PORT &= ~(1 << BUZZER_PIN);  
}

// Plays a square wave tone at a specific frequency (in Hz)
void buzzer_tone(uint16_t freq_hz, uint16_t duration_ms) {
    if (g_alarm_active != 0 && (SELECTOR_PRESS || SETTER_PRESS)) {
        return; // Terminate immediately if a button is pushed
    }

    if (freq_hz == 0) {
        for (uint16_t d = 0; d < duration_ms; d++) {
            _delay_ms(1);
            if (g_alarm_active != 0 && (SELECTOR_PRESS || SETTER_PRESS)) return;
        }
        return;
    }
    
    uint16_t half_period_us = 1000000UL / (2UL * freq_hz);
    uint32_t total_cycles = ((uint32_t)duration_ms * 1000UL) / (2UL * half_period_us);

    for (uint32_t i = 0; i < total_cycles; i++) {
        if (g_alarm_active != 0 && (SELECTOR_PRESS || SETTER_PRESS)) return;
        BUZZER_PORT |= (1 << BUZZER_PIN);
        for (uint16_t h = 0; h < half_period_us / 4; h++) _delay_us(4);
        BUZZER_PORT &= ~(1 << BUZZER_PIN);
        for (uint16_t h = 0; h < half_period_us / 4; h++) _delay_us(4);
    }
}

void buzzer_beep(uint16_t duration_ms) {
    buzzer_tone(2500, duration_ms);
}

// Alarm Pattern A: Fast triple pip (2.8kHz)
void play_alarm_pattern_a(void) {
    for (uint8_t i = 0; i < 3; i++) {
        if (g_alarm_active == 0 || SELECTOR_PRESS || SETTER_PRESS) return;
        buzzer_tone(2800, 60);
        buzzer_tone(0, 50);
    }
    buzzer_tone(0, 350);
}

// Alarm Pattern B: High-Low siren cadence (3.0kHz / 2.0kHz)
void play_alarm_pattern_b(void) {
    for (uint8_t i = 0; i < 2; i++) {
        if (g_alarm_active == 0 || SELECTOR_PRESS || SETTER_PRESS) return;
        buzzer_tone(3000, 110);
        buzzer_tone(0, 30);
        buzzer_tone(2000, 110);
        buzzer_tone(0, 50);
    }
    buzzer_tone(0, 300);
}

// ============================================================================
// HT16K33 DRIVER ROUTINES
// ============================================================================

void ht16k33_write_cmd(uint8_t cmd) {
    i2c_start();
    i2c_write(HT16K33_ADDR);
    i2c_write(cmd);
    i2c_stop();
}

void ht16k33_init(void) {
    _delay_ms(10);
    ht16k33_write_cmd(HT16K33_CMD_OSC_ON);
    ht16k33_write_cmd(HT16K33_CMD_BRIGHTNESS);
    ht16k33_write_cmd(HT16K33_CMD_DISPLAY_ON);
}

void ht16k33_render_ram(const uint8_t digit_patterns[4], bool colon_on, uint8_t dp_mask) {
    uint8_t ram_buffer[16] = {0};

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t pattern = digit_patterns[i];
        if (dp_mask & (1 << i)) {
            pattern |= (1 << 7);
        }
        ram_buffer[i * 2] = pattern;
    }

    if (colon_on) {
        ram_buffer[8] = (0x01 | 0x02);
    }

    i2c_start();
    i2c_write(HT16K33_ADDR);
    i2c_write(0x00);

    for (uint8_t i = 0; i < 16; i++) {
        i2c_write(ram_buffer[i]);
    }

    i2c_stop();
}

// ============================================================================
// DS3231 RTC ROUTINES
// ============================================================================

void ds3231_write_time(void) {
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x00);
    i2c_write(dec_to_bcd(g_watch.sec));
    i2c_write(dec_to_bcd(g_watch.min));
    i2c_write(dec_to_bcd(g_watch.hour) & 0x3F);
    i2c_write(dec_to_bcd(g_watch.dow));
    i2c_write(dec_to_bcd(g_watch.date));
    i2c_write(dec_to_bcd(g_watch.month) & 0x1F);
    i2c_write(dec_to_bcd((uint8_t)(g_watch.year % 100)));
    i2c_stop();
}

void ds3231_write_alarms(void) {
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x07);
    i2c_write(0x00);
    i2c_write(dec_to_bcd(g_watch.a1_min));
    i2c_write(dec_to_bcd(g_watch.a1_hour) & 0x3F);
    i2c_write(0x80);
    i2c_stop();

    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x0B);
    i2c_write(dec_to_bcd(g_watch.a2_min));
    i2c_write(dec_to_bcd(g_watch.a2_hour) & 0x3F);
    i2c_write(0x80);
    i2c_stop();
}

void ds3231_read_all(void) {
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x00);

    i2c_start();
    i2c_write(DS3231_ADDR | 0x01);
    g_watch.sec   = bcd_to_dec(i2c_read_ack() & 0x7F);
    g_watch.min   = bcd_to_dec(i2c_read_ack());
    g_watch.hour  = bcd_to_dec(i2c_read_ack() & 0x3F);
    g_watch.dow   = bcd_to_dec(i2c_read_ack());
    g_watch.date  = bcd_to_dec(i2c_read_ack());
    g_watch.month = bcd_to_dec(i2c_read_ack() & 0x1F);
    uint8_t yr    = bcd_to_dec(i2c_read_nack());
    i2c_stop();

    g_watch.year = 2000 + yr;
}

void ds3231_check_power_loss(void) {
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x0F);

    i2c_start();
    i2c_write(DS3231_ADDR | 0x01);
    uint8_t status = i2c_read_nack();
    i2c_stop();

    if (status & (1 << 7)) {
        uart_print("Power loss detected! Initializing defaults...\r\n");
        ds3231_write_time();
        ds3231_write_alarms();

        i2c_start();
        i2c_write(DS3231_ADDR);
        i2c_write(0x0F);
        i2c_write(status & ~(1 << 7));
        i2c_stop();
    }
}

// ============================================================================
// SYSTEM TIMERS & INTERRUPTS
// ============================================================================

void buttons_init(void) {
    BTN_DDR &= ~((1 << BTN_SELECT) | (1 << BTN_SET));
    BTN_PORT |= (1 << BTN_SELECT) | (1 << BTN_SET);

    PCICR  |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT1) | (1 << PCINT2);
}

void timer1_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, Prescaler = 64
    OCR1A = 499; // 250Hz = 4.0ms interrupt rate
    TIMSK1 = (1 << OCIE1A);
}

ISR(PCINT0_vect) {
    // PB1 - Selector button
    if ((!(PINB & _BV(PB1))) && (SELECTOR_DEBOUNCE == 0)) {
        SELECTOR_PRESS = 1;
        SELECTOR_DEBOUNCE = 50;  // ~200ms debounce guard
    }

    // PB2 - Setter button
    if ((!(PINB & _BV(PB2))) && (SETTER_DEBOUNCE == 0)) {
        SETTER_PRESS = 1;
        SETTER_DEBOUNCE = 50;    // ~200ms debounce guard
    }
}

ISR(TIMER1_COMPA_vect) {
    if (SELECTOR_DEBOUNCE > 0) {
        SELECTOR_DEBOUNCE--;
    }

    if (SETTER_DEBOUNCE > 0) {
        SETTER_DEBOUNCE--;
    }

    if (++g_blink_ticks >= 125) { // 500ms toggle
        g_blink_ticks = 0;
        g_blink_state = !g_blink_state;
        g_update_display_flag = true;
    }

    if (++g_display_ticks >= 12) { // ~20Hz refresh
        g_display_ticks = 0;
        g_update_display_flag = true;
    }

    if (++g_rtc_ticks >= 125) { // 2Hz RTC read
        g_rtc_ticks = 0;
        g_read_rtc_flag = true;
    }
}

uint8_t get_days_in_month(uint8_t month, uint16_t year) {
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
        return 28;
    }
    return 31;
}

// ============================================================================
// SETTING & INCREMENT LOGIC
// ============================================================================

void increment_set_field(void) {
    switch (g_set_field) {
        case SET_TIME_HH: g_watch.hour = (g_watch.hour + 1) % 24; break;
        case SET_TIME_MM: g_watch.min = (g_watch.min + 1) % 60; break;

        case SET_DATE_MM: {
            g_watch.month++;
            if (g_watch.month > 12) g_watch.month = 1;
            uint8_t max_d = get_days_in_month(g_watch.month, g_watch.year);
            if (g_watch.date > max_d) g_watch.date = max_d;
            break;
        }

        case SET_DATE_DD: {
            uint8_t max_days = get_days_in_month(g_watch.month, g_watch.year);
            g_watch.date++;
            if (g_watch.date > max_days) g_watch.date = 1;
            break;
        }

        case SET_YEAR_YY:
            g_watch.year++;
            if (g_watch.year > 2030) g_watch.year = 2020;
            break;

        case SET_DOW:
            g_watch.dow++;
            if (g_watch.dow > 7) g_watch.dow = 1;
            break;

        case SET_A1_HH: g_watch.a1_hour = (g_watch.a1_hour + 1) % 24; break;
        case SET_A1_MM: g_watch.a1_min = (g_watch.a1_min + 1) % 60; break;
        case SET_A2_HH: g_watch.a2_hour = (g_watch.a2_hour + 1) % 24; break;
        case SET_A2_MM: g_watch.a2_min = (g_watch.a2_min + 1) % 60; break;

        case SET_NONE:
        default: break;
    }
}

void advance_set_field(void) {
    switch (g_current_page) {
        case PAGE_TIME:
            if (g_set_field == SET_NONE) g_set_field = SET_TIME_HH;
            else if (g_set_field == SET_TIME_HH) g_set_field = SET_TIME_MM;
            else { g_set_field = SET_NONE; ds3231_write_time(); }
            break;

        case PAGE_DATE:
            if (g_set_field == SET_NONE) g_set_field = SET_DATE_MM;
            else if (g_set_field == SET_DATE_MM) g_set_field = SET_DATE_DD;
            else { g_set_field = SET_NONE; ds3231_write_time(); }
            break;

        case PAGE_YEAR:
            if (g_set_field == SET_NONE) g_set_field = SET_YEAR_YY;
            else { g_set_field = SET_NONE; ds3231_write_time(); }
            break;

        case PAGE_DOW:
            if (g_set_field == SET_NONE) g_set_field = SET_DOW;
            else { g_set_field = SET_NONE; ds3231_write_time(); }
            break;

        case PAGE_ALARM1:
            if (g_set_field == SET_NONE) g_set_field = SET_A1_HH;
            else if (g_set_field == SET_A1_HH) g_set_field = SET_A1_MM;
            else { g_set_field = SET_NONE; ds3231_write_alarms(); }
            break;

        case PAGE_ALARM2:
            if (g_set_field == SET_NONE) g_set_field = SET_A2_HH;
            else if (g_set_field == SET_A2_HH) g_set_field = SET_A2_MM;
            else { g_set_field = SET_NONE; ds3231_write_alarms(); }
            break;

        default:
            g_set_field = SET_NONE;
            break;
    }
}

void update_display(void) {
    uint8_t patterns[4] = {0};
    uint8_t dp_mask = 0x00;
    bool colon_on = false;

    switch (g_current_page) {
        case PAGE_TIME:
            patterns[0] = segment_font[g_watch.hour / 10];
            patterns[1] = segment_font[g_watch.hour % 10];
            patterns[2] = segment_font[g_watch.min / 10];
            patterns[3] = segment_font[g_watch.min % 10];

            if (g_set_field == SET_NONE) {
                colon_on = true;
            } else if (g_set_field == SET_TIME_HH) {
                colon_on = g_blink_state;
                if (g_blink_state) dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_TIME_MM) {
                colon_on = g_blink_state;
                if (g_blink_state) dp_mask |= (1 << 2) | (1 << 3);
            }
            break;

        case PAGE_DATE:
            patterns[0] = segment_font[g_watch.month / 10];
            patterns[1] = segment_font[g_watch.month % 10];
            patterns[2] = segment_font[g_watch.date / 10];
            patterns[3] = segment_font[g_watch.date % 10];

            if (g_set_field == SET_NONE) {
                dp_mask |= (1 << 1);
            } else if (g_set_field == SET_DATE_MM) {
                if (g_blink_state) dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_DATE_DD) {
                if (g_blink_state) dp_mask |= (1 << 2) | (1 << 3);
            }
            break;

        case PAGE_YEAR:
            patterns[0] = segment_font[g_watch.year / 1000];
            patterns[1] = segment_font[(g_watch.year / 100) % 10];
            patterns[2] = segment_font[(g_watch.year / 10) % 10];
            patterns[3] = segment_font[g_watch.year % 10];

            if (g_set_field == SET_YEAR_YY && g_blink_state) {
                dp_mask |= 0x0F;
            }
            break;

        case PAGE_DOW: {
            uint8_t day_idx = (g_watch.dow >= 1 && g_watch.dow <= 7) ? g_watch.dow : 1;
            patterns[0] = dow_font[day_idx][0];
            patterns[1] = dow_font[day_idx][1];
            patterns[2] = dow_font[day_idx][2];
            patterns[3] = dow_font[day_idx][3];

            if (g_set_field == SET_DOW && g_blink_state) {
                dp_mask |= (1 << 1) | (1 << 2) | (1 << 3);
            }
            break;
        }

        // Clean Alarm 1 Time Display: HH:MM with NO extra DPs
        case PAGE_ALARM1:
            patterns[0] = segment_font[g_watch.a1_hour / 10];
            patterns[1] = segment_font[g_watch.a1_hour % 10];
            patterns[2] = segment_font[g_watch.a1_min / 10];
            patterns[3] = segment_font[g_watch.a1_min % 10];

            if (g_set_field == SET_NONE) {
                colon_on = true;
            } else if (g_set_field == SET_A1_HH) {
                colon_on = g_blink_state;
                if (g_blink_state) dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_A1_MM) {
                colon_on = g_blink_state;
                if (g_blink_state) dp_mask |= (1 << 2) | (1 << 3);
            }
            break;

        // Alarm 1 Toggle Screen: "A  on" or "A OFF"
        case PAGE_ALARM1_TOGGLE:
            patterns[0] = SEG_A;
            if (g_watch.a1_enabled) {
                patterns[1] = SEG_BLANK;
                patterns[2] = SEG_o;
                patterns[3] = SEG_n;
            } else {
                patterns[1] = SEG_o;
                patterns[2] = SEG_F;
                patterns[3] = SEG_F;
            }
            colon_on = false;
            break;

        // Clean Alarm 2 Time Display: HH:MM with NO extra DPs
        case PAGE_ALARM2:
            patterns[0] = segment_font[g_watch.a2_hour / 10];
            patterns[1] = segment_font[g_watch.a2_hour % 10];
            patterns[2] = segment_font[g_watch.a2_min / 10];
            patterns[3] = segment_font[g_watch.a2_min % 10];

            if (g_set_field == SET_NONE) {
                colon_on = true;
            } else if (g_set_field == SET_A2_HH) {
                colon_on = g_blink_state;
                if (g_blink_state) dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_A2_MM) {
                colon_on = g_blink_state;
                if (g_blink_state) dp_mask |= (1 << 2) | (1 << 3);
            }
            break;

        // Alarm 2 Toggle Screen: "b  on" or "b OFF"
        case PAGE_ALARM2_TOGGLE:
            patterns[0] = SEG_b;
            if (g_watch.a2_enabled) {
                patterns[1] = SEG_BLANK;
                patterns[2] = SEG_o;
                patterns[3] = SEG_n;
            } else {
                patterns[1] = SEG_o;
                patterns[2] = SEG_F;
                patterns[3] = SEG_F;
            }
            colon_on = false;
            break;
    }

    ht16k33_render_ram(patterns, colon_on, dp_mask);
}

// ============================================================================
// MAIN APPLICATION LOOP
// ============================================================================

int main(void) {   
    char buffer[64];
    static int8_t last_buzzed_minute = -1;

    buttons_init();
    buzzer_init();
    timer1_init();
    uart_init(MYUBRR);
    i2c_init();

    sei();

    ht16k33_init();
    ds3231_check_power_loss();
    ds3231_read_all();

    buzzer_beep(40);

    uart_print("\r\n--- HT16K33 Watch (20-Cycle Dual Alarms) Ready ---\r\n");

    while (true) {
        // ====================================================================
        // 1. BUTTON EVENT HANDLING (Normal UI vs. Alarm Suspension)
        // ====================================================================

        // If the alarm is currently sounding, suspend normal actions:
        // Either button turns OFF the alarm immediately and exits the block.
        if (g_alarm_active != 0) {
            if (SELECTOR_PRESS || SETTER_PRESS) {
                SELECTOR_PRESS = 0;
                SETTER_PRESS = 0;
                g_alarm_active = 0;
                g_alarm_cycle_count = 0;
                update_display();
            }
        } else {
            // Normal button functionality active only when alarm is quiet
            if (SELECTOR_PRESS) {
                SELECTOR_PRESS = 0;
                if (g_set_field != SET_NONE) {
                    advance_set_field();
                } else {
                    g_current_page = (DisplayPage_t)((g_current_page + 1) % TOTAL_PAGES);
                }
                update_display();
            }

            if (SETTER_PRESS) {
                SETTER_PRESS = 0;
                if (g_set_field != SET_NONE) {
                    increment_set_field();
                } else if (g_current_page == PAGE_ALARM1_TOGGLE) {
                    g_watch.a1_enabled = !g_watch.a1_enabled;
                    buzzer_beep(20);
                } else if (g_current_page == PAGE_ALARM2_TOGGLE) {
                    g_watch.a2_enabled = !g_watch.a2_enabled;
                    buzzer_beep(20);
                } else {
                    advance_set_field();
                }
                update_display();
            }
        }

        // ====================================================================
        // 2. BACKGROUND RTC READ & ALARM ARMS
        // ====================================================================
        if (g_read_rtc_flag) {
            g_read_rtc_flag = false;
            if (g_set_field == SET_NONE) {
                ds3231_read_all();

                // Alarm 1 Trigger Check
                if (g_watch.a1_enabled && 
                    g_watch.hour == g_watch.a1_hour && 
                    g_watch.min == g_watch.a1_min && 
                    g_watch.sec < 10 && 
                    g_last_alarm1_min != g_watch.min) {
                    g_alarm_active = 1;
                    g_alarm_cycle_count = 20; // Run 20 times max
                    g_last_alarm1_min = g_watch.min;
                }

                // Alarm 2 Trigger Check
                if (g_watch.a2_enabled && 
                    g_watch.hour == g_watch.a2_hour && 
                    g_watch.min == g_watch.a2_min && 
                    g_watch.sec < 10 && 
                    g_last_alarm2_min != g_watch.min) {
                    g_alarm_active = 2;
                    g_alarm_cycle_count = 20; // Run 20 times max
                    g_last_alarm2_min = g_watch.min;
                }

                // Rearm triggers when minute rolls over
                if (g_watch.min != g_watch.a1_min) g_last_alarm1_min = -1;
                if (g_watch.min != g_watch.a2_min) g_last_alarm2_min = -1;

                // Minute Chime at 00s (only when alarm isn't active)
                if (g_alarm_active == 0) {
                    if (g_watch.sec == 0 && g_watch.min != last_buzzed_minute) {
                        last_buzzed_minute = g_watch.min;
                        buzzer_beep(35);
                    } else if (g_watch.sec != 0) {
                        last_buzzed_minute = -1;
                    }
                }
            }

            if (g_uart_debug_enabled) {
                snprintf(buffer, sizeof(buffer), "P:%d | Date: %04d-%02d-%02d | Time: %02d:%02d:%02d\r\n", 
                         g_current_page + 1, g_watch.year, g_watch.month, g_watch.date, 
                         g_watch.hour, g_watch.min, g_watch.sec);
                uart_print(buffer);
            }
        }

        // ====================================================================
        // 3. SOUND GENERATION & CYCLE DECREMENT
        // ====================================================================
        if (g_alarm_active == 1) {
            play_alarm_pattern_a();
            if (g_alarm_active != 0) { // If not silenced mid-melody
                if (--g_alarm_cycle_count == 0) {
                    g_alarm_active = 0; // Terminate after 20 cycles
                }
            }
        } else if (g_alarm_active == 2) {
            play_alarm_pattern_b();
            if (g_alarm_active != 0) { // If not silenced mid-melody
                if (--g_alarm_cycle_count == 0) {
                    g_alarm_active = 0; // Terminate after 20 cycles
                }
            }
        }

        // ====================================================================
        // 4. PERIODIC DISPLAY REFRESH
        // ====================================================================
        if (g_update_display_flag) {
            g_update_display_flag = false;
            update_display();
        }
    }

    return 0;
}