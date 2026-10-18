/**
 * Generated Pins header File
 * 
 * @file pins.h
 * 
 * @defgroup  pinsdriver Pins Driver
 * 
 * @brief This is generated driver header for pins. 
 *        This header file provides APIs for all pins selected in the GUI.
 *
 * @version Driver Version  1.1.0
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

#ifndef PINS_H_INCLUDED
#define PINS_H_INCLUDED

#include <avr/io.h>
#include "./port.h"

//get/set MISO aliases
#define MISO_SetHigh() do { PORTA_OUTSET = 0x20; } while(0)
#define MISO_SetLow() do { PORTA_OUTCLR = 0x20; } while(0)
#define MISO_Toggle() do { PORTA_OUTTGL = 0x20; } while(0)
#define MISO_GetValue() (VPORTA.IN & (0x1 << 5))
#define MISO_SetDigitalInput() do { PORTA_DIRCLR = 0x20; } while(0)
#define MISO_SetDigitalOutput() do { PORTA_DIRSET = 0x20; } while(0)
#define MISO_SetPullUp() do { PORTA_PIN5CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define MISO_ResetPullUp() do { PORTA_PIN5CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define MISO_SetInverted() do { PORTA_PIN5CTRL  |= PORT_INVEN_bm; } while(0)
#define MISO_ResetInverted() do { PORTA_PIN5CTRL  &= ~PORT_INVEN_bm; } while(0)
#define MISO_DisableInterruptOnChange() do { PORTA.PIN5CTRL = (PORTA.PIN5CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define MISO_EnableInterruptForBothEdges() do { PORTA.PIN5CTRL = (PORTA.PIN5CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define MISO_EnableInterruptForRisingEdge() do { PORTA.PIN5CTRL = (PORTA.PIN5CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define MISO_EnableInterruptForFallingEdge() do { PORTA.PIN5CTRL = (PORTA.PIN5CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define MISO_DisableDigitalInputBuffer() do { PORTA.PIN5CTRL = (PORTA.PIN5CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define MISO_EnableInterruptForLowLevelSensing() do { PORTA.PIN5CTRL = (PORTA.PIN5CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA5_SetInterruptHandler MISO_SetInterruptHandler

//get/set MOSI aliases
#define MOSI_SetHigh() do { PORTA_OUTSET = 0x10; } while(0)
#define MOSI_SetLow() do { PORTA_OUTCLR = 0x10; } while(0)
#define MOSI_Toggle() do { PORTA_OUTTGL = 0x10; } while(0)
#define MOSI_GetValue() (VPORTA.IN & (0x1 << 4))
#define MOSI_SetDigitalInput() do { PORTA_DIRCLR = 0x10; } while(0)
#define MOSI_SetDigitalOutput() do { PORTA_DIRSET = 0x10; } while(0)
#define MOSI_SetPullUp() do { PORTA_PIN4CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define MOSI_ResetPullUp() do { PORTA_PIN4CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define MOSI_SetInverted() do { PORTA_PIN4CTRL  |= PORT_INVEN_bm; } while(0)
#define MOSI_ResetInverted() do { PORTA_PIN4CTRL  &= ~PORT_INVEN_bm; } while(0)
#define MOSI_DisableInterruptOnChange() do { PORTA.PIN4CTRL = (PORTA.PIN4CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define MOSI_EnableInterruptForBothEdges() do { PORTA.PIN4CTRL = (PORTA.PIN4CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define MOSI_EnableInterruptForRisingEdge() do { PORTA.PIN4CTRL = (PORTA.PIN4CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define MOSI_EnableInterruptForFallingEdge() do { PORTA.PIN4CTRL = (PORTA.PIN4CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define MOSI_DisableDigitalInputBuffer() do { PORTA.PIN4CTRL = (PORTA.PIN4CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define MOSI_EnableInterruptForLowLevelSensing() do { PORTA.PIN4CTRL = (PORTA.PIN4CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA4_SetInterruptHandler MOSI_SetInterruptHandler

//get/set SCK aliases
#define SCK_SetHigh() do { PORTA_OUTSET = 0x40; } while(0)
#define SCK_SetLow() do { PORTA_OUTCLR = 0x40; } while(0)
#define SCK_Toggle() do { PORTA_OUTTGL = 0x40; } while(0)
#define SCK_GetValue() (VPORTA.IN & (0x1 << 6))
#define SCK_SetDigitalInput() do { PORTA_DIRCLR = 0x40; } while(0)
#define SCK_SetDigitalOutput() do { PORTA_DIRSET = 0x40; } while(0)
#define SCK_SetPullUp() do { PORTA_PIN6CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define SCK_ResetPullUp() do { PORTA_PIN6CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define SCK_SetInverted() do { PORTA_PIN6CTRL  |= PORT_INVEN_bm; } while(0)
#define SCK_ResetInverted() do { PORTA_PIN6CTRL  &= ~PORT_INVEN_bm; } while(0)
#define SCK_DisableInterruptOnChange() do { PORTA.PIN6CTRL = (PORTA.PIN6CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define SCK_EnableInterruptForBothEdges() do { PORTA.PIN6CTRL = (PORTA.PIN6CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define SCK_EnableInterruptForRisingEdge() do { PORTA.PIN6CTRL = (PORTA.PIN6CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define SCK_EnableInterruptForFallingEdge() do { PORTA.PIN6CTRL = (PORTA.PIN6CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define SCK_DisableDigitalInputBuffer() do { PORTA.PIN6CTRL = (PORTA.PIN6CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define SCK_EnableInterruptForLowLevelSensing() do { PORTA.PIN6CTRL = (PORTA.PIN6CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA6_SetInterruptHandler SCK_SetInterruptHandler

//get/set VBAT aliases
#define VBAT_SetHigh() do { PORTF_OUTSET = 0x10; } while(0)
#define VBAT_SetLow() do { PORTF_OUTCLR = 0x10; } while(0)
#define VBAT_Toggle() do { PORTF_OUTTGL = 0x10; } while(0)
#define VBAT_GetValue() (VPORTF.IN & (0x1 << 4))
#define VBAT_SetDigitalInput() do { PORTF_DIRCLR = 0x10; } while(0)
#define VBAT_SetDigitalOutput() do { PORTF_DIRSET = 0x10; } while(0)
#define VBAT_SetPullUp() do { PORTF_PIN4CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define VBAT_ResetPullUp() do { PORTF_PIN4CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define VBAT_SetInverted() do { PORTF_PIN4CTRL  |= PORT_INVEN_bm; } while(0)
#define VBAT_ResetInverted() do { PORTF_PIN4CTRL  &= ~PORT_INVEN_bm; } while(0)
#define VBAT_DisableInterruptOnChange() do { PORTF.PIN4CTRL = (PORTF.PIN4CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define VBAT_EnableInterruptForBothEdges() do { PORTF.PIN4CTRL = (PORTF.PIN4CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define VBAT_EnableInterruptForRisingEdge() do { PORTF.PIN4CTRL = (PORTF.PIN4CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define VBAT_EnableInterruptForFallingEdge() do { PORTF.PIN4CTRL = (PORTF.PIN4CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define VBAT_DisableDigitalInputBuffer() do { PORTF.PIN4CTRL = (PORTF.PIN4CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define VBAT_EnableInterruptForLowLevelSensing() do { PORTF.PIN4CTRL = (PORTF.PIN4CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PF4_SetInterruptHandler VBAT_SetInterruptHandler

//get/set SCL aliases
#define SCL_SetHigh() do { PORTA_OUTSET = 0x8; } while(0)
#define SCL_SetLow() do { PORTA_OUTCLR = 0x8; } while(0)
#define SCL_Toggle() do { PORTA_OUTTGL = 0x8; } while(0)
#define SCL_GetValue() (VPORTA.IN & (0x1 << 3))
#define SCL_SetDigitalInput() do { PORTA_DIRCLR = 0x8; } while(0)
#define SCL_SetDigitalOutput() do { PORTA_DIRSET = 0x8; } while(0)
#define SCL_SetPullUp() do { PORTA_PIN3CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define SCL_ResetPullUp() do { PORTA_PIN3CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define SCL_SetInverted() do { PORTA_PIN3CTRL  |= PORT_INVEN_bm; } while(0)
#define SCL_ResetInverted() do { PORTA_PIN3CTRL  &= ~PORT_INVEN_bm; } while(0)
#define SCL_DisableInterruptOnChange() do { PORTA.PIN3CTRL = (PORTA.PIN3CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define SCL_EnableInterruptForBothEdges() do { PORTA.PIN3CTRL = (PORTA.PIN3CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define SCL_EnableInterruptForRisingEdge() do { PORTA.PIN3CTRL = (PORTA.PIN3CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define SCL_EnableInterruptForFallingEdge() do { PORTA.PIN3CTRL = (PORTA.PIN3CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define SCL_DisableDigitalInputBuffer() do { PORTA.PIN3CTRL = (PORTA.PIN3CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define SCL_EnableInterruptForLowLevelSensing() do { PORTA.PIN3CTRL = (PORTA.PIN3CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA3_SetInterruptHandler SCL_SetInterruptHandler

//get/set SDA aliases
#define SDA_SetHigh() do { PORTA_OUTSET = 0x4; } while(0)
#define SDA_SetLow() do { PORTA_OUTCLR = 0x4; } while(0)
#define SDA_Toggle() do { PORTA_OUTTGL = 0x4; } while(0)
#define SDA_GetValue() (VPORTA.IN & (0x1 << 2))
#define SDA_SetDigitalInput() do { PORTA_DIRCLR = 0x4; } while(0)
#define SDA_SetDigitalOutput() do { PORTA_DIRSET = 0x4; } while(0)
#define SDA_SetPullUp() do { PORTA_PIN2CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define SDA_ResetPullUp() do { PORTA_PIN2CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define SDA_SetInverted() do { PORTA_PIN2CTRL  |= PORT_INVEN_bm; } while(0)
#define SDA_ResetInverted() do { PORTA_PIN2CTRL  &= ~PORT_INVEN_bm; } while(0)
#define SDA_DisableInterruptOnChange() do { PORTA.PIN2CTRL = (PORTA.PIN2CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define SDA_EnableInterruptForBothEdges() do { PORTA.PIN2CTRL = (PORTA.PIN2CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define SDA_EnableInterruptForRisingEdge() do { PORTA.PIN2CTRL = (PORTA.PIN2CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define SDA_EnableInterruptForFallingEdge() do { PORTA.PIN2CTRL = (PORTA.PIN2CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define SDA_DisableDigitalInputBuffer() do { PORTA.PIN2CTRL = (PORTA.PIN2CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define SDA_EnableInterruptForLowLevelSensing() do { PORTA.PIN2CTRL = (PORTA.PIN2CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA2_SetInterruptHandler SDA_SetInterruptHandler

//get/set TOF_GPIO aliases
#define TOF_GPIO_SetHigh() do { PORTA_OUTSET = 0x2; } while(0)
#define TOF_GPIO_SetLow() do { PORTA_OUTCLR = 0x2; } while(0)
#define TOF_GPIO_Toggle() do { PORTA_OUTTGL = 0x2; } while(0)
#define TOF_GPIO_GetValue() (VPORTA.IN & (0x1 << 1))
#define TOF_GPIO_SetDigitalInput() do { PORTA_DIRCLR = 0x2; } while(0)
#define TOF_GPIO_SetDigitalOutput() do { PORTA_DIRSET = 0x2; } while(0)
#define TOF_GPIO_SetPullUp() do { PORTA_PIN1CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define TOF_GPIO_ResetPullUp() do { PORTA_PIN1CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define TOF_GPIO_SetInverted() do { PORTA_PIN1CTRL  |= PORT_INVEN_bm; } while(0)
#define TOF_GPIO_ResetInverted() do { PORTA_PIN1CTRL  &= ~PORT_INVEN_bm; } while(0)
#define TOF_GPIO_DisableInterruptOnChange() do { PORTA.PIN1CTRL = (PORTA.PIN1CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define TOF_GPIO_EnableInterruptForBothEdges() do { PORTA.PIN1CTRL = (PORTA.PIN1CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define TOF_GPIO_EnableInterruptForRisingEdge() do { PORTA.PIN1CTRL = (PORTA.PIN1CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define TOF_GPIO_EnableInterruptForFallingEdge() do { PORTA.PIN1CTRL = (PORTA.PIN1CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define TOF_GPIO_DisableDigitalInputBuffer() do { PORTA.PIN1CTRL = (PORTA.PIN1CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define TOF_GPIO_EnableInterruptForLowLevelSensing() do { PORTA.PIN1CTRL = (PORTA.PIN1CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA1_SetInterruptHandler TOF_GPIO_SetInterruptHandler

//get/set SW aliases
#define SW_SetHigh() do { PORTA_OUTSET = 0x80; } while(0)
#define SW_SetLow() do { PORTA_OUTCLR = 0x80; } while(0)
#define SW_Toggle() do { PORTA_OUTTGL = 0x80; } while(0)
#define SW_GetValue() (VPORTA.IN & (0x1 << 7))
#define SW_SetDigitalInput() do { PORTA_DIRCLR = 0x80; } while(0)
#define SW_SetDigitalOutput() do { PORTA_DIRSET = 0x80; } while(0)
#define SW_SetPullUp() do { PORTA_PIN7CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define SW_ResetPullUp() do { PORTA_PIN7CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define SW_SetInverted() do { PORTA_PIN7CTRL  |= PORT_INVEN_bm; } while(0)
#define SW_ResetInverted() do { PORTA_PIN7CTRL  &= ~PORT_INVEN_bm; } while(0)
#define SW_DisableInterruptOnChange() do { PORTA.PIN7CTRL = (PORTA.PIN7CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define SW_EnableInterruptForBothEdges() do { PORTA.PIN7CTRL = (PORTA.PIN7CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define SW_EnableInterruptForRisingEdge() do { PORTA.PIN7CTRL = (PORTA.PIN7CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define SW_EnableInterruptForFallingEdge() do { PORTA.PIN7CTRL = (PORTA.PIN7CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define SW_DisableDigitalInputBuffer() do { PORTA.PIN7CTRL = (PORTA.PIN7CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define SW_EnableInterruptForLowLevelSensing() do { PORTA.PIN7CTRL = (PORTA.PIN7CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA7_SetInterruptHandler SW_SetInterruptHandler

//get/set IO_PD0 aliases
#define IO_PD0_SetHigh() do { PORTD_OUTSET = 0x1; } while(0)
#define IO_PD0_SetLow() do { PORTD_OUTCLR = 0x1; } while(0)
#define IO_PD0_Toggle() do { PORTD_OUTTGL = 0x1; } while(0)
#define IO_PD0_GetValue() (VPORTD.IN & (0x1 << 0))
#define IO_PD0_SetDigitalInput() do { PORTD_DIRCLR = 0x1; } while(0)
#define IO_PD0_SetDigitalOutput() do { PORTD_DIRSET = 0x1; } while(0)
#define IO_PD0_SetPullUp() do { PORTD_PIN0CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD0_ResetPullUp() do { PORTD_PIN0CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD0_SetInverted() do { PORTD_PIN0CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD0_ResetInverted() do { PORTD_PIN0CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD0_DisableInterruptOnChange() do { PORTD.PIN0CTRL = (PORTD.PIN0CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD0_EnableInterruptForBothEdges() do { PORTD.PIN0CTRL = (PORTD.PIN0CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD0_EnableInterruptForRisingEdge() do { PORTD.PIN0CTRL = (PORTD.PIN0CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD0_EnableInterruptForFallingEdge() do { PORTD.PIN0CTRL = (PORTD.PIN0CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD0_DisableDigitalInputBuffer() do { PORTD.PIN0CTRL = (PORTD.PIN0CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD0_EnableInterruptForLowLevelSensing() do { PORTD.PIN0CTRL = (PORTD.PIN0CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD0_SetInterruptHandler IO_PD0_SetInterruptHandler

//get/set IO_PD1 aliases
#define IO_PD1_SetHigh() do { PORTD_OUTSET = 0x2; } while(0)
#define IO_PD1_SetLow() do { PORTD_OUTCLR = 0x2; } while(0)
#define IO_PD1_Toggle() do { PORTD_OUTTGL = 0x2; } while(0)
#define IO_PD1_GetValue() (VPORTD.IN & (0x1 << 1))
#define IO_PD1_SetDigitalInput() do { PORTD_DIRCLR = 0x2; } while(0)
#define IO_PD1_SetDigitalOutput() do { PORTD_DIRSET = 0x2; } while(0)
#define IO_PD1_SetPullUp() do { PORTD_PIN1CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD1_ResetPullUp() do { PORTD_PIN1CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD1_SetInverted() do { PORTD_PIN1CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD1_ResetInverted() do { PORTD_PIN1CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD1_DisableInterruptOnChange() do { PORTD.PIN1CTRL = (PORTD.PIN1CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD1_EnableInterruptForBothEdges() do { PORTD.PIN1CTRL = (PORTD.PIN1CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD1_EnableInterruptForRisingEdge() do { PORTD.PIN1CTRL = (PORTD.PIN1CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD1_EnableInterruptForFallingEdge() do { PORTD.PIN1CTRL = (PORTD.PIN1CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD1_DisableDigitalInputBuffer() do { PORTD.PIN1CTRL = (PORTD.PIN1CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD1_EnableInterruptForLowLevelSensing() do { PORTD.PIN1CTRL = (PORTD.PIN1CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD1_SetInterruptHandler IO_PD1_SetInterruptHandler

//get/set IO_PD2 aliases
#define IO_PD2_SetHigh() do { PORTD_OUTSET = 0x4; } while(0)
#define IO_PD2_SetLow() do { PORTD_OUTCLR = 0x4; } while(0)
#define IO_PD2_Toggle() do { PORTD_OUTTGL = 0x4; } while(0)
#define IO_PD2_GetValue() (VPORTD.IN & (0x1 << 2))
#define IO_PD2_SetDigitalInput() do { PORTD_DIRCLR = 0x4; } while(0)
#define IO_PD2_SetDigitalOutput() do { PORTD_DIRSET = 0x4; } while(0)
#define IO_PD2_SetPullUp() do { PORTD_PIN2CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD2_ResetPullUp() do { PORTD_PIN2CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD2_SetInverted() do { PORTD_PIN2CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD2_ResetInverted() do { PORTD_PIN2CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD2_DisableInterruptOnChange() do { PORTD.PIN2CTRL = (PORTD.PIN2CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD2_EnableInterruptForBothEdges() do { PORTD.PIN2CTRL = (PORTD.PIN2CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD2_EnableInterruptForRisingEdge() do { PORTD.PIN2CTRL = (PORTD.PIN2CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD2_EnableInterruptForFallingEdge() do { PORTD.PIN2CTRL = (PORTD.PIN2CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD2_DisableDigitalInputBuffer() do { PORTD.PIN2CTRL = (PORTD.PIN2CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD2_EnableInterruptForLowLevelSensing() do { PORTD.PIN2CTRL = (PORTD.PIN2CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD2_SetInterruptHandler IO_PD2_SetInterruptHandler

//get/set IO_PD3 aliases
#define IO_PD3_SetHigh() do { PORTD_OUTSET = 0x8; } while(0)
#define IO_PD3_SetLow() do { PORTD_OUTCLR = 0x8; } while(0)
#define IO_PD3_Toggle() do { PORTD_OUTTGL = 0x8; } while(0)
#define IO_PD3_GetValue() (VPORTD.IN & (0x1 << 3))
#define IO_PD3_SetDigitalInput() do { PORTD_DIRCLR = 0x8; } while(0)
#define IO_PD3_SetDigitalOutput() do { PORTD_DIRSET = 0x8; } while(0)
#define IO_PD3_SetPullUp() do { PORTD_PIN3CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD3_ResetPullUp() do { PORTD_PIN3CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD3_SetInverted() do { PORTD_PIN3CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD3_ResetInverted() do { PORTD_PIN3CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD3_DisableInterruptOnChange() do { PORTD.PIN3CTRL = (PORTD.PIN3CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD3_EnableInterruptForBothEdges() do { PORTD.PIN3CTRL = (PORTD.PIN3CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD3_EnableInterruptForRisingEdge() do { PORTD.PIN3CTRL = (PORTD.PIN3CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD3_EnableInterruptForFallingEdge() do { PORTD.PIN3CTRL = (PORTD.PIN3CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD3_DisableDigitalInputBuffer() do { PORTD.PIN3CTRL = (PORTD.PIN3CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD3_EnableInterruptForLowLevelSensing() do { PORTD.PIN3CTRL = (PORTD.PIN3CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD3_SetInterruptHandler IO_PD3_SetInterruptHandler

//get/set IO_PD4 aliases
#define IO_PD4_SetHigh() do { PORTD_OUTSET = 0x10; } while(0)
#define IO_PD4_SetLow() do { PORTD_OUTCLR = 0x10; } while(0)
#define IO_PD4_Toggle() do { PORTD_OUTTGL = 0x10; } while(0)
#define IO_PD4_GetValue() (VPORTD.IN & (0x1 << 4))
#define IO_PD4_SetDigitalInput() do { PORTD_DIRCLR = 0x10; } while(0)
#define IO_PD4_SetDigitalOutput() do { PORTD_DIRSET = 0x10; } while(0)
#define IO_PD4_SetPullUp() do { PORTD_PIN4CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD4_ResetPullUp() do { PORTD_PIN4CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD4_SetInverted() do { PORTD_PIN4CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD4_ResetInverted() do { PORTD_PIN4CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD4_DisableInterruptOnChange() do { PORTD.PIN4CTRL = (PORTD.PIN4CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD4_EnableInterruptForBothEdges() do { PORTD.PIN4CTRL = (PORTD.PIN4CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD4_EnableInterruptForRisingEdge() do { PORTD.PIN4CTRL = (PORTD.PIN4CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD4_EnableInterruptForFallingEdge() do { PORTD.PIN4CTRL = (PORTD.PIN4CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD4_DisableDigitalInputBuffer() do { PORTD.PIN4CTRL = (PORTD.PIN4CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD4_EnableInterruptForLowLevelSensing() do { PORTD.PIN4CTRL = (PORTD.PIN4CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD4_SetInterruptHandler IO_PD4_SetInterruptHandler

//get/set IO_PD5 aliases
#define IO_PD5_SetHigh() do { PORTD_OUTSET = 0x20; } while(0)
#define IO_PD5_SetLow() do { PORTD_OUTCLR = 0x20; } while(0)
#define IO_PD5_Toggle() do { PORTD_OUTTGL = 0x20; } while(0)
#define IO_PD5_GetValue() (VPORTD.IN & (0x1 << 5))
#define IO_PD5_SetDigitalInput() do { PORTD_DIRCLR = 0x20; } while(0)
#define IO_PD5_SetDigitalOutput() do { PORTD_DIRSET = 0x20; } while(0)
#define IO_PD5_SetPullUp() do { PORTD_PIN5CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD5_ResetPullUp() do { PORTD_PIN5CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD5_SetInverted() do { PORTD_PIN5CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD5_ResetInverted() do { PORTD_PIN5CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD5_DisableInterruptOnChange() do { PORTD.PIN5CTRL = (PORTD.PIN5CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD5_EnableInterruptForBothEdges() do { PORTD.PIN5CTRL = (PORTD.PIN5CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD5_EnableInterruptForRisingEdge() do { PORTD.PIN5CTRL = (PORTD.PIN5CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD5_EnableInterruptForFallingEdge() do { PORTD.PIN5CTRL = (PORTD.PIN5CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD5_DisableDigitalInputBuffer() do { PORTD.PIN5CTRL = (PORTD.PIN5CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD5_EnableInterruptForLowLevelSensing() do { PORTD.PIN5CTRL = (PORTD.PIN5CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD5_SetInterruptHandler IO_PD5_SetInterruptHandler

//get/set IO_PD6 aliases
#define IO_PD6_SetHigh() do { PORTD_OUTSET = 0x40; } while(0)
#define IO_PD6_SetLow() do { PORTD_OUTCLR = 0x40; } while(0)
#define IO_PD6_Toggle() do { PORTD_OUTTGL = 0x40; } while(0)
#define IO_PD6_GetValue() (VPORTD.IN & (0x1 << 6))
#define IO_PD6_SetDigitalInput() do { PORTD_DIRCLR = 0x40; } while(0)
#define IO_PD6_SetDigitalOutput() do { PORTD_DIRSET = 0x40; } while(0)
#define IO_PD6_SetPullUp() do { PORTD_PIN6CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD6_ResetPullUp() do { PORTD_PIN6CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD6_SetInverted() do { PORTD_PIN6CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD6_ResetInverted() do { PORTD_PIN6CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD6_DisableInterruptOnChange() do { PORTD.PIN6CTRL = (PORTD.PIN6CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD6_EnableInterruptForBothEdges() do { PORTD.PIN6CTRL = (PORTD.PIN6CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD6_EnableInterruptForRisingEdge() do { PORTD.PIN6CTRL = (PORTD.PIN6CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD6_EnableInterruptForFallingEdge() do { PORTD.PIN6CTRL = (PORTD.PIN6CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD6_DisableDigitalInputBuffer() do { PORTD.PIN6CTRL = (PORTD.PIN6CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD6_EnableInterruptForLowLevelSensing() do { PORTD.PIN6CTRL = (PORTD.PIN6CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD6_SetInterruptHandler IO_PD6_SetInterruptHandler

//get/set IO_PD7 aliases
#define IO_PD7_SetHigh() do { PORTD_OUTSET = 0x80; } while(0)
#define IO_PD7_SetLow() do { PORTD_OUTCLR = 0x80; } while(0)
#define IO_PD7_Toggle() do { PORTD_OUTTGL = 0x80; } while(0)
#define IO_PD7_GetValue() (VPORTD.IN & (0x1 << 7))
#define IO_PD7_SetDigitalInput() do { PORTD_DIRCLR = 0x80; } while(0)
#define IO_PD7_SetDigitalOutput() do { PORTD_DIRSET = 0x80; } while(0)
#define IO_PD7_SetPullUp() do { PORTD_PIN7CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PD7_ResetPullUp() do { PORTD_PIN7CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PD7_SetInverted() do { PORTD_PIN7CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PD7_ResetInverted() do { PORTD_PIN7CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PD7_DisableInterruptOnChange() do { PORTD.PIN7CTRL = (PORTD.PIN7CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PD7_EnableInterruptForBothEdges() do { PORTD.PIN7CTRL = (PORTD.PIN7CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PD7_EnableInterruptForRisingEdge() do { PORTD.PIN7CTRL = (PORTD.PIN7CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PD7_EnableInterruptForFallingEdge() do { PORTD.PIN7CTRL = (PORTD.PIN7CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PD7_DisableDigitalInputBuffer() do { PORTD.PIN7CTRL = (PORTD.PIN7CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PD7_EnableInterruptForLowLevelSensing() do { PORTD.PIN7CTRL = (PORTD.PIN7CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PD7_SetInterruptHandler IO_PD7_SetInterruptHandler

//get/set VBUS_DET aliases
#define VBUS_DET_SetHigh() do { PORTF_OUTSET = 0x4; } while(0)
#define VBUS_DET_SetLow() do { PORTF_OUTCLR = 0x4; } while(0)
#define VBUS_DET_Toggle() do { PORTF_OUTTGL = 0x4; } while(0)
#define VBUS_DET_GetValue() (VPORTF.IN & (0x1 << 2))
#define VBUS_DET_SetDigitalInput() do { PORTF_DIRCLR = 0x4; } while(0)
#define VBUS_DET_SetDigitalOutput() do { PORTF_DIRSET = 0x4; } while(0)
#define VBUS_DET_SetPullUp() do { PORTF_PIN2CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define VBUS_DET_ResetPullUp() do { PORTF_PIN2CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define VBUS_DET_SetInverted() do { PORTF_PIN2CTRL  |= PORT_INVEN_bm; } while(0)
#define VBUS_DET_ResetInverted() do { PORTF_PIN2CTRL  &= ~PORT_INVEN_bm; } while(0)
#define VBUS_DET_DisableInterruptOnChange() do { PORTF.PIN2CTRL = (PORTF.PIN2CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define VBUS_DET_EnableInterruptForBothEdges() do { PORTF.PIN2CTRL = (PORTF.PIN2CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define VBUS_DET_EnableInterruptForRisingEdge() do { PORTF.PIN2CTRL = (PORTF.PIN2CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define VBUS_DET_EnableInterruptForFallingEdge() do { PORTF.PIN2CTRL = (PORTF.PIN2CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define VBUS_DET_DisableDigitalInputBuffer() do { PORTF.PIN2CTRL = (PORTF.PIN2CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define VBUS_DET_EnableInterruptForLowLevelSensing() do { PORTF.PIN2CTRL = (PORTF.PIN2CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PF2_SetInterruptHandler VBUS_DET_SetInterruptHandler

//get/set IO_PF5 aliases
#define IO_PF5_SetHigh() do { PORTF_OUTSET = 0x20; } while(0)
#define IO_PF5_SetLow() do { PORTF_OUTCLR = 0x20; } while(0)
#define IO_PF5_Toggle() do { PORTF_OUTTGL = 0x20; } while(0)
#define IO_PF5_GetValue() (VPORTF.IN & (0x1 << 5))
#define IO_PF5_SetDigitalInput() do { PORTF_DIRCLR = 0x20; } while(0)
#define IO_PF5_SetDigitalOutput() do { PORTF_DIRSET = 0x20; } while(0)
#define IO_PF5_SetPullUp() do { PORTF_PIN5CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define IO_PF5_ResetPullUp() do { PORTF_PIN5CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define IO_PF5_SetInverted() do { PORTF_PIN5CTRL  |= PORT_INVEN_bm; } while(0)
#define IO_PF5_ResetInverted() do { PORTF_PIN5CTRL  &= ~PORT_INVEN_bm; } while(0)
#define IO_PF5_DisableInterruptOnChange() do { PORTF.PIN5CTRL = (PORTF.PIN5CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define IO_PF5_EnableInterruptForBothEdges() do { PORTF.PIN5CTRL = (PORTF.PIN5CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define IO_PF5_EnableInterruptForRisingEdge() do { PORTF.PIN5CTRL = (PORTF.PIN5CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define IO_PF5_EnableInterruptForFallingEdge() do { PORTF.PIN5CTRL = (PORTF.PIN5CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define IO_PF5_DisableDigitalInputBuffer() do { PORTF.PIN5CTRL = (PORTF.PIN5CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define IO_PF5_EnableInterruptForLowLevelSensing() do { PORTF.PIN5CTRL = (PORTF.PIN5CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PF5_SetInterruptHandler IO_PF5_SetInterruptHandler

//get/set FLASH_CS aliases
#define FLASH_CS_SetHigh() do { PORTA_OUTSET = 0x1; } while(0)
#define FLASH_CS_SetLow() do { PORTA_OUTCLR = 0x1; } while(0)
#define FLASH_CS_Toggle() do { PORTA_OUTTGL = 0x1; } while(0)
#define FLASH_CS_GetValue() (VPORTA.IN & (0x1 << 0))
#define FLASH_CS_SetDigitalInput() do { PORTA_DIRCLR = 0x1; } while(0)
#define FLASH_CS_SetDigitalOutput() do { PORTA_DIRSET = 0x1; } while(0)
#define FLASH_CS_SetPullUp() do { PORTA_PIN0CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define FLASH_CS_ResetPullUp() do { PORTA_PIN0CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define FLASH_CS_SetInverted() do { PORTA_PIN0CTRL  |= PORT_INVEN_bm; } while(0)
#define FLASH_CS_ResetInverted() do { PORTA_PIN0CTRL  &= ~PORT_INVEN_bm; } while(0)
#define FLASH_CS_DisableInterruptOnChange() do { PORTA.PIN0CTRL = (PORTA.PIN0CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define FLASH_CS_EnableInterruptForBothEdges() do { PORTA.PIN0CTRL = (PORTA.PIN0CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define FLASH_CS_EnableInterruptForRisingEdge() do { PORTA.PIN0CTRL = (PORTA.PIN0CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define FLASH_CS_EnableInterruptForFallingEdge() do { PORTA.PIN0CTRL = (PORTA.PIN0CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define FLASH_CS_DisableDigitalInputBuffer() do { PORTA.PIN0CTRL = (PORTA.PIN0CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define FLASH_CS_EnableInterruptForLowLevelSensing() do { PORTA.PIN0CTRL = (PORTA.PIN0CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PA0_SetInterruptHandler FLASH_CS_SetInterruptHandler

//get/set LED aliases
#define LED_SetHigh() do { PORTC_OUTSET = 0x8; } while(0)
#define LED_SetLow() do { PORTC_OUTCLR = 0x8; } while(0)
#define LED_Toggle() do { PORTC_OUTTGL = 0x8; } while(0)
#define LED_GetValue() (VPORTC.IN & (0x1 << 3))
#define LED_SetDigitalInput() do { PORTC_DIRCLR = 0x8; } while(0)
#define LED_SetDigitalOutput() do { PORTC_DIRSET = 0x8; } while(0)
#define LED_SetPullUp() do { PORTC_PIN3CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define LED_ResetPullUp() do { PORTC_PIN3CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define LED_SetInverted() do { PORTC_PIN3CTRL  |= PORT_INVEN_bm; } while(0)
#define LED_ResetInverted() do { PORTC_PIN3CTRL  &= ~PORT_INVEN_bm; } while(0)
#define LED_DisableInterruptOnChange() do { PORTC.PIN3CTRL = (PORTC.PIN3CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define LED_EnableInterruptForBothEdges() do { PORTC.PIN3CTRL = (PORTC.PIN3CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define LED_EnableInterruptForRisingEdge() do { PORTC.PIN3CTRL = (PORTC.PIN3CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define LED_EnableInterruptForFallingEdge() do { PORTC.PIN3CTRL = (PORTC.PIN3CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define LED_DisableDigitalInputBuffer() do { PORTC.PIN3CTRL = (PORTC.PIN3CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define LED_EnableInterruptForLowLevelSensing() do { PORTC.PIN3CTRL = (PORTC.PIN3CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PC3_SetInterruptHandler LED_SetInterruptHandler

//get/set TOF_XSHUT aliases
#define TOF_XSHUT_SetHigh() do { PORTF_OUTSET = 0x8; } while(0)
#define TOF_XSHUT_SetLow() do { PORTF_OUTCLR = 0x8; } while(0)
#define TOF_XSHUT_Toggle() do { PORTF_OUTTGL = 0x8; } while(0)
#define TOF_XSHUT_GetValue() (VPORTF.IN & (0x1 << 3))
#define TOF_XSHUT_SetDigitalInput() do { PORTF_DIRCLR = 0x8; } while(0)
#define TOF_XSHUT_SetDigitalOutput() do { PORTF_DIRSET = 0x8; } while(0)
#define TOF_XSHUT_SetPullUp() do { PORTF_PIN3CTRL  |= PORT_PULLUPEN_bm; } while(0)
#define TOF_XSHUT_ResetPullUp() do { PORTF_PIN3CTRL  &= ~PORT_PULLUPEN_bm; } while(0)
#define TOF_XSHUT_SetInverted() do { PORTF_PIN3CTRL  |= PORT_INVEN_bm; } while(0)
#define TOF_XSHUT_ResetInverted() do { PORTF_PIN3CTRL  &= ~PORT_INVEN_bm; } while(0)
#define TOF_XSHUT_DisableInterruptOnChange() do { PORTF.PIN3CTRL = (PORTF.PIN3CTRL & ~PORT_ISC_gm) | 0x0 ; } while(0)
#define TOF_XSHUT_EnableInterruptForBothEdges() do { PORTF.PIN3CTRL = (PORTF.PIN3CTRL & ~PORT_ISC_gm) | 0x1 ; } while(0)
#define TOF_XSHUT_EnableInterruptForRisingEdge() do { PORTF.PIN3CTRL = (PORTF.PIN3CTRL & ~PORT_ISC_gm) | 0x2 ; } while(0)
#define TOF_XSHUT_EnableInterruptForFallingEdge() do { PORTF.PIN3CTRL = (PORTF.PIN3CTRL & ~PORT_ISC_gm) | 0x3 ; } while(0)
#define TOF_XSHUT_DisableDigitalInputBuffer() do { PORTF.PIN3CTRL = (PORTF.PIN3CTRL & ~PORT_ISC_gm) | 0x4 ; } while(0)
#define TOF_XSHUT_EnableInterruptForLowLevelSensing() do { PORTF.PIN3CTRL = (PORTF.PIN3CTRL & ~PORT_ISC_gm) | 0x5 ; } while(0)
#define PF3_SetInterruptHandler TOF_XSHUT_SetInterruptHandler

/**
 * @ingroup  pinsdriver
 * @brief GPIO and peripheral I/O initialization
 * @param none
 * @return none
 */
void PIN_MANAGER_Initialize();

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for MISO pin. 
 *        This is a predefined interrupt handler to be used together with the MISO_SetInterruptHandler() method.
 *        This handler is called every time the MISO ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void MISO_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for MISO pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for MISO at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void MISO_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for MOSI pin. 
 *        This is a predefined interrupt handler to be used together with the MOSI_SetInterruptHandler() method.
 *        This handler is called every time the MOSI ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void MOSI_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for MOSI pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for MOSI at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void MOSI_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for SCK pin. 
 *        This is a predefined interrupt handler to be used together with the SCK_SetInterruptHandler() method.
 *        This handler is called every time the SCK ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void SCK_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for SCK pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for SCK at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void SCK_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for VBAT pin. 
 *        This is a predefined interrupt handler to be used together with the VBAT_SetInterruptHandler() method.
 *        This handler is called every time the VBAT ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void VBAT_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for VBAT pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for VBAT at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void VBAT_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for SCL pin. 
 *        This is a predefined interrupt handler to be used together with the SCL_SetInterruptHandler() method.
 *        This handler is called every time the SCL ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void SCL_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for SCL pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for SCL at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void SCL_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for SDA pin. 
 *        This is a predefined interrupt handler to be used together with the SDA_SetInterruptHandler() method.
 *        This handler is called every time the SDA ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void SDA_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for SDA pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for SDA at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void SDA_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for TOF_GPIO pin. 
 *        This is a predefined interrupt handler to be used together with the TOF_GPIO_SetInterruptHandler() method.
 *        This handler is called every time the TOF_GPIO ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void TOF_GPIO_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for TOF_GPIO pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for TOF_GPIO at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void TOF_GPIO_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for SW pin. 
 *        This is a predefined interrupt handler to be used together with the SW_SetInterruptHandler() method.
 *        This handler is called every time the SW ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void SW_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for SW pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for SW at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void SW_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD0 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD0_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD0 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD0_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD0 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD0 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD0_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD1 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD1_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD1 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD1_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD1 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD1 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD1_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD2 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD2_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD2 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD2_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD2 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD2 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD2_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD3 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD3_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD3 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD3_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD3 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD3 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD3_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD4 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD4_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD4 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD4_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD4 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD4 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD4_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD5 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD5_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD5 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD5_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD5 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD5 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD5_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD6 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD6_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD6 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD6_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD6 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD6 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD6_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PD7 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PD7_SetInterruptHandler() method.
 *        This handler is called every time the IO_PD7 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PD7_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PD7 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PD7 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PD7_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for VBUS_DET pin. 
 *        This is a predefined interrupt handler to be used together with the VBUS_DET_SetInterruptHandler() method.
 *        This handler is called every time the VBUS_DET ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void VBUS_DET_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for VBUS_DET pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for VBUS_DET at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void VBUS_DET_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for IO_PF5 pin. 
 *        This is a predefined interrupt handler to be used together with the IO_PF5_SetInterruptHandler() method.
 *        This handler is called every time the IO_PF5 ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void IO_PF5_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for IO_PF5 pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for IO_PF5 at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void IO_PF5_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for FLASH_CS pin. 
 *        This is a predefined interrupt handler to be used together with the FLASH_CS_SetInterruptHandler() method.
 *        This handler is called every time the FLASH_CS ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void FLASH_CS_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for FLASH_CS pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for FLASH_CS at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void FLASH_CS_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for LED pin. 
 *        This is a predefined interrupt handler to be used together with the LED_SetInterruptHandler() method.
 *        This handler is called every time the LED ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void LED_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for LED pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for LED at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void LED_SetInterruptHandler(void (* interruptHandler)(void)) ; 

/**
 * @ingroup  pinsdriver
 * @brief Default Interrupt Handler for TOF_XSHUT pin. 
 *        This is a predefined interrupt handler to be used together with the TOF_XSHUT_SetInterruptHandler() method.
 *        This handler is called every time the TOF_XSHUT ISR is executed. 
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param none
 * @return none
 */
void TOF_XSHUT_DefaultInterruptHandler(void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt Handler Setter for TOF_XSHUT pin input-sense-config functionality.
 *        Allows selecting an interrupt handler for TOF_XSHUT at application runtime
 * @pre PIN_MANAGER_Initialize() has been called at least once
 * @param InterruptHandler function pointer.
 * @return none
 */
void TOF_XSHUT_SetInterruptHandler(void (* interruptHandler)(void)) ; 
#endif /* PINS_H_INCLUDED */
