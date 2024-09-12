// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef __ADC_H
#define __ADC_H

#include <stdbool.h>
#include <stdint.h>

/** ADC Max samples, DRP register size and measurement bit
 * width are defined in the documentation:
 * https://docs.amd.com/r/en-US/ug480_7Series_XADC/Introduction-and-Quick-Start?section=XREF_92049_Introduction
 */
#define ADC_MAX_SAMPLES (1 * 1000 * 1000)
#define ADC_MIN_CLCK_FREQ (1 * 1000 * 1000)
#define ADC_MAX_CLCK_FREQ (26 * 1000 * 1000)
#define ADC_DRP_REG_SIZE 16
#define ADC_BIT_WIDTH 12

/**
 * ADC DRP register offsets (each mapped sequentially to 4 bytes in memory).
 * Source / Documentation:
 * https://docs.amd.com/r/en-US/ug480_7Series_XADC/XADC-Register-Interface
 */
typedef enum AdcDrpRegisterOffset
{
	AdcRegTemperature = 0x00,
	AdcRegVCcint      = 0x01,
	AdcRegVCcaux      = 0x02,
	AdcRegVPN         = 0x03,
	AdcRegVRefP       = 0x04,
	AdcRegVRefN       = 0x05,
	AdcRegVCcbram     = 0x06,
	AdcRegVauxPN0     = 0x10,
	AdcRegVauxPN1     = 0x11,
	AdcRegVauxPN2     = 0x12,
	AdcRegVauxPN3     = 0x13,
	AdcRegVauxPN4     = 0x14,
	AdcRegVauxPN5     = 0x15,
	AdcRegVauxPN6     = 0x16,
	AdcRegVauxPN7     = 0x17,
	AdcRegVauxPN8     = 0x18,
	AdcRegVauxPN9     = 0x19,
	AdcRegVauxPN10    = 0x1A,
	AdcRegVauxPN11    = 0x1B,
	AdcRegVauxPN12    = 0x1C,
	AdcRegVauxPN13    = 0x1D,
	AdcRegVauxPN14    = 0x1E,
	AdcRegVauxPN15    = 0x1F,
	AdcRegConfig0     = 0x40,
	AdcRegConfig1     = 0x41,
	AdcRegConfig2     = 0x42,
} AdcDrpRegisterOffset;

/**
 * ADC Config Register 2 bit field definitions. Source / Documentation:
 * https://docs.amd.com/r/en-US/ug480_7Series_XADC/Control-Registers?section=XREF_53021_Configuration
 */
typedef enum AdcConfig2Field
{
	/* Power-down bits for the XADC. Setting both to 1 powers down the entire
	XADC block, setting PD0=0 and PD1=1 powers down XADC block B permanently.*/
	AdcPowerDownMask = (0x3 << 4),

	/* Bits used to select the division ratio between the DRP clock (DCLK) and
	the lower frequency ADC clock (ADCCLK). Note that values of 0x0 and 0x1
	are mapped to a division of 2 by the DCLK division selection specification.
	All other 8-bit values are mapped identically. That is, the minimum division
	ratio is 2 such that ADCCLK = DCLK/2. */
	AdcClockDividerMask = (0xFF << 8),
} AdcConfig2Field;

/**
 * Power down settings that can be selected using XADC Config Register 2.
 * Source / Documentation:
 * https://docs.amd.com/r/en-US/ug480_7Series_XADC/Control-Registers?section=XREF_93518_Power_Down
 */
typedef enum AdcPowerDownMode
{
	AdcPowerDownNone = 0x0, // Default
	AdcPowerDownAdcB = 0x2,
	AdcPowerDownXadc = 0x3,
} AdcPowerDownMode;

/**
 * An enum representing the offsets of the ADC DRP status registers that are
 * used to store values that are measured/sampled by the XADC.
 */
typedef enum AdcSampleStatusRegister
{
	/**
	 * The mapping of Sonata Arduino Analogue pins to the status registers that
	 * store the results of sampling the auxiliary analogue channels that those
	 * pins are connected to in the XADC. Source / Documentation: Sheet 15 of
	 * https://github.com/newaetech/sonata-pcb/blob/649b11c2fb758f798966605a07a8b6b68dd434e9/sonata-schematics-r09.pdf
	 */
	AdcStatusArduinoA0 = AdcRegVauxPN4,
	AdcStatusArduinoA1 = AdcRegVauxPN12,
	AdcStatusArduinoA2 = AdcRegVauxPN5,
	AdcStatusArduinoA3 = AdcRegVauxPN13,
	AdcStatusArduinoA4 = AdcRegVauxPN6,
	AdcStatusArduinoA5 = AdcRegVauxPN14,

	// Status registers for internal XADC measurements.
	AdcStatusTemperature = AdcRegTemperature,
	AdcStatusVCcint      = AdcRegVCcint,
	AdcStatusVCcaux      = AdcRegVCcaux,
	AdcStatusVRefP       = AdcRegVRefP,
	AdcStatusVRefN       = AdcRegVRefN,
	AdcStatusVCcbram     = AdcRegVCcbram,
} AdcSampleStatusRegister;

typedef uint8_t AdcClockDivider;

/**
 * The result of measurements / analog conversions stored in DRP status
 * registers are 12 bit values that are stored MSB justified.
 */
#define ADC_MEASUREMENT_MASK 0xFFF0

/**
 * ADC DRP registers are 16 bits, but each 16 bit register is mapped to
 * the lower bits of 1 word (4 bytes) in memory.
 */
typedef uint32_t *AdcReg;

#define ADC_FROM_BASE_ADDR(addr) ((AdcReg)(addr))
#define ADC_FROM_ADDR_AND_OFFSET(addr, offset) ((AdcReg)((addr) + (offset)))

typedef struct Adc
{
	AdcReg           baseReg;
	AdcClockDivider  divider;
	AdcPowerDownMode pd;
} Adc;

void    adc_init(Adc *adc, AdcReg baseReg, AdcClockDivider divider);
void    adc_set_clock_divider(Adc *adc, AdcClockDivider divider);
void    adc_set_power_down(Adc *adc, AdcPowerDownMode pd);
int16_t read_adc(Adc *adc, AdcSampleStatusRegister reg);

#endif // ADC_H__
