#include "Flash.h"
#include <libpic30.h>>

uint8_t buffer_flag = 0;
uint8_t timer_flag = 0;

// Idk why, but in the original code, the routine erased the interrupt flag
// It could have caused to enter in an infinite loop as 
void __attribute__((__interrupt__,__auto_psv__)) _SPI1Interrupt(){
    buffer_flag = 1;
    _SPI1IF = 0;
    
}

void __attribute__((__interrupt__,__auto_psv__)) _T4Interrupt(){
    timer_flag = 1;
    _T4IF = 0;
}

// COMPREHENSION BASED ON 
// https://github.com/ccsrvs/SPIWinFlash/blob/master/firmware/SPIWinFlash.cpp


//inline

void delay_30us(void){
    PR4 = 0x4AF;
    T4CONbits.TON = 1;
    
    while(timer_flag == 0);
    
    timer_flag = 0;
    T4CONbits.TON = 0;
}


 volatile uint8_t FlashSendByte(uint8_t instruction) {
    
    LATBbits.LATB6 = 0;
    /*
    if (PORTBbits.RB7 == 1)
        while (PORTBbits.RB7 == 1);
    else
        while (PORTBbits.RB7 == 0);
    LATBbits.LATB6 = 1;
     */
    
     
    while(SPI1STATbits.SPITBF); // Wait for Tx buffer to be empty before writing inside it
    
    SPI1BUF = instruction; // Écrit dans le SPI1BUF, qui écrit en fait dans le SPI1TXB
    // Interrupt Quand le Shift Buffer a fait un tour. 
    // Càd quand il a envoyé et reçu (en même temps) un byte/word.
    while(buffer_flag == 0);
    // Bah ça crash meme en mode PAS DEBUG    _SPI1IF = 0;
    buffer_flag = 0;
    
    // Retourne la reçue en même temps qu'on envoyait
    return SPI1BUF;
}

inline void FlashID(void) {
    FLASH_CE_PIN_WR = 0;
    
    FlashSendByte(FI_ID);
    
    FlashSendByte(0x00);
    FlashSendByte(0x00);
    FlashSendByte(0x00);
    
    uint8_t mid = FlashRecvByte();
    uint8_t did = FlashRecvByte();
    
    FLASH_CE_PIN_WR = 1;
}


// W25Q64JV.pdf - p35
inline void FlashWriteBuffer(uint32_t address, char* buffer, uint16_t len) {
    // Start Communication
    FLASH_CE_PIN_WR = 0;
    
    FlashSendByte(FI_PAGE_PROGRAM);
    FlashSendByte((uint8_t)(address >> 16));
    FlashSendByte((uint8_t)(address >> 8));
    FlashSendByte((uint8_t)(address));
//    FlashSendByte(FI_DUMMY);
//    FlashSendByte(FI_DUMMY);
//    FlashSendByte(FI_DUMMY);
    
    
    for (uint16_t i = 0; i < len; i++){
        FlashSendByte(buffer[i]);
    }
    
    // End Communication
    FLASH_CE_PIN_WR = 1;
}



inline void FlashRecvBuffer(uint32_t address, char* buffer, uint16_t len) {
    // Start Communication
    FLASH_CE_PIN_WR = 0;
    
    FlashSendByte(FI_READ_DATA);
    FlashSendByte((uint8_t)(address >> 16));
    FlashSendByte((uint8_t)((address >> 8) & 0xFF));
    FlashSendByte((uint8_t)(address & 0xFF));
    
    for (uint16_t i = 0; i < len; i++){
        buffer[i] = FlashRecvByte();
    }
    
    Nop();
    
    // End Communication
    FLASH_CE_PIN_WR = 1;
}

inline void FlashRecvFastBuffer(uint32_t address, char* buffer, uint16_t len) {
    // Start Communication
    FLASH_CE_PIN_WR = 0;
    
    FlashSendByte(FI_FAST_READ);
    FlashSendByte(address >> 16);
    FlashSendByte(address >> 8);
    FlashSendByte(address);
    
    // Dummy byte
    FlashRecvByte();
    
    for (uint16_t i = 0; i < len; i++){
        buffer[i] = FlashRecvByte();
    }
    
    // End Communication
    FLASH_CE_PIN_WR = 1;
}

inline volatile uint8_t FlashRecvByte(void) {
    
    // while(_SPI1IF == 0);    // Wait for Rx buffer to be full before reading in it    FLASH_CE_PIN_WR = 1;
    return FlashSendByte(FI_DUMMY);
}



inline void FlashReset(void) {
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_ENABLE_RESET);
    
    FLASH_CE_PIN_WR = 1;
    //for (int i = 0; i < 40; i++); // Just to see the 0-1-0 transition
    
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_RESET_DEVICE);
    FLASH_CE_PIN_WR = 1;
    delay_30us();
}

inline void FlashInit(void) {

    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_WRITE_ENABLE);
    FLASH_CE_PIN_WR = 1;

    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_CHIP_ERASE);
    FLASH_CE_PIN_WR = 1;
}
