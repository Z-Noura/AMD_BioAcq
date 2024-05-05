#include "Flash.h"

// Idk why, but in the original code, the routine erased the interrupt flag
// It could have caused to enter in an infinite loop as 
void __attribute__((__interrupt__,__auto_psv__)) _SPI1Interrupt(){
    _SPI1IF = 0;
}


// COMPREHENSION BASED ON 
// https://github.com/ccsrvs/SPIWinFlash/blob/master/firmware/SPIWinFlash.cpp


//inline
 void FlashSendByte(uint8_t instruction) {
    // Is never sent from  SPI1TXB to SPI1SB ... 
    // Or at least never sets the SPITBF bit back to 0 when sending
    while(SPI1STATbits.SPITBF == 1) {
        Nop();
    }    // Wait for Tx buffer to be empty before writing inside it
    
    // Écrit dans le SPI1BUF, qui écrit en fait dans le SPI1TXB
    SPI1BUF = instruction;
    
    //// Attend le temps d'un envoi (prend plus que 8 instr clock pulse)
    //for (short i = 0; i < 8; i++) 
    //    Nop();
    
    
    // Interrupt Quand le Shift Buffer a fait un tour. 
    // Càd quand il a envoyé et reçu (en même temps) un byte/word.
    // Ne finit jamais... 
    // à croire qu'il ne transfère jamais dans le SPI1SB
    while(IFS0bits.SPI1IF == 0) {
        Nop();
    }
    //Nop();
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
    FlashSendByte((uint8_t)(address >> 8));
    FlashSendByte((uint8_t)(address));
    
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

inline uint8_t FlashRecvByte(void) {
    FlashSendByte(FI_DUMMY);
    // while(_SPI1IF == 0);    // Wait for Rx buffer to be full before reading in it    FLASH_CE_PIN_WR = 1;
    return SPI1BUF;
}



inline void FlashReset(void) {
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_ENABLE_RESET);
    FLASH_CE_PIN_WR = 1;
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_RESET_DEVICE);
    FLASH_CE_PIN_WR = 1;
}

inline void FlashInit(void) {
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_WRITE_ENABLE);
    FLASH_CE_PIN_WR = 1;
    
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_CHIP_ERASE);
    FLASH_CE_PIN_WR = 1;
}
