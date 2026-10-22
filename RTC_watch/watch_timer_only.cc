#ifndef F_CPU
#define F_CPU 8000000UL // 8MHz internal RC oscillator
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>

#define BAUD 9600
#define MYUBRR (F_CPU / 16 / BAUD - 1)

#define DS3231_ADDR 0xD0 // 0x68 shifted left by 1 bit for I2C write (0x68 << 1)

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
    i2c_write(dec_to_bcd(1));            // Day of week (1-7, placeholder)
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

        // Set default initial date & time: 2026-10-16 20:30:00
        ds3231_set_time(2026, 10, 17, 7, 53, 40);
        
        // Clear OSF bit in Status Register
        i2c_start();
        i2c_write(DS3231_ADDR);
        i2c_write(0x0F);
        i2c_write(status & ~(1 << 7));
        i2c_stop();
    }
}

int main(void) {   
    char buffer[48];
    uint8_t sec, min, hour, day, date, month, year_short;

    // Configure PB1 (ATmega328 Pin 15) as output
    DDRB |= (1 << PB1);

    // Initialize peripherals
    uart_init(MYUBRR);
    i2c_init();

    // Check power status & auto-initialize time if power was removed
    ds3231_check_power_loss();

    uart_print("\r\n--- DS3231 Bare-Metal Reader ---\r\n");

    while (true) {
        PORTB &= ~(1 << PB1); // Set PB1 LOW
        _delay_ms(500);

        PORTB |= (1 << PB1);  // Set PB1 HIGH
        _delay_ms(500);

        // 1. Set register pointer to 0x00 (Seconds)
        i2c_start();
        i2c_write(DS3231_ADDR);
        i2c_write(0x00);

        // 2. Read 7 registers sequentially: Sec, Min, Hour, Day, Date, Month, Year
        i2c_start();
        i2c_write(DS3231_ADDR | 0x01); // Read mode
        sec        = bcd_to_dec(i2c_read_ack() & 0x7F);
        min        = bcd_to_dec(i2c_read_ack());
        hour       = bcd_to_dec(i2c_read_ack() & 0x3F);
        day        = bcd_to_dec(i2c_read_ack()); // Day of week
        date       = bcd_to_dec(i2c_read_ack());
        month      = bcd_to_dec(i2c_read_ack() & 0x1F);
        year_short = bcd_to_dec(i2c_read_nack());
        i2c_stop();

        // 3. Format and output date and time over UART
        snprintf(buffer, sizeof(buffer), "Date: 20%02d-%02d-%02d | Time: %02d:%02d:%02d\r\n", 
                 year_short, month, date, hour, min, sec);
        uart_print(buffer);
    }

    return 0;
}