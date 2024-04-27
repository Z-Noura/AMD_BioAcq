
#include <xc.h>
#include <libpic30.h>


#include "Flash.h"


#define FLASH_RPN_REPRESENTATION_SDO1 0b00111
#define FLASH_RPN_REPRESENTATION_SCK1 0b01000
#define FLASH_RPN_REPRESENTATION_NSS1 0b01001


// For Better SW architecture

//typedef struct Flash {
//    uint8_t DO;
//    uint8_t DI;
//    uint8_t CS;
//    uint8_t CLK;
//} FLASH, *PFLASH;

typedef union addr {
    uint32_t addr_uint;
    struct addr_struct {
        uint16_t page;
        uint8_t offset;
    };
};

// Move each to seperate files !!!!

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




// 70216C.pdf:
// p20:
// - PLL can't be used while setting FRCDIV
//   - 1. Swicth to non-PLL mode -> 2. Change FRCDIV -> 3. Switch to PLL mode
// p21:
// - PLL Computation to find Fosc from Fin (FRC here)
// - N1 = PLLPRE + 2
// - N2 = 2 x (PLLPOST + 1)
// - M = PLLDIV + 2

inline void setupClock(void) {
    // Fosc = Fin*M/(N1*N2), where :
	//		M = PLLFBD + 2
	// 		N1 = PLLPRE + 2
	// 		N2 = 2 x (PLLPOST + 1)
    PLLFBD = 63;
    CLKDIVbits.PLLPRE = 1;
    CLKDIVbits.PLLPOST = 0;

    
    // Set FRC clock divider ...  1:1 to 1:256 -> FRCDIV<2:0> inside CLKDIV<10:8>
    
    
    
    // Set Pre and Post scaler
    
    
    
    
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



int main(void) {
    
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
    
   
    
    return 0;
}