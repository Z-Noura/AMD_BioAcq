#include "xc.h"
#include <stdint.h>
#include <libpic30.h>
#include "initPIC.h"


static int a = 0;
void __attribute__((__interrupt__,__no_auto_psv__)) _T3Interrupt() {
    if (a == 0){a = 1;}
    else {a = 0;}
    PORTBbits.RB6 = a;
    AD1CON1bits.SAMP = 1;
    float buffer;
    buffer = ADC1BUF0;
    
    IFS0bits.T3IF = 0; // Clear Timer 3 interrupt flag    
}

int main(void) {
    TRISBbits.TRISB6 = 0;
    PORTBbits.RB6 = 1;
    initTimer();
    initADC();
    
    float ADCValue;
    for(;;){
        
        AD1CON1bits.SAMP = 1; // Start sampling
        __delay_us(10); // Wait for sampling time (10us)
        AD1CON1bits.SAMP = 0; // Start the conversion
        while (!AD1CON1bits.DONE); // Wait for the conversion to complete
        ADCValue = ADC1BUF0; // Read the conversion result
        
        Idle();
    }
    return 0;
}




