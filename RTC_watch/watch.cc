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

#define DS3231_ADDR 0xD0 // 0x68 shifted left by 1 bit for I2C write

// ============================================================================
// HARDWARE PIN MAPPING DEFINITIONS
// ============================================================================

// Segment Pins
#define SEG_A_PORT   PORTD
#define SEG_A_DDR    DDRD
#define SEG_A_PIN    PD3

#define SEG_B_PORT   PORTD
#define SEG_B_DDR    DDRD
#define SEG_B_PIN    PD4

#define SEG_C_PORT   PORTB
#define SEG_C_DDR    DDRB
#define SEG_C_PIN    PB6

#define SEG_D_PORT   PORTB
#define SEG_D_DDR    DDRB
#define SEG_D_PIN    PB7

#define SEG_E_PORT   PORTD
#define SEG_E_DDR    DDRD
#define SEG_E_PIN    PD5

#define SEG_F_PORT   PORTD
#define SEG_F_DDR    DDRD
#define SEG_F_PIN    PD6

#define SEG_G_PORT   PORTD
#define SEG_G_DDR    DDRD
#define SEG_G_PIN    PD7

#define SEG_DP_PORT  PORTB
#define SEG_DP_DDR   DDRB
#define SEG_DP_PIN   PB0

// Digit Selection Pins (Driven HIGH to turn on AO3400 Low-Side MOSFETs)
#define DIG1_PORT    PORTC
#define DIG1_DDR     DDRC
#define DIG1_PIN     PC0

#define DIG2_PORT    PORTC
#define DIG2_DDR     DDRC
#define DIG2_PIN     PC1

#define DIG3_PORT    PORTC
#define DIG3_DDR     DDRC
#define DIG3_PIN     PC2

#define DIG4_PORT    PORTC
#define DIG4_DDR     DDRC
#define DIG4_PIN     PC3

// Button Pins (External Pull-Up to 3.3V, Tactile switch to GND)
#define BTN_PORT     PORTB
#define BTN_PIN      PINB
#define BTN_DDR      DDRB
#define BTN_SELECT   PB1
#define BTN_SET      PB2

// ============================================================================
// ENUMS & STATE STRUCTURES
// ============================================================================

typedef enum {
    PAGE_TIME = 0,    // Page 1: HH.MM
    PAGE_DATE,        // Page 2: MM.DD
    PAGE_YEAR,        // Page 3: YYYY
    PAGE_DOW,         // Page 4: 3-Letter Weekday (non, tuE...)
    PAGE_ALARM1,      // Page 5: A1 HH.MM
    PAGE_ALARM2       // Page 6: A2 HH.MM
} DisplayPage_t;

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

// Time and Alarm State Data Structure
typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t dow;     // 1 - 7
    uint8_t date;    // 1 - 31
    uint8_t month;   // 1 - 12
    uint16_t year;   // 2020 - 2030
    uint8_t a1_hour;
    uint8_t a1_min;
    uint8_t a2_hour;
    uint8_t a2_min;
} WatchData_t;

// ============================================================================
// GLOBAL VARIABLES & 7-SEGMENT FONTS
// ============================================================================

volatile uint8_t g_display_digits[4] = {0, 0, 0, 0};
volatile uint8_t g_dp_mask = 0x00; // Bitfield for DP status on Digits 1-4

// System state variables
static WatchData_t g_watch = {0, 0, 12, 1, 17, 10, 2026, 7, 0, 8, 0};
static DisplayPage_t g_current_page = PAGE_TIME;
static SetField_t g_set_field = SET_NONE;

// Debounce & Long-Press tracking
volatile uint8_t g_debounce_ticks = 0;
volatile uint16_t g_blink_ticks = 0;
volatile bool g_blink_state = true;

// Raw segment font for digits 0-9 (Active HIGH)
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

// Raw 7-segment bit patterns for 3-letter weekday characters
#define SEG_BLANK 0b00000000
#define SEG_A     0b01110111
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

// 7-Day Lookup Table: [Digit 1, Digit 2, Digit 3, Digit 4]
static const uint8_t dow_font[8][4] = {
    {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK}, // 0: Placeholder
    {SEG_BLANK, SEG_n,     SEG_o,     SEG_n    }, // 1:  non (Mon)
    {SEG_BLANK, SEG_t,     SEG_u,     SEG_E    }, // 2:  tuE (Tue)
    {SEG_BLANK, SEG_u,     SEG_E,     SEG_d    }, // 3:  uEd (Wed)
    {SEG_BLANK, SEG_t,     SEG_h,     SEG_u    }, // 4:  thu (Thu)
    {SEG_BLANK, SEG_F,     SEG_r,     SEG_i    }, // 5:  Fri (Fri)
    {SEG_BLANK, SEG_S,     SEG_A,     SEG_t    }, // 6:  SAt (Sat)
    {SEG_BLANK, SEG_S,     SEG_u,     SEG_n    }  // 7:  Sun (Sun)
};

// --- UART Functions ---
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
    while (*str) {
        uart_transmit(*str++);
    }
}

// --- Hardware I2C (TWI) Functions ---
void i2c_init(void) {
    TWSR = 0x00;
    TWBR = 32;   // 100kHz @ 8MHz
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

// BCD / Decimal Converters
uint8_t bcd_to_dec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));
}

// Write updated Date & Time registers to DS3231
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

// Write Alarm 1 (0x07) and Alarm 2 (0x0B) to DS3231 registers
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

// Read current RTC state
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

// Check power loss status
void ds3231_check_power_loss(void) {
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x0E);
    i2c_write(0x00);
    i2c_stop();

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
// DISPLAY & MULTIPLEXING FUNCTIONS
// ============================================================================

void display_init(void) {
    SEG_A_DDR  |= (1 << SEG_A_PIN);
    SEG_B_DDR  |= (1 << SEG_B_PIN);
    SEG_C_DDR  |= (1 << SEG_C_PIN);
    SEG_D_DDR  |= (1 << SEG_D_PIN);
    SEG_E_DDR  |= (1 << SEG_E_PIN);
    SEG_F_DDR  |= (1 << SEG_F_PIN);
    SEG_G_DDR  |= (1 << SEG_G_PIN);
    SEG_DP_DDR |= (1 << SEG_DP_PIN);

    DIG1_DDR |= (1 << DIG1_PIN);
    DIG2_DDR |= (1 << DIG2_PIN);
    DIG3_DDR |= (1 << DIG3_PIN);
    DIG4_DDR |= (1 << DIG4_PIN);

    DIG1_PORT &= ~(1 << DIG1_PIN);
    DIG2_PORT &= ~(1 << DIG2_PIN);
    DIG3_PORT &= ~(1 << DIG3_PIN);
    DIG4_PORT &= ~(1 << DIG4_PIN);

    BTN_DDR &= ~((1 << BTN_SELECT) | (1 << BTN_SET));
}

void display_clear_all(void) {
    DIG1_PORT &= ~(1 << DIG1_PIN);
    DIG2_PORT &= ~(1 << DIG2_PIN);
    DIG3_PORT &= ~(1 << DIG3_PIN);
    DIG4_PORT &= ~(1 << DIG4_PIN);

    SEG_A_PORT &= ~(1 << SEG_A_PIN);
    SEG_B_PORT &= ~(1 << SEG_B_PIN);
    SEG_C_PORT &= ~(1 << SEG_C_PIN);
    SEG_D_PORT &= ~(1 << SEG_D_PIN);
    SEG_E_PORT &= ~(1 << SEG_E_PIN);
    SEG_F_PORT &= ~(1 << SEG_F_PIN);
    SEG_G_PORT &= ~(1 << SEG_G_PIN);
    SEG_DP_PORT &= ~(1 << SEG_DP_PIN);
}

void display_set_segments(uint8_t pattern) {
    if (pattern & (1 << 0)) SEG_A_PORT |= (1 << SEG_A_PIN); else SEG_A_PORT &= ~(1 << SEG_A_PIN);
    if (pattern & (1 << 1)) SEG_B_PORT |= (1 << SEG_B_PIN); else SEG_B_PORT &= ~(1 << SEG_B_PIN);
    if (pattern & (1 << 2)) SEG_C_PORT |= (1 << SEG_C_PIN); else SEG_C_PORT &= ~(1 << SEG_C_PIN);
    if (pattern & (1 << 3)) SEG_D_PORT |= (1 << SEG_D_PIN); else SEG_D_PORT &= ~(1 << SEG_D_PIN);
    if (pattern & (1 << 4)) SEG_E_PORT |= (1 << SEG_E_PIN); else SEG_E_PORT &= ~(1 << SEG_E_PIN);
    if (pattern & (1 << 5)) SEG_F_PORT |= (1 << SEG_F_PIN); else SEG_F_PORT &= ~(1 << SEG_F_PIN);
    if (pattern & (1 << 6)) SEG_G_PORT |= (1 << SEG_G_PIN); else SEG_G_PORT &= ~(1 << SEG_G_PIN);
    if (pattern & (1 << 7)) SEG_DP_PORT |= (1 << SEG_DP_PIN); else SEG_DP_PORT &= ~(1 << SEG_DP_PIN);
}

void timer1_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, Prescaler = 64
    OCR1A = 499; // 250Hz refresh rate
    TIMSK1 = (1 << OCIE1A);
}

void buttons_init(void) {
    PCICR |= (1 << PCIE0);                         // Enable Port B Pin Change Interrupts
    PCMSK0 |= (1 << PCINT1) | (1 << PCINT2);       // Enable interrupts for PB1 and PB2
}

ISR(TIMER1_COMPA_vect) {
    static uint8_t current_digit = 0;

    display_clear_all();

    uint8_t pattern = 0;
    if (g_current_page == PAGE_DOW) {
        uint8_t day_index = g_watch.dow;
        if (day_index < 1 || day_index > 7) day_index = 1;
        pattern = dow_font[day_index][current_digit];
    } else {
        pattern = segment_font[g_display_digits[current_digit] % 10];
    }

    if (g_dp_mask & (1 << current_digit)) {
        pattern |= (1 << 7);
    }

    display_set_segments(pattern);

    switch (current_digit) {
        case 0: DIG1_PORT |= (1 << DIG1_PIN); break;
        case 1: DIG2_PORT |= (1 << DIG2_PIN); break;
        case 2: DIG3_PORT |= (1 << DIG3_PIN); break;
        case 3: DIG4_PORT |= (1 << DIG4_PIN); break;
    }

    current_digit = (current_digit + 1) % 4;

    if (g_debounce_ticks > 0) {
        g_debounce_ticks--;
    }

    if (++g_blink_ticks >= 125) {
        g_blink_ticks = 0;
        g_blink_state = !g_blink_state;
    }
}

ISR(PCINT0_vect) {
    // Pin change captured; state machine debouncing handled in main loop
}

uint8_t get_days_in_month(uint8_t month, uint16_t year) {
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    } else if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29;
        }
        return 28;
    }
    return 31;
}

// ============================================================================
// SETTING & INCREMENT LOGIC
// ============================================================================

void increment_set_field(void) {
    switch (g_set_field) {
        case SET_TIME_HH:
            g_watch.hour = (g_watch.hour + 1) % 24;
            break;

        case SET_TIME_MM:
            g_watch.min = (g_watch.min + 1) % 60;
            break;

        case SET_DATE_MM: {
            g_watch.month++;
            if (g_watch.month > 12) {
                g_watch.month = 1;
            }
            uint8_t max_d = get_days_in_month(g_watch.month, g_watch.year);
            if (g_watch.date > max_d) {
                g_watch.date = max_d;
            }
            break;
        }

        case SET_DATE_DD: {
            uint8_t max_days = get_days_in_month(g_watch.month, g_watch.year);
            g_watch.date++;
            if (g_watch.date > max_days) {
                g_watch.date = 1;
            }
            break;
        }

        case SET_YEAR_YY:
            g_watch.year++;
            if (g_watch.year > 2030) {
                g_watch.year = 2020; // Wraps cleanly from 2030 back to 2020
            }
            break;

        case SET_DOW:
            g_watch.dow++;
            if (g_watch.dow > 7) {
                g_watch.dow = 1;
            }
            break;

        case SET_A1_HH:
            g_watch.a1_hour = (g_watch.a1_hour + 1) % 24;
            break;

        case SET_A1_MM:
            g_watch.a1_min = (g_watch.a1_min + 1) % 60;
            break;

        case SET_A2_HH:
            g_watch.a2_hour = (g_watch.a2_hour + 1) % 24;
            break;

        case SET_A2_MM:
            g_watch.a2_min = (g_watch.a2_min + 1) % 60;
            break;

        case SET_NONE:
        default:
            break;
    }
}

void advance_set_field(void) {
    switch (g_current_page) {
        case PAGE_TIME:
            if (g_set_field == SET_NONE) g_set_field = SET_TIME_HH;
            else if (g_set_field == SET_TIME_HH) g_set_field = SET_TIME_MM;
            else {
                g_set_field = SET_NONE;
                ds3231_write_time();
            }
            break;

        case PAGE_DATE:
            if (g_set_field == SET_NONE) g_set_field = SET_DATE_MM;
            else if (g_set_field == SET_DATE_MM) g_set_field = SET_DATE_DD;
            else {
                g_set_field = SET_NONE;
                ds3231_write_time();
            }
            break;

        case PAGE_YEAR:
            if (g_set_field == SET_NONE) g_set_field = SET_YEAR_YY;
            else {
                g_set_field = SET_NONE;
                ds3231_write_time();
            }
            break;

        case PAGE_DOW:
            if (g_set_field == SET_NONE) g_set_field = SET_DOW;
            else {
                g_set_field = SET_NONE;
                ds3231_write_time();
            }
            break;

        case PAGE_ALARM1:
            if (g_set_field == SET_NONE) g_set_field = SET_A1_HH;
            else if (g_set_field == SET_A1_HH) g_set_field = SET_A1_MM;
            else {
                g_set_field = SET_NONE;
                ds3231_write_alarms();
            }
            break;

        case PAGE_ALARM2:
            if (g_set_field == SET_NONE) g_set_field = SET_A2_HH;
            else if (g_set_field == SET_A2_HH) g_set_field = SET_A2_MM;
            else {
                g_set_field = SET_NONE;
                ds3231_write_alarms();
            }
            break;
    }
}

void update_display_buffer(void) {
    g_dp_mask = 0x00;

    switch (g_current_page) {
        case PAGE_TIME:
            g_display_digits[0] = g_watch.hour / 10;
            g_display_digits[1] = g_watch.hour % 10;
            g_display_digits[2] = g_watch.min / 10;
            g_display_digits[3] = g_watch.min % 10;
            
            if (g_set_field == SET_NONE) {
                g_dp_mask |= (1 << 1);
            } else if (g_set_field == SET_TIME_HH) {
                if (g_blink_state) g_dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_TIME_MM) {
                if (g_blink_state) g_dp_mask |= (1 << 2) | (1 << 3);
            }
            break;

        case PAGE_DATE:
            g_display_digits[0] = g_watch.month / 10;
            g_display_digits[1] = g_watch.month % 10;
            g_display_digits[2] = g_watch.date / 10;
            g_display_digits[3] = g_watch.date % 10;

            if (g_set_field == SET_NONE) {
                g_dp_mask |= (1 << 1);
            } else if (g_set_field == SET_DATE_MM) {
                if (g_blink_state) g_dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_DATE_DD) {
                if (g_blink_state) g_dp_mask |= (1 << 2) | (1 << 3);
            }
            break;

        case PAGE_YEAR:
            g_display_digits[0] = g_watch.year / 1000;
            g_display_digits[1] = (g_watch.year / 100) % 10;
            g_display_digits[2] = (g_watch.year / 10) % 10;
            g_display_digits[3] = g_watch.year % 10;

            if (g_set_field == SET_YEAR_YY && g_blink_state) {
                g_dp_mask |= 0x0F;
            }
            break;

        case PAGE_DOW:
            if (g_set_field == SET_DOW && g_blink_state) {
                g_dp_mask |= (1 << 1) | (1 << 2) | (1 << 3);
            }
            break;

        case PAGE_ALARM1:
            g_display_digits[0] = g_watch.a1_hour / 10;
            g_display_digits[1] = g_watch.a1_hour % 10;
            g_display_digits[2] = g_watch.a1_min / 10;
            g_display_digits[3] = g_watch.a1_min % 10;

            if (g_set_field == SET_NONE) {
                g_dp_mask |= (1 << 0);
            } else if (g_set_field == SET_A1_HH) {
                if (g_blink_state) g_dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_A1_MM) {
                if (g_blink_state) g_dp_mask |= (1 << 2) | (1 << 3);
            }
            break;

        case PAGE_ALARM2:
            g_display_digits[0] = g_watch.a2_hour / 10;
            g_display_digits[1] = g_watch.a2_hour % 10;
            g_display_digits[2] = g_watch.a2_min / 10;
            g_display_digits[3] = g_watch.a2_min % 10;

            if (g_set_field == SET_NONE) {
                g_dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_A2_HH) {
                if (g_blink_state) g_dp_mask |= (1 << 0) | (1 << 1);
            } else if (g_set_field == SET_A2_MM) {
                if (g_blink_state) g_dp_mask |= (1 << 2) | (1 << 3);
            }
            break;
    }
}

// ============================================================================
// MAIN APPLICATION LOOP
// ============================================================================

int main(void) {   
    char buffer[64]; // Expanded to 64 bytes to eliminate gcc truncation warning

    static bool select_was_pressed = false;
    static uint16_t select_hold_ticks = 0;
    static bool set_was_pressed = false;

    display_init();
    buttons_init();
    timer1_init();
    uart_init(MYUBRR);
    i2c_init();

    sei();

    ds3231_check_power_loss();
    ds3231_read_all();

    uart_print("\r\n--- Non-Blocking State Watch Ready ---\r\n");

    while (true) {
        if (g_set_field == SET_NONE) {
            ds3231_read_all();
        }

        if (g_debounce_ticks == 0) {
            
            // SELECT BUTTON (PB1) - State-based Press/Hold
            bool select_is_down = !(BTN_PIN & (1 << BTN_SELECT));

            if (select_is_down) {
                if (!select_was_pressed) {
                    select_was_pressed = true;
                    select_hold_ticks = 0;
                } else {
                    select_hold_ticks++;
                    
                    // Long press threshold (~500ms at 10ms main loop rate)
                    if (select_hold_ticks >= 50) {
                        advance_set_field();
                        select_was_pressed = false; 
                        g_debounce_ticks = 30; // 300ms guard after hold
                    }
                }
            } else {
                if (select_was_pressed) {
                    // Short press triggers cleanly on button RELEASE
                    if (g_set_field != SET_NONE) {
                        advance_set_field();
                    } else {
                        g_current_page = (DisplayPage_t)((g_current_page + 1) % 6);
                    }
                    
                    select_was_pressed = false;
                    select_hold_ticks = 0;
                    g_debounce_ticks = 15; // 150ms release debounce
                }
            }

            // SET BUTTON (PB2) - Value Increment
            bool set_is_down = !(BTN_PIN & (1 << BTN_SET));

            if (set_is_down) {
                if (!set_was_pressed) {
                    set_was_pressed = true;
                    if (g_set_field != SET_NONE) {
                        increment_set_field();
                    }
                    g_debounce_ticks = 15; // 150ms light debounce
                }
            } else {
                set_was_pressed = false;
            }
        }

        update_display_buffer();

        snprintf(buffer, sizeof(buffer), "P:%d | Date: %04d-%02d-%02d | Time: %02d:%02d:%02d\r\n", 
                 g_current_page + 1, g_watch.year, g_watch.month, g_watch.date, 
                 g_watch.hour, g_watch.min, g_watch.sec);
        uart_print(buffer);

        _delay_ms(10); // Fast 10ms main iteration for smooth button polling
    }

    return 0;
}