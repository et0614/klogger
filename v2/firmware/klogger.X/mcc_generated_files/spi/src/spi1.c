/**
 * SPI1 Generated Driver File
 *
 * @file spi1.c
 *
 * @ingroup spi1
 *
 * @brief This file contains the driver code for the SPI1 module.
 *
 * @version SPI1 Driver Version 3.1.0
 *  
 * @version SPI1 Package Version 5.1.0
*/
/*
? [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/


#include "../spi1.h"
#include "../spi_polling_types.h"

const struct SPI_INTERFACE SPI1_Host = 
{
    .Initialize = SPI1_Initialize,
    .Deinitialize = SPI1_Deinitialize,
    .Open = SPI1_Open,
    .Close = SPI1_Close,
    .Transfer = SPI1_Transfer,
    .BufferExchange = SPI1_BufferExchange,
    .BufferWrite = SPI1_BufferWrite,
    .BufferRead = SPI1_BufferRead,	
    .ByteExchange = SPI1_ByteExchange,
    .ByteWrite = SPI1_ByteWrite,
    .ByteRead = SPI1_ByteRead,
    .IsTxReady = SPI1_IsTxReady,
    .IsRxReady = SPI1_IsRxReady,
    .RxCompleteCallbackRegister = NULL,
    .TxCompleteCallbackRegister = NULL
};

static const spi_configuration_t spi1_configuration[] =
{
    { 0x23, 0xc4 },
    { 0x21, 0xc4 },
    { 0x20, 0xC4 }
};

void SPI1_Initialize(void)
{
    SPI1.CTRLA &= ~SPI_ENABLE_bm;
    SPI1.CTRLA = (0 << SPI_CLK2X_bp)                /* CLK2X (disabled) */
				|(0 << SPI_DORD_bp)                 /* DORD (disabled) */
				|(0 << SPI_ENABLE_bp)               /* ENABLE (disabled) */
				|(1 << SPI_MASTER_bp)               /* MASTER (enabled) */
				|(SPI_PRESC_DIV4_gc);               /* PRESC (DIV4) */
    SPI1.CTRLB = (1 << SPI_BUFEN_bp)                /* BUFEN (enabled) */
				|(1 << SPI_BUFWR_bp)                /* BUFWR (enabled) */
				|(SPI_MODE_0_gc)                    /* MODE (0) */
				|(1 << SPI_SSD_bp);                 /* SSD (enabled) */
}

void SPI1_Deinitialize(void)
{
    SPI1.CTRLA = 0x0;
    SPI1.CTRLB = 0x0;
    SPI1.INTCTRL = 0x0;
}

bool SPI1_Open(uint8_t spiConfigIndex)
{
    bool returnValue = false;
    if (0 == (SPI1.CTRLA & SPI_ENABLE_bm)) 
    {
        SPI1.CTRLB = spi1_configuration[spiConfigIndex].ctrlb;
        SPI1.CTRLA = spi1_configuration[spiConfigIndex].ctrla;
        SPI1.CTRLA |= SPI_ENABLE_bm;
        returnValue = true;
    } 
    else 
    {
        returnValue = false;
    }
    return returnValue;
}

void SPI1_Close(void)
{
    SPI1.CTRLA &= ~SPI_ENABLE_bm;
}

uint8_t SPI1_ByteExchange(uint8_t byteData)
{
    uint8_t readData = 0;
    SPI1.DATA = byteData;
    while (0 == (SPI1.INTFLAGS & SPI_RXCIF_bm))
    { 
        ; // Wait until ongoing SPI transfer is completed
    }
    readData = SPI1.DATA;
    return readData;
}

void SPI1_ByteWrite(uint8_t byteData)
{
    SPI1.DATA = byteData;
}

uint8_t SPI1_ByteRead(void)
{
    return SPI1.DATA;
}

void SPI1_Transfer(const void *txBuffer, void *rxBuffer, size_t bufferSize)
{
    const uint8_t *bufferTransmit = (const uint8_t *)txBuffer;
    uint8_t *bufferReceive = (uint8_t *)rxBuffer;
	size_t bufferInputSize = bufferSize;
    while(0U != bufferInputSize) 
    {
        SPI1.DATA = *bufferTransmit;
        while (0 == (SPI1.INTFLAGS & SPI_RXCIF_bm))
        {
            ; // Wait until ongoing SPI transfer is completed
        }
        *bufferReceive = SPI1.DATA;
        bufferTransmit++;
        bufferReceive++;  
        bufferInputSize--;
    }
}

void SPI1_BufferExchange(void *bufferData, size_t bufferSize)
{
    uint8_t *buffer = (uint8_t *)bufferData;
	size_t bufferInputSize = bufferSize;
    while(0U != bufferInputSize) 
    {
        SPI1.DATA = *buffer;
        while (0 == (SPI1.INTFLAGS & SPI_RXCIF_bm))
        {
            ; // Wait until ongoing SPI transfer is completed
        }
        *buffer = SPI1.DATA;
        buffer++;
        bufferInputSize--;
    }
}

void SPI1_BufferWrite(void *bufferData, size_t bufferSize)
{
    uint8_t *buffer = (uint8_t *)bufferData;
    uint8_t  readData = 0;
	size_t bufferInputSize = bufferSize;
    while(0U != bufferInputSize) 
    {
        SPI1.DATA = *buffer;
        while(0 == (SPI1.INTFLAGS & SPI_RXCIF_bm))
        {
            ; // Wait until ongoing SPI transfer is completed
        }        
        readData = SPI1.DATA;
        (void) readData;
        buffer++;
        bufferInputSize--;
    }
}

void SPI1_BufferRead(void *bufferData, size_t bufferSize)
{
    uint8_t *buffer = (uint8_t *)bufferData;
	size_t bufferInputSize = bufferSize;
    while(0U != bufferInputSize) 
    {
        SPI1.DATA = 0;
        while (0 == (SPI1.INTFLAGS & SPI_RXCIF_bm))
            {
                ; // Wait until ongoing SPI transfer is completed
            }
        *buffer = SPI1.DATA;
        buffer++;
        bufferInputSize--;
    }
}

bool SPI1_IsTxReady(void)
{
    bool returnValue = false;
    if(0 != (SPI1.CTRLA & SPI_ENABLE_bm))
    {
        if(0 != (SPI1.INTFLAGS & SPI_DREIF_bm))
        {
            returnValue = true;
        }
        else
        {
           returnValue = false; 
        }
    }
    else
    {
        returnValue = false;
    }
    return returnValue;
}

bool SPI1_IsRxReady(void)
{
    bool returnValue = false;
    if(0 != (SPI1.CTRLA & SPI_ENABLE_bm))
    {
        if(0 != (SPI1.INTFLAGS & SPI_RXCIF_bm))
        {
            returnValue = true;
        }
        else
        {
           returnValue = false; 
        }
    }
    else
    {
        returnValue = false;
    }
    return returnValue;
}