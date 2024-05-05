
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



//// 70216C.pdf:
//// p25: Code Example for Using the PLL with the 7.37 MHz Internal FRC Oscillator
//// Select Internal FRC at POR
//_FOSCSEL(FNOSC_FRC);
//// Enable Clock Switching and Configure
//_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF);

#pragma config FNOSC = FRC              // Oscillator Mode (Internal Fast RC (FRC))
#pragma config POSCMD = NONE            // Primary Oscillator Source (XT Oscillator Mode)
#pragma config OSCIOFNC = ON            // OSC2 Pin Function (OSC2 pin has digital I/O function)
#pragma config FCKSM = CSECMD           // Clock Switching and Monitor (Clock switching is enabled, Fail-Safe Clock Monitor is disabled)

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
    
    // ==================
    // || For 80 [MHz] ||
    // ==================
    
    // M = 63 + 2 = 65
    // N1 = 1 + 2 = 3
    // N2 = 2*(0 + 1) = 2
    // M / (N1*N2) = 65 / (3*2) = 10,83333
    // Fosc = 7.37 [MHz] * 10,83333 = 79,8416421 [MHz]
    
    PLLFBDbits.PLLDIV = 63;
    CLKDIVbits.PLLPRE = 1;
    CLKDIVbits.PLLPOST = 0;

    
    // ===================
    // || For 800 [MHz] ||
    // ===================
    // We could go waaaaaaaay further than that ! 
    // IF WE COULD LOL
    
    // PLLDIV can go up to 2^9 - 1 = 513 ! --> M -> 434
    // With N1*N2 = 4 -> Fosc = 7.37 * 108.5 = 799,645 [MHz]
    
//    PLLFBDbits.PLLDIV = 432;
//    CLKDIVbits.PLLPRE = 0;
//    CLKDIVbits.PLLPOST = 0;
    
    
    // We could even go further and set FRC to a bit more !
    // 
    // FRC_wanted = 800[MHz] / 108.5 = 7,373271889400922 [MHz]
    // The amount of Hz to add is 
    // FRC_wanted - FRC_nominal = 7,373271889400922 - 7,37 = 0,003271889400922 [MHz] = 3,271889400922 [kHz]
    // It is not worth to tune the FRC via TUN<5:0>
    
    // ->  Fosc_wanted - Fosc = 800 - 799,645 = 0.355 [Mhz] of frequency difference
    // -> |Fosc_wanted - Fosc| / Fosc_wanted = 4,4*10^-6 [%] of frequency difference
    
    
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
    
    // W25Q64JV -> 6.1 Standard SPI Instructions -> Works on (0,0) and (1,1)
    SPI1CON1bits.CKE = 0;       // Serial output data changes on transition 
    SPI1CON1bits.CKP = 0;       // Idle state for clock is a low level; 
    // (CKE, CKP) -> mode 0,0

    
    // W25Q64JV -> 2. Features -> Highest Performance Serial Flash
    // 133MHz Single SPI
    // dsPIC -> "70206C - SPI Implementation.pdf" -> 18.4 MASTER MODE CLOCK FREQUENCY
    // Fsck = Fcy / ( Primary Prescaler * Secondary Prescaler )
    SPI1CON1bits.PPRE = 2;      // Primary   prescaler 4:1
    SPI1CON1bits.SPRE = 7;      // Secondary prescaler 1:1
    // Fsck = 40MHz / 4 = 10 MHz < 133 Mhz
    
    //SPI1STAT Register Settings
    SPI1CON1bits.MSTEN = 1;     // Master mode Enabled
    SPI1CON1bits.SMP = 0;       // W25Q64JV -> Sample on the falling edge a.k.a. "middle of the frame"
    SPI1STATbits.SPIEN = 1;     // Enable SPI module
    
    // Interrupt Controller Settings
    IFS0bits.SPI1IF = 0; // Clear the Interrupt Flag
    IEC0bits.SPI1IE = 1; // Enable the Interrupt
}


// Maybe setup as Slave ?
// Or a trigger from the RPi that makes us communicate as a Master ?
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
    TRISBbits.TRISB6 = 0;
    LATBbits.LATB6 = 1;
    
    // Debug Pin For SPI Step
    TRISBbits.TRISB7 = 1;
    
    
    // Setup the Clock
    setupClock();
    
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