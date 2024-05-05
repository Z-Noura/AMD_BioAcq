#include <xc.h>
#include "buffer.h"
#include "spiAdc.h"

#define BUFFER_SIZE 186 // 2 bytes for metadata + 92 bytes for ADC data
#define BUFFER_NUMBER 80

// Definitions for pin and other settings may need to be adjusted to match your actual setup
uint8_t buffer[BUFFER_NUMBER][BUFFER_SIZE];
uint16_t bufIndex = 0;
uint16_t curBuffer = 0;
uint16_t bufCount = 0;
uint8_t status = 0;
uint8_t channel= 0;
uint16_t periodNumber = 0;

void bufInit() {
    curBuffer = 0;
    bufCount = 0;
    status = 0;
    buffer[curBuffer][0] = bufCount++; // Store buffer count
    buffer[curBuffer][1] = status; // Store status
    bufIndex = 2; // Start storing data from index 2
}

int ADC(void) {
    spiAdcInit(); // Initialize ADC and SPI settings
    bufInit(); // Initialize buffer system

    while (1) {
        if (bufIndex < BUFFER_SIZE) {
            
            uint16_t sample = adcSample(CHANNEL0);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            
            sample = adcSample(CHANNEL1);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            
            if (periodNumber % 20 == 0)
            {
            sample = adcSample(CHANNEL2);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            
            sample = adcSample(CHANNEL3);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            
            sample = adcSample(CHANNEL4);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            
            sample = adcSample(CHANNEL5);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            
            sample = adcSample(CHANNEL6);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            
            sample = adcSample(CHANNEL7);
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample >> 8); // Store MSB
            buffer[curBuffer][bufIndex++] = (uint8_t)(sample & 0xFF); // Store LSB
            }
        } 
        else {
            // Switch to the next buffer
            curBuffer = (curBuffer + 1) % BUFFER_NUMBER;   
            bufInit(); // Reinitialize buffer system
        }
    }
}