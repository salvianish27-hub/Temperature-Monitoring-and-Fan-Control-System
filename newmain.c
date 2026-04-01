// PIC16F877A Configuration Bit Settings
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming disabled
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)
#include <xc.h>
#include <stdio.h>
// Define Oscillator Frequency for delay functions (Assuming 20MHz crystal)
#define _XTAL_FREQ 20000000
// LCD Pin Definitions mapping to your schematic
#define RS PORTBbits.RB0
#define RW PORTBbits.RB1
#define EN PORTBbits.RB2
#define LCD_DATA PORTD

// --- LCD 8-Bit Interface Functions ---
void lcd_cmd(unsigned char cmd) {
    LCD_DATA = cmd;
    RS = 0;             // Command mode
    RW = 0;             // Write mode
    EN = 1;
    __delay_ms(2);
    EN = 0;             // Latch the data
}

void lcd_data(unsigned char data) {
    LCD_DATA = data;
    RS = 1;             // Data mode
    RW = 0;             // Write mode
    EN = 1;
    __delay_ms(2);
    EN = 0;             // Latch the data
}

void lcd_init() {
    TRISB &= 0xF8;      // Set RB0, RB1, RB2 as outputs
    TRISD = 0x00;       // Set PORTD as outputs
    __delay_ms(15);     // Wait for LCD to power up
    lcd_cmd(0x38);      // 8-bit mode, 2-line display, 5x7 font
    lcd_cmd(0x0C);      // Display ON, Cursor OFF
    lcd_cmd(0x01);      // Clear display
    __delay_ms(2);
}

void lcd_string(const char *str) {
    while(*str) {
        lcd_data(*str++);
    }
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
    if(row == 1) {
        lcd_cmd(0x80 + col); // Row 1 address
    } else if(row == 2) {
        lcd_cmd(0xC0 + col); // Row 2 address
    }
}

// --- ADC Functions (For LM35) ---
void adc_init() {
    TRISAbits.TRISA0=1; // Acting as an input 
    // ADCON1: Set RA0/AN0 as analog, others digital. Right justified result. Vref = VDD (5V)
    ADCON1 = 0x8E; 
    // ADCON0: Fosc/32 clock, Channel 0 (AN0), ADC ON
    ADCON0 = 0x81; 
}
unsigned int adc_read() {
    __delay_us(20);          // Acquisition time
    ADCON0bits.GO_nDONE = 1; // Start conversion
    while(ADCON0bits.GO_nDONE); // Wait for conversion to finish
    return ((ADRESH << 8) + ADRESL); // Return 10-bit result
}
// --- PWM Functions (For Fan Control on Timer2/CCP1) ---
void pwm_init() {
    TRISCbits.TRISC2 = 0;    // Set RC2 as output
    CCP1CON = 0x0C;          // Configure CCP1 module in PWM mode
    PR2 = 124;               // Timer2 period register (sets PWM frequency to ~10kHz at 20MHz)
    T2CON = 0x04;            // Enable Timer2 with Prescaler = 1
}

// Set duty cycle from 0 (OFF) to 500 (100% ON) based on PR2=124
void pwm_set_duty(unsigned int duty) {
    CCPR1L = duty >> 2;                        // Store top 8 bits
    CCP1CON = (CCP1CON & 0xCF) | ((duty & 0x03) << 4); // Store lowest 2 bits
}

// --- Main Program Logic ---
void main() {
    adc_init();
    lcd_init();
    pwm_init();
    unsigned int adc_val;
    unsigned long temp_calc;
    unsigned int temp_whole;
    unsigned int temp_frac;
    char buffer[16];

    while(1) {
        adc_val = adc_read(); // Read sensor value on RA0
        // LM35 outputs 10mV/°C. ADC step for 5V ref and 10-bit is ~4.88mV.
        // Therefore, Temp = ADC_val * 4.88mV / 10mV = ADC_val * 0.488.
        // We use integer math (x 488) to avoid heavy floating-point libraries.
        temp_calc = (unsigned long)adc_val * 488;
        temp_whole = temp_calc / 1000;
        temp_frac = (temp_calc / 100) % 10;
        
        // Display Temperature on Line 1
        lcd_set_cursor(1, 0);
        sprintf(buffer, "Temp: %u.%u C   ", temp_whole, temp_frac);
        lcd_string(buffer);
        
        // Fan Control Logic on Line 2
        lcd_set_cursor(2, 0);
        
        if (temp_whole < 35) {
            pwm_set_duty(0);         // 0% Duty Cycle
            lcd_string("Fan: OFF        ");
        } 
        else if (temp_whole >= 35 && temp_whole < 40) {
            pwm_set_duty(200);       // ~40% Duty Cycle (Low speed)
            lcd_string("Fan: LOW        ");
        } 
        else if (temp_whole >= 40 && temp_whole < 45) {
            pwm_set_duty(350);       // ~70% Duty Cycle (Medium speed)
            lcd_string("Fan: MEDIUM     ");
        } 
        else {
            pwm_set_duty(500);       // 100% Duty Cycle (High speed)
            lcd_string("Fan: HIGH       ");
        }
        
        __delay_ms(300); // Update display every 300ms
    }
}