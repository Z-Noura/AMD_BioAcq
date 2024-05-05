#include "xc.h"
#include <stdint.h>
#include "libpic30.h"
#include "initPIC.h"
#include "hardConfig.h"
#include "spiAdc.h"
#include "spiRpi.h"
#include "Flash.h"
#define FCY 40000000

#define FLASH_RPN_REPRESENTATION_SDO1 0b00111
#define FLASH_RPN_REPRESENTATION_SCK1 0b01000
#define FLASH_RPN_REPRESENTATION_NSS1 0b01001


uint16_t bufIndex;
uint16_t curBuffer;
uint16_t bufCount;
uint8_t status;

void bufInit() {
    bufToSend = 0;
    curBuffer = 0;
    bufCount = 0;
    status = NO_ERROR;
    buffer[curBuffer][0] = bufCount++;
    buffer[curBuffer][1] = status;
    bufIndex = 2;
}


inline void setupClock(void) {
    // Fosc = Fin*M/(N1*N2), where :
	//		M = PLLFBD + 2
	// 		N1 = PLLPRE + 2
	// 		N2 = 2 x (PLLPOST + 1)
    PLLFBD = 63;
    CLKDIVbits.PLLPRE = 1;
    CLKDIVbits.PLLPOST = 0; 
    
    
	// Initiate Clock Switch to FRC with PLL
    
    // Set NOSC<2:0> ... Choose FRC for the New Oscillator
	__builtin_write_OSCCONH( 0x01 );
    
    // Set OSWEN ... Request Oscillator Switch To New Oscillator (NOSC<2:0>)
	__builtin_write_OSCCONL( OSCCON | 0x01 );
    
    
	// Wait for Clock switch to occur
    while (OSCCONbits.COSC != 0b001);
    
    // Wait for PLL to lock (PLL has started up)
    while(OSCCONbits.LOCK != 1);
}


// 70206C.pdf - 18.3.2.1.1 Master Mode Setup Procedures
inline void SetupSPI1Register(void) {
    IFS0bits.SPI1IF = 0; // Clear the Interrupt Flag
    IEC0bits.SPI1IE = 0; // Disable the Interrupt
    
    // SPI1CON1 Register Settings
    SPI1CON1bits.DISSCK = 0;    // Internal Serial Clock is Enabled
    SPI1CON1bits.DISSDO = 0;    // SDOx pin is controlled by the module
    SPI1CON1bits.MODE16 = 0;    // Communication is byte-wide (8 bits)
    SPI1CON1bits.SMP = 0;       // Input data is sampled at the middle of data
    SPI1CON1bits.CKE = 0;       // Serial output data changes on transition 
    SPI1CON1bits.CKP = 0;       // Idle state for clock is a low level; 
    // (CKE, CKP) -> mode 0,0
    SPI1CON1bits.MSTEN = 1;     // Master mode Enabled
    
    //SPI1STAT Register Settings
    SPI1STATbits.SPIEN = 1;     // Enable SPI module
    
    // SPI1BUF = 0x0000;
    
    // Interrupt Controller Settings
    IFS0bits.SPI1IF = 0; // Clear the Interrupt Flag
    IEC0bits.SPI1IE = 1; // Enable the Interrupt
}

inline void SetupSPI2Register(void) {
    IFS2bits.SPI2IF = 0; // Clear the Interrupt Flag
    IEC2bits.SPI2IE = 0; // Disable the Interrupt
    
    // SPI1CON1 Register Settings
    SPI2CON1bits.DISSCK = 0;    // Internal Serial Clock is Enabled
    SPI2CON1bits.DISSDO = 0;    // SDOx pin is controlled by the module
    SPI2CON1bits.MODE16 = 1;    // Communication is word-wide (16 bits)
    SPI2CON1bits.SMP = 0;       // Input data is sampled at the middle of data
    SPI2CON1bits.CKE = 0;       // Serial output data changes on transition 
    SPI2CON1bits.CKP = 0;       // Idle state for clock is a low level; 
    // (CKE, CKP) -> mode 0,0
    SPI2CON1bits.MSTEN = 1;     // Master mode Enabled
    
    //SPI1STAT Register Settings
    SPI2STATbits.SPIEN = 1;     // Enable SPI module
    
    // Zeroing the 16bit data
    // --> Could be sending already
    SPI2BUF = 0x0000;
    
    // Interrupt Controller Settings
    IFS2bits.SPI2IF = 0; // Clear the Interrupt Flag
    IEC2bits.SPI2IE = 1; // Enable the Interrupt
}


// Setup AND Connect the SPI PORT
inline void SetupADC(void) {
    
}



inline void SetupFlashPins(void) {
    TRISBbits.TRISB2 = 1;           // Set 2nd pin as an Input pin
    TRISB &= 0b1111111111000111;    // Set the 3rd, 4th and 5th pins as Output pins
    FLASH_CE_PIN_WR = 1;
}

inline void SetupRPIPins(void) {
    
}



inline void ConnectSPI1ToFlash(void) {
    // Check for lock
    RPINR20bits.SDI1R = 2;                              // Bind RP2, alias RB2 to SDI1
    RPOR1bits.RP3R = FLASH_RPN_REPRESENTATION_SDO1;     // Bind RP3, alias RB3 to SDO1
    RPOR2bits.RP4R = FLASH_RPN_REPRESENTATION_SCK1;     // Bind RP4, alias RB4 to SCK1
    RPOR2bits.RP5R = FLASH_RPN_REPRESENTATION_NSS1;     // Bind RP5, alias RB5 to ~SS1
}

inline void ConnectSPI1ToRPI(void) {
    
}


// Initialize the Flash via Instructions
inline void InitFlash(void) {
    FlashReset();
    FlashInit();
}


static int a = 0;
void __attribute__((__interrupt__,__no_auto_psv__)) _T3Interrupt() {
    if (a == 0){a = 1;}
    else {a = 0;}
    PORTBbits.RB6 = a;
    AD1CON1bits.SAMP = 1;
    float buffer;
    IFS0bits.T3IF = 0; // Clear Timer 3 interrupt flag    
}

int main(void) {
    initTimer();
    initADC();
    
    
    uint16_t periodCount = 0;
    uint16_t newSample;
    uint16_t ledCount, ledMax;

    
    // configure FCY at 40MHz
	frcPllConfig();
    // LED PIN configuration
    MAIN_LOOP_LED = 0;
    MAIN_LOOP_LED_TRIS = 0;
    ACQ_LED = 0;
    ACQ_LED_TRIS = 0;
    // configure all pins in digital mode
    AD1PCFGL = ANALOG_PINS_CFG;
    // RPI pins configuration
    RPI_TRIG_TRIS = 0;
    RPI_TRIG = 0;
    RPI_DISABLE_TRIS = 1;
    RPI_DISABLE_PUE = 1;
    // ISR priorities configuration
    //INTCON1bits.NSTDIS = 1;
    _SPI1IP = 6;
    _SPI2IP = 5;
    _T2IP = 4;
    // SPI busses configuration
    spiAdcInit();
    // ADC sampling frequency configuration
    T2CONbits.TCKPS = 0;        // prescaler = 1:1 => PR2 unit = 25nsec
    PR2 = 1999;                  // Period = 2000*25ns = 50us
    _T2IF = 0;

    T3CONbits.TCKPS = 3;        // prescaler = 1:256 => PR3 unit = 6.4usec
    PR3 = 5*7811u;              // 6.4us*7812 = 49.9968ms
    T3CONbits.TON = 1;
    
    ledMax = 6;
    ledCount = 0;
    bufInit();
    newSample = 0;
    periodCount = 0;
	while(1) {
        if (_T2IF) {
            _T2IF = 0;
            MAIN_LOOP_LED = 1;
//            newSample = adcSample(CHANNEL0);
            // Acquire channel 0 and set CHANNEL1 as next to be acquired
            newSample = adcSample(CHANNEL1);
            buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
            buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
            if (periodCount != 0) {     // Usually, we only acquire CHANNEL0 and CHANNEL1
                // Acquire CHANNEL1 and set CHANNEL0 as next to be acquired
                newSample = adcSample(CHANNEL0);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
            } else {    // each 20 sampling period, we acquire all channels
                // Acquire CHANNEL1 and set CHANNEL2 as next to be acquired
                newSample = adcSample(CHANNEL2);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
                // Acquire CHANNEL2 and set CHANNEL3 as next to be acquired
                newSample = adcSample(CHANNEL3);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
                // Acquire CHANNEL3 and set CHANNEL4 as next to be acquired
                newSample = adcSample(CHANNEL4);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
				// Acquire CHANNEL4 and set CHANNEL5 as next to be acquired
                newSample = adcSample(CHANNEL5);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
				// Acquire CHANNEL5 and set CHANNEL6 as next to be acquired
                newSample = adcSample(CHANNEL6);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
				// Acquire CHANNEL6 and set CHANNEL7 as next to be acquired
                newSample = adcSample(CHANNEL7);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
				// Acquire CHANNEL7 and set CHANNEL0 as next to be acquired
                newSample = adcSample(CHANNEL0);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample >> 8);
                buffer[curBuffer][bufIndex++] = (uint8_t)(newSample & 0x00FF);
            }
            if (++periodCount > 19) {
                periodCount = 0;
            }
            MAIN_LOOP_LED = 0;
            
            if (bufIndex >= BUFFER_SIZE) {
                ACQ_LED = 1;
                if (periodCount !=0) {
                    //ACQ_LED = 1;
                    status = PERIOD_ERR;
                    periodCount = 0;
                } else {
                    status = NO_ERROR;
                }
                if (++curBuffer > BUFFER_NUMBER-1) {
                    curBuffer = 0;
                }
                buffer[curBuffer][0] = bufCount++;
                buffer[curBuffer][1] = status;
                bufIndex = 2;
                bufToSend++;
                if (bufToSend >= BUFFER_NUMBER) {
                    bufToSend = BUFFER_NUMBER;
                    status |= BUFFER_ERR;
                    //ACQ_LED = 1;
                }
                RPI_TRIG = 1;
                ACQ_LED = 0;
            }
        }
        
         if (RPI_DISABLE == 0) {
            T2CONbits.TON = 1;
            ledMax = 2;
        } else {
            T2CONbits.TON = 0;
            ledMax = 6;
            bufInit();
            RPI_TRIG = 0;
            periodCount = 0;
        }
	}
    
    // LED STATUS
    TRISBbits.TRISB15 = 0;
    LATBbits.LATB15 = 1;
    
    // Gloabl insterrupt enable
    
    
    // Setup SPI Modules
    SetupSPI1Register();
    SetupSPI2Register();
    
    // Setup ADC (pins I/O + bindings)
    SetupADC();
    
    // Setup Flash SPI-pins I/O
    SetupFlashPins();
    // Setup RPI SPI-pins I/O
    SetupRPIPins();
    
    // Connect SPI1 Module pins to dsPIC I/O pins
    ConnectSPI1ToFlash();
    // Send Setup Instructions To Flash
    InitFlash();
   
    // DEPRECATED : Setup the pins used by the SPI
    // SetupSPI1IOPins();
    
    uint32_t addr = 0xFF111100;
    uint16_t page;
    uint8_t offset;
    char buffer[5] = {0};
    
    for (uint8_t i = 0; i < 5; i++)
        buffer[i] = i;
    
    FlashWriteBuffer(addr, (char*)buffer, 5);
    
    char rcvBuffer[5] = {0};
    
    FlashRecvBuffer(addr, (char*)rcvBuffer, 5);
    
    
    Nop();
    
    

    for (;;)
    {
        Nop();
        Nop();
    }
}

typedef union addr {
    uint32_t addr_uint;
    struct addr_struct {
        uint16_t page;
        uint8_t offset;
    };
};

// Move each to seperate files !!!!
/*
void setupClock(void);

void SetupSPI1Register(void);
void SetupSPI2Register(void);
void SetupADC(void);
void SetupFlashPins(void);
void SetupRPIPins(void);
void ConnectSPI1ToFlash(void);
void ConnectSPI1ToRPI(void);
void SetupSPI1IOPins(void);
void InitFlash(void);
 */