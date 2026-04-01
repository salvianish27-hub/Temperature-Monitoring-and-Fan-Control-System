/*
 * File:   ADC.c
 * Author: Anish Salvi
 *
 * Created on 25 March, 2026, 6:26 PM
 */
// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)
#include <xc.h>
#include <pic16f877a.h>
#define _XTAL_FREQ 2000000
#define RS PORTBbits.RB0
#define RW PORTBbits.RB1
#define E PORTBbits.RB2
unsigned float adc=0;
unsigned int temp_c;
unsigned int a,b;
void PWM_Init();
void PWM_Set_Duty(unsigned int duty_val);
void lcd_cmd(unsigned char cmd){
    PORTD=cmd;
    RS=0; // Writing commands 
    RW=0; // Writing on LCD
    E=1;
   __delay_ms(10);
    E=0;
}
void lcd_data(unsigned char data){
    PORTD=data;
    RS=1; // Writing Data
    RW=0; // Writing on LCD
    E=1;
   __delay_ms(10);
    E=0;
}
void lcd_init(){
    
   __delay_ms(10);
   lcd_cmd(0x38); // Writing is in 8 bit mode 
   lcd_cmd(0x0C); //display on cursor off
   __delay_ms(10);
}
void lcd_string(char *a){
   int i=0;
   while(a[i]!='\0')
   {
       lcd_data(a[i]);
       i++;
   }
    __delay_ms(10);
}
void ADC_config()
{
    TRISAbits.TRISA0=1;
    /* Initialization of RA0 as the input port to get ADC value */
    ADCON1=0X8E; //10001110
    /*Bit 7: ADFM = 1 (A/D Result Format Select) 1 = Right Justified (ADRESH << 8) | ADRESL */
    /*Bit 6: ADCS2 = 0 (A/D Conversion Clock Select) */
    /*Bits 5?4: Unimplemented = 00*/
    /*Bits 3?0: PCFG3:PCFG0 = 1110 (Port Configuration Control)*/
    ADCON0=0X41;  //0100 000 
    /* Bits 7?6: ADCS1:ADCS0 = 01 (A/D Conversion Clock) 01 selects F_{OSC}/8. */
    /* Bits 5?3: CHS2:CHS0 = 000 (Analog Channel Select)*/
    /* Bit 2: GO/DONE = 0 (A/D Conversion Status) o is idel not converting*/
    /* Bit 1: Unimplemented = 0 Not used by Hardware*/
    /* Bit 0: ADON = 1 (ADC On/Off) */
    ADCON0=ADCON0|0x04;
    /* ADCON0= 0100 0000 | 0000 0100 =0100 0100 ORing Operation */  
}
void ADC_interrupt()
{
    // To deal with any interrupt INTCON must be looked into
    INTCONbits.GIE=1;  // Enable Global interrupt 
    INTCONbits.PEIE=1; // Enable Peripheral interrupt 
    PIE1bits.ADIE = 1; //Peripheral Interrupts Enable Register with ADC which enables ADC conversion
}

void __interrupt() adc_value()  // Creating interrupt based ADC 
{
   if(PIR1bits.ADIF==1) // Raise the flag from PIR1 register after ADC complete 
   { 
       adc=(ADRESH<<8)|ADRESL;//10BIT CONVERSION
       temp_c = (int)(adc*0.488);
       a =(temp_c/ 10);      // Get the 3
       b=(temp_c% 10);
       PIR1bits.ADIF=0;//CLEAR FLAG
   }
}
void main(void)
{
    PWM_Init();
    TRISB=0x00;
    TRISD=0x00;
    lcd_init();
    lcd_cmd(0x80);
    lcd_string("Temp Value:");
    ADC_config();
    ADC_interrupt();
    while(1) 
    {    
    lcd_cmd(0Xc6);
    lcd_data(a+0x30);     // Displays 'Ten Place Digit'
    lcd_data(b+0x30);    // Displays 'Unit Place Digit'
    lcd_data(0xDF); 
    }
    return;
}


