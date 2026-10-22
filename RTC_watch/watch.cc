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

#define DS3231_ADDR 0xD0 // 0x68 shifted left by 1 bit for I2C write (0x68 << 1)

// ============================================================================
// HARDWARE PIN MAPPING DEFINITIONS
// ============================================================================

// Segment Pins
// Segment A -> PD3
#define SEG_A_PORT   PORTD
#define SEG_A_DDR    DDRD
#define SEG_A_PIN    PD3

// Segment B -> PD4
#define SEG_B_PORT   PORTD
#define SEG_B_DDR    DDRD
#define SEG_B_PIN    PD4

// Segment C -> PB6
#define SEG_C_PORT   PORTB
#define SEG_C_DDR    DDRB
#define SEG_C_PIN    PB6

// Segment D -> PB7
#define SEG_D_PORT   PORTB
#define SEG_D_DDR    DDRB
#define SEG_D_PIN    PB7

// Segment E -> PD5
#define SEG_E_PORT   PORTD
#define SEG_E_DDR    DDRD
#define SEG_E_PIN    PD5

// Segment F -> PD6
#define SEG_F_PORT   PORTD
#define SEG_F_DDR    DDRD
#define SEG_F_PIN    PD6

// Segment G -> PD7
#define SEG_G_PORT   PORTD
#define SEG_G_DDR    DDRD
#define SEG_G_PIN    PD7

// Decimal Point -> PB0
#define SEG_DP_PORT  PORTB
#define SEG_DP_DDR   DDRB
#define SEG_DP_PIN   PB0

// Digit Selection Pins (Driven HIGH to turn on AO3400 Low-Side MOSFETs)
// Digit 1 -> PC0
#define DIG1_PORT    PORTC
#define DIG1_DDR     DDRC
#define DIG1_PIN     PC0

// Digit 2 -> PC1
#define DIG2_PORT    PORTC
#define DIG2_DDR     DDRC
#define DIG2_PIN     PC1

// Digit 3 -> PC2
#define DIG3_PORT    PORTC
#define DIG3_DDR     DDRC
#define DIG3_PIN     PC2

// Digit 4 -> PC3
#define DIG4_PORT    PORTC
#define DIG4_DDR     DDRC
#define DIG4_PIN     PC3

// Status LED Pin
#define LED_PORT     PORTB
#define LED_DDR      DDRB
#define LED_PIN      PB1

// ============================================================================
// GLOBAL DISPLAY BUFFERS & LOOKUP TABLES
// ============================================================================

// Volatile digit buffer updated by main loop and read by ISR
volatile uint8_t g_display_digits[4] = {0, 0, 0, 0};

// 7-Segment Active-HIGH encoding for numbers 0-9 (Bit 0 = A, Bit 1 = B, ..., Bit 6 = G)
static const uint8_t segment_font[10] = {
    0b00111111, // 0 (A, B, C, D, E, F)
    0b00000110, // 1 (B, C)
    0b01011011, // 2 (A, B, D, E, G)
    0b01001111, // 3 (A, B, C, D, G)
    0b01100110, // 4 (B, C, F, G)
    0b01101101, // 5 (A, C, D, F, G)
    0b01111101, // 6 (A, C, D, E, F, G)
    0b00000111, // 7 (A, B, C)
    0b01111111, // 8 (A, B, C, D, E, F, G)
    0b01101111  // 9 (A, B, C, D, F, G)
};

// --- UART Functions ---
void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    
    UCSR0B = (1 << TXEN0);                  // Enable transmitter
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 data bits, 1 stop bit
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));       // Wait for empty transmit buffer
    UDR0 = data;
}

void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

// --- Hardware I2C (TWI) Functions ---
void i2c_init(void) {
    TWSR = 0x00; // Prescaler = 1
    TWBR = 32;   // SCL frequency = 100kHz at 8MHz F_CPU
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

// Write initial Date & Time to DS3231 registers
void ds3231_set_time(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec) {
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x00); // Start at Seconds register

    i2c_write(dec_to_bcd(sec));
    i2c_write(dec_to_bcd(min));
    i2c_write(dec_to_bcd(hour) & 0x3F); // 24-hour mode
    i2c_write(dec_to_bcd(1));            // Day of week placeholder
    i2c_write(dec_to_bcd(date));
    i2c_write(dec_to_bcd(month) & 0x1F);
    i2c_write(dec_to_bcd(year % 100));   // Store last 2 digits of year

    i2c_stop();
}

// Check if oscillator stopped (power lost) and initialize if needed
void ds3231_check_power_loss(void) {
    // 1. Enable oscillator (Clear EOSC bit in Control Reg 0x0E)
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x0E);
    i2c_write(0x00);
    i2c_stop();

    // 2. Read Status Register (0x0F) to check OSF (Bit 7)
    i2c_start();
    i2c_write(DS3231_ADDR);
    i2c_write(0x0F);

    i2c_start();
    i2c_write(DS3231_ADDR | 0x01);
    uint8_t status = i2c_read_nack();
    i2c_stop();

    // OSF (Bit 7) = 1 means power was lost or oscillator stopped
    if (status & (1 << 7)) {
        uart_print("Power loss detected! Setting default time...\r\n");

        // Set default initial date & time: 2026-10-17 07:53:40
        ds3231_set_time(2026, 10, 20, 22, 0, 0);
        
        // Clear OSF bit in Status Register
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

// Configure GPIO direction for all segments and digit selection pins
void display_init(void) {
    // Set Segment pins as Outputs
    SEG_A_DDR  |= (1 << SEG_A_PIN);
    SEG_B_DDR  |= (1 << SEG_B_PIN);
    SEG_C_DDR  |= (1 << SEG_C_PIN);
    SEG_D_DDR  |= (1 << SEG_D_PIN);
    SEG_E_DDR  |= (1 << SEG_E_PIN);
    SEG_F_DDR  |= (1 << SEG_F_PIN);
    SEG_G_DDR  |= (1 << SEG_G_PIN);
    SEG_DP_DDR |= (1 << SEG_DP_PIN);

    // Set Digit selection pins as Outputs
    DIG1_DDR |= (1 << DIG1_PIN);
    DIG2_DDR |= (1 << DIG2_PIN);
    DIG3_DDR |= (1 << DIG3_PIN);
    DIG4_DDR |= (1 << DIG4_PIN);

    // Ensure all digits start OFF (MOSFET Gates LOW)
    DIG1_PORT &= ~(1 << DIG1_PIN);
    DIG2_PORT &= ~(1 << DIG2_PIN);
    DIG3_PORT &= ~(1 << DIG3_PIN);
    DIG4_PORT &= ~(1 << DIG4_PIN);
}

// Turn off all display segments and disable all digits (prevents ghosting)
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

// Map a 7-segment bit pattern to the mapped GPIO output pins
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

// Configure Timer1 for ~250Hz Compare Match Interrupt (CTC Mode)
void timer1_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, Prescaler = 64
    OCR1A = 499; // (8,000,000 / (64 * 250)) - 1 = 499 (250Hz refresh rate)
    TIMSK1 = (1 << OCIE1A); // Enable Compare Match A Interrupt
}

// Timer1 Interrupt Service Routine (ISR) for Multiplex Scanning
ISR(TIMER1_COMPA_vect) {
    static uint8_t current_digit = 0;

    // 1. Clear current digit to prevent ghosting
    display_clear_all();

    // 2. Fetch value and pattern for active digit
    uint8_t val = g_display_digits[current_digit];
    uint8_t pattern = segment_font[val % 10];

    // Optional: Light DP on Digit 2 (HH.MM) as time separator
    if (current_digit == 1) {
        pattern |= (1 << 7);
    }

    // 3. Write segment state to output pins
    display_set_segments(pattern);

    // 4. Enable the single active digit MOSFET
    switch (current_digit) {
        case 0: DIG1_PORT |= (1 << DIG1_PIN); break;
        case 1: DIG2_PORT |= (1 << DIG2_PIN); break;
        case 2: DIG3_PORT |= (1 << DIG3_PIN); break;
        case 3: DIG4_PORT |= (1 << DIG4_PIN); break;
    }

    // 5. Advance to next digit
    current_digit = (current_digit + 1) % 4;
}

// ============================================================================
// MAIN APPLICATION
// ============================================================================

int main(void) {   
    char buffer[48];
    uint8_t sec, min, hour, day, date, month, year_short;

    // Configure Status LED on PB1
    LED_DDR |= (1 << LED_PIN);

    // Initialize display and communication peripherals
    display_init();
    timer1_init();
    uart_init(MYUBRR);
    i2c_init();

    // Enable global interrupts for display multiplexing
    sei();

    // Check power status & auto-initialize time if power was removed
    ds3231_check_power_loss();

    uart_print("\r\n--- DS3231 + 4-Digit Display Watch Ready ---\r\n");

    while (true) {
        LED_PORT &= ~(1 << LED_PIN); // Set PB1 LOW
        _delay_ms(500);

        LED_PORT |= (1 << LED_PIN);  // Set PB1 HIGH
        _delay_ms(500);

        // 1. Set register pointer to 0x00 (Seconds)
        i2c_start();
        i2c_write(DS3231_ADDR);
        i2c_write(0x00);

        // 2. Read 7 registers sequentially
        i2c_start();
        i2c_write(DS3231_ADDR | 0x01);
        sec        = bcd_to_dec(i2c_read_ack() & 0x7F);
        min        = bcd_to_dec(i2c_read_ack());
        hour       = bcd_to_dec(i2c_read_ack() & 0x3F);
        day        = bcd_to_dec(i2c_read_ack());
        date       = bcd_to_dec(i2c_read_ack());
        month      = bcd_to_dec(i2c_read_ack() & 0x1F);
        year_short = bcd_to_dec(i2c_read_nack());
        i2c_stop();

        // 3. Update global 4-digit HHMM display buffer
        g_display_digits[0] = hour / 10;
        g_display_digits[1] = hour % 10;
        g_display_digits[2] = min / 10;
        g_display_digits[3] = min % 10;

        // 4. Format and output date and time over UART
        snprintf(buffer, sizeof(buffer), "Date: 20%02d-%02d-%02d | Time: %02d:%02d:%02d\r\n", 
                 year_short, month, date, hour, min, sec);
        uart_print(buffer);
    }

    return 0;
}