#include "Flash.h"
#include <libpic30.h>>

uint8_t buffer_flag = 0;
uint8_t timer_flag = 0;


void __attribute__((__interrupt__,__auto_psv__)) _SPI1Interrupt(){
    // ISR caused by the SPI module
    buffer_flag = 1;
    _SPI1IF = 0;    
}

void __attribute__((__interrupt__,__auto_psv__)) _T4Interrupt(){
    //ISR of the timer4 interrupt
    timer_flag = 1;
    _T4IF = 0;
}

// COMPREHENSION BASED ON 
// https://github.com/ccsrvs/SPIWinFlash/blob/master/firmware/SPIWinFlash.cpp


void delay_30us(void){
    PR4 = 0x4AF;
    T4CONbits.TON = 1;
    
    while(timer_flag == 0);
    
    timer_flag = 0;
    T4CONbits.TON = 0;
}

void delay_us(uint16_t time){
    PR4 = 0x27*time;
    T4CONbits.TON = 1;
    
    while(timer_flag == 0);
    
    timer_flag = 0;
    T4CONbits.TON = 0;
}


 volatile uint8_t FlashSendByte(uint8_t instruction) {
     
    uint8_t data_rx = 0x34;
    LATBbits.LATB6 = 0;
     
    while(SPI1STATbits.SPITBF); // Wait for Tx buffer to be empty before writing inside it 
    SPI1BUF = instruction; //Write in the SPI1BUF, which writes in the SPI1TXB
                            
  
    
    //Interrupts when the Shift Buffer did a cycle.
    //Meaning, when it sent and received(simultaneously) a byte/word
    while(buffer_flag == 0);
    buffer_flag = 0;
    
    //Return the received value
    return SPI1BUF;
}

inline void FlashID(void) {
    // Aks the flash for the Manufacteurer/Device ID
    // Based on the 8.3.8 section of the flash's Datasheet.
    FLASH_CE_PIN_WR = 0;
    
    FlashSendByte(FI_ID); // ID adress command 
    
    FlashSendByte(0x00);
    FlashSendByte(0x00);
    FlashSendByte(0x00);
    
    uint8_t mid = FlashRecvByte(); //Wait for the manufacutrer ID
    uint8_t did = FlashRecvByte(); //Wait for the device ID
    
    FLASH_CE_PIN_WR = 1;
    delay_us(1);
}


// W25Q64JV.pdf - p35
inline void FlashWriteBuffer(uint32_t address, char* buffer, uint16_t len) {
    // Start Communication
    FLASH_CE_PIN_WR = 0;
    
    FlashSendByte(FI_PAGE_PROGRAM);
    FlashSendByte((uint8_t)(address >> 16));
    FlashSendByte((uint8_t)(address >> 8));
    FlashSendByte((uint8_t)(address));
    
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
    //Sends clock pulses with dummy values to get the slave's response
    
    return FlashSendByte(FI_DUMMY);
}



inline void FlashReset(void) {
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_ENABLE_RESET);
    
    FLASH_CE_PIN_WR = 1;
    delay_us(1);
    
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_RESET_DEVICE);
    FLASH_CE_PIN_WR = 1;
    delay_30us(); //30 us is needed for the flash to reset itself, 
                  //no instruction will be accepted during that time (Cf Datasheet, section 6.4)
}

inline void FlashInit(void) {

    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_WRITE_ENABLE);
    FLASH_CE_PIN_WR = 1;
    delay_us(1);
    FLASH_CE_PIN_WR = 0;
    FlashSendByte(FI_CHIP_ERASE);
    FLASH_CE_PIN_WR = 1;
    delay_us(1);
}
