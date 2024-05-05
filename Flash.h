/* 
 * File:   Flash.h
 * Author: airko
 *
 * Created on 10 avril 2024, 13:31
 */

#ifndef FLASH_H
#define	FLASH_H


#include <xc.h>
#include <libpic30.h>

#ifdef	__cplusplus
extern "C" {
#endif
    
    #define FLASH_SDI_PIN_WR    _LATB2
    #define FLASH_SDO_PIN_WR    _LATB3
    #define FLASH_SCK_PIN_WR    _LATB4
    #define FLASH_CE_PIN_WR     _LATB5

    #define FI_DUMMY         0xFF
    #define FI_WRITE_ENABLE  0x06
    #define FI_WRITE_DISABLE 0x04
    #define FI_READ_DATA     0x03 // Multibyte (ADDR<23:0> : D<7-0>)
    #define FI_FAST_READ     0x0B // Multibyte (ADDR<23:0> : dummy<7:0> : D<7-0>)
    #define FI_PAGE_PROGRAM  0x02 // Multibyte (ADDR<23:0> : D<7-0> (: 255*D<7-0>) )

    #define FI_POWER_DOWN           0xB9
    #define FI_RELEASE_POWER_DOWN   0xAB // 3 dummy bytes before

    #define FI_CHIP_ERASE   0x60
    #define FI_ENABLE_RESET 0x66
    #define FI_RESET_DEVICE 0x99

    #define FI_READ_STATUS_1    0x05
    #define FI_READ_STATUS_2    0x35
    #define FI_READ_STATUS_3    0x15
    #define FI_WRITE_STATUS_1   0x01
    #define FI_WRITE_STATUS_2   0x31
    #define FI_WRITE_STATUS_3   0x11
    
    
    void __attribute__((__interrupt__,__auto_psv__)) _SPI1Interrupt();
    
    // Send
    volatile uint8_t FlashSendByte(uint8_t instruction);
    void FlashWriteBuffer(uint32_t address, char* buffer, uint16_t len);
    
    // Receive
    volatile uint8_t FlashRecvByte(void);
    void FlashRecvFastBuffer(uint32_t address, char* buffer, uint16_t len);
    void FlashRecvBuffer(uint32_t address, char* buffer, uint16_t len);
    
    
    // Control
    void FlashReset(void);
    void FlashInit(void);
    


#ifdef	__cplusplus
}
#endif

#endif	/* FLASH_H */

