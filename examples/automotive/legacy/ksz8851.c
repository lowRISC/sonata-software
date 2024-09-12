// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

// This file is adapted from
// `third_party/sonata-system/sw/legacy/common/demo/ethernet/ksz8851.c`, but
// instead extracts only the functionality relevant to sending Ethernet frames,
// ignoring the rest of the LWIP TCP/IP stack.

#include "ksz8851.h"

#include <string.h>

#include "../../../third_party/sonata-system/sw/legacy/common/rv_plic.h"
#include "../../../third_party/sonata-system/sw/legacy/common/sonata_system.h"
#include "../../../third_party/sonata-system/sw/legacy/common/spi.h"
#include "../../../third_party/sonata-system/sw/legacy/common/timer.h"

// Define our own GPIO_OUT as the version from `sonata-system` uses void
// pointer arithmetic, which clang-tidy forbids.
#define GPIO_OUT_ETH GPIO_FROM_BASE_ADDR((GPIO_BASE + GPIO_OUT_REG))

enum
{
	// Ethernet IRQs
	EthIntrIrq = 47,
	// Ethernet GPIO Output Pins
	EthCsPin  = 13,
	EthRstPin = 14,
};

static struct Netif *ethNetif;

static void timer_delay(uint32_t ms)
{
	uint32_t timeout = get_elapsed_time() + ms;
	while (get_elapsed_time() < timeout)
	{
		__asm__ volatile("wfi");
	}
}

static uint16_t ksz8851_reg_read(spi_t *spi, uint8_t reg)
{
	uint8_t be = (reg & 0x2) == 0 ? 0b0011 : 0b1100;
	uint8_t bytes[2];
	bytes[0] = (0b00 << 6) | (be << 2) | (reg >> 6);
	bytes[1] = (reg << 2) & 0b11110000;

	set_output_bit(GPIO_OUT_ETH, EthCsPin, 0);
	spi_tx(spi, bytes, 2);
	uint16_t val;
	spi_rx(spi, (uint8_t *)&val, 2);
	set_output_bit(GPIO_OUT_ETH, EthCsPin, 1);
	return val;
}

static void ksz8851_reg_write(spi_t *spi, uint8_t reg, uint16_t val)
{
	uint8_t be = (reg & 0x2) == 0 ? 0b0011 : 0b1100;
	uint8_t bytes[2];
	bytes[0] = (0b01 << 6) | (be << 2) | (reg >> 6);
	bytes[1] = (reg << 2) & 0b11110000;

	set_output_bit(GPIO_OUT_ETH, EthCsPin, 0);
	spi_tx(spi, bytes, 2);
	spi_tx(spi, (uint8_t *)&val, 2);
	spi_wait_idle(spi);
	set_output_bit(GPIO_OUT_ETH, EthCsPin, 1);
}

static void ksz8851_reg_set(spi_t *spi, uint8_t reg, uint16_t mask)
{
	uint16_t old = ksz8851_reg_read(spi, reg);
	ksz8851_reg_write(spi, reg, old | mask);
}

static void ksz8851_reg_clear(spi_t *spi, uint8_t reg, uint16_t mask)
{
	uint16_t old = ksz8851_reg_read(spi, reg);
	ksz8851_reg_write(spi, reg, old & ~mask);
}

static void ksz8851_write_mac(spi_t *spi, const uint8_t addr[6])
{
	ksz8851_reg_write(spi, ETH_MARH, (addr[0] << 8) | addr[1]);
	ksz8851_reg_write(spi, ETH_MARM, (addr[2] << 8) | addr[3]);
	ksz8851_reg_write(spi, ETH_MARL, (addr[4] << 8) | addr[5]);
}

static void ksz8851_dump(spi_t *spi)
{
#ifdef KSZ8851_DEBUG_PRINT
	uint16_t phy_status = ksz8851_reg_read(spi, ETH_P1MBSR);
	putstr("PHY status is ");
	puthexn(phy_status, 4);
	puts("");

	uint16_t port_status = ksz8851_reg_read(spi, ETH_P1SCLMD);
	putstr("Port special status is ");
	puthexn(port_status, 4);
	puts("");

	port_status = ksz8851_reg_read(spi, ETH_P1SR);
	putstr("Port status is ");
	puthexn(port_status, 4);
	puts("");

	uint16_t isr_status = ksz8851_reg_read(spi, ETH_ISR);
	putstr("ISR status is ");
	puthexn(isr_status, 4);
	puts("");

	uint16_t p1cr = ksz8851_reg_read(spi, ETH_P1CR);
	putstr("P1CR status is ");
	puthexn(p1cr, 4);
	puts("");

	uint16_t p1mbcr = ksz8851_reg_read(spi, ETH_P1MBCR);
	putstr("P1MBCR status is ");
	puthexn(p1mbcr, 4);
	puts("");
#endif
}

// Will block output - waits till the transmit buffer is available
bool ksz8851_output(struct Netif *netif, struct Fbuf *buf)
{
	spi_t *spi = netif->spi;

	// Wait until transmit buffer is available.
	while (1)
	{
		uint16_t txmir = ksz8851_reg_read(spi, ETH_TXMIR) & 0x0FFF;
		if (txmir < buf->len + 4)
		{
#ifdef KSZ8851_DEBUG_PRINT
			puts("KSZ8851: Transmit buffer full");
#endif
			continue;
		}
		break;
	}

	// Disable IRQ to avoid interrupting DMA transfer.
	uint32_t flags = arch_local_irq_save();

	// Start QMU DMA transfer operation
	ksz8851_reg_set(spi, ETH_RXQCR, StartDmaAccess);

	// Start transmission.
	uint8_t cmd = 0b11 << 6;
	set_output_bit(GPIO_OUT_ETH, EthCsPin, 0);
	spi_tx(spi, &cmd, 1);

	uint32_t header = 0x8000 | (buf->len << 16);
	spi_tx(spi, (uint8_t *)&header, 4);

	if (buf->len != 0)
	{
		spi_tx(spi, (uint8_t *)(buf->payload), buf->len);
	}

	static const uint8_t Padding[3] = {0, 0, 0};
	// The transmission needs to be dword-aligned, so we pad the packet to 4
	// bytes.
	uint32_t pad = (-buf->len) & 0x3;
	if (pad != 0)
	{
		spi_tx(spi, Padding, pad);
	}

	spi_wait_idle(spi);
	set_output_bit(GPIO_OUT_ETH, EthCsPin, 1);

	// Stop QMU DMA transfer operation
	ksz8851_reg_clear(spi, ETH_RXQCR, StartDmaAccess);

	// TxQ Manual-Enqueue
	ksz8851_reg_set(spi, ETH_TXQCR, ManualEnqueueTxQFrameEnable);

	arch_local_irq_restore(flags);

	return true;
}

static void ksz8851_irq_handler(irq_t irq)
{
	spi_t   *spi = ethNetif->spi;
	uint32_t isr = ksz8851_reg_read(spi, ETH_ISR);
	if (!isr)
	{
		return;
	}

	// Acknowledging the interrupts.
	ksz8851_reg_write(spi, ETH_ISR, isr);
}

bool ksz8851_get_phy_status(struct Netif *netif)
{
	spi_t *spi = netif->spi;
	return (ksz8851_reg_read(spi, ETH_P1MBSR) & (1 << 5)) > 0;
}

bool ksz8851_init(struct Netif *netif, uint8_t hwaddr[6])
{
	spi_t *spi = netif->spi;
	if (!spi)
		return false;

	// Reset chip
	set_output_bit(GPIO_OUT_ETH, EthRstPin, 0);
	timer_delay(150);
	set_output_bit(GPIO_OUT_ETH, EthRstPin, 0x1);

	uint16_t cider = ksz8851_reg_read(spi, ETH_CIDER);
#ifdef KSZ8851_DEBUG_PRINT
	putstr("KSZ8851: Chip ID is ");
	puthexn(cider, 4);
	puts("");
#endif

	// Check the chip ID. The last nibble is revision ID and can be ignored.
	if ((cider & 0xFFF0) != 0x8870)
	{
		puts("KSZ8851: Unexpected Chip ID");
		return false;
	}

	// Write the MAC address
	ksz8851_write_mac(spi, hwaddr);
#ifdef KSZ8851_DEBUG_PRINT
	putstr("KSZ8851: MAC address is ");
	for (int i = 0; i < 6; i++)
	{
		if (i != 0)
			putchar(':');
		puthexn(hwaddr[i], 2);
	}
	puts("");
#endif

	// Enable QMU Transmit Frame Data Pointer Auto Increment
	ksz8851_reg_write(spi, ETH_TXFDPR, 0x4000);
	// Enable QMU Transmit flow control / Transmit padding / Transmit CRC, and
	// IP/TCP/UDP checksum generation.
	ksz8851_reg_write(spi, ETH_TXCR, 0x00EE);
	// Enable QMU Receive Frame Data Pointer Auto Increment.
	ksz8851_reg_write(spi, ETH_RXFDPR, 0x4000);
	// Configure Receive Frame Threshold for one frame.
	ksz8851_reg_write(spi, ETH_RXFCTR, 0x0001);
	// Enable QMU Receive flow control / Receive all broadcast frames /Receive
	// unicast frames, and IP/TCP/UDP checksum verification etc.
	ksz8851_reg_write(spi, ETH_RXCR1, 0x7CE0);
	// Enable QMU Receive UDP Lite frame checksum verification, UDP Lite frame
	// checksum generation, IPv6 UDP fragment frame pass, and IPv4/IPv6 UDP UDP
	// checksum field is zero pass. In addition (not in the programmer's guide),
	// enable single-frame data burst.
	ksz8851_reg_write(spi, ETH_RXCR2, 0x009C);
	// Enable QMU Receive Frame Count Threshold / RXQ Auto-Dequeue frame.
	ksz8851_reg_write(
	  spi, ETH_RXQCR, RxFrameCountThresholdEnable | AutoDequeueRxQFrameEnable);

	// Programmer's guide have a step to set the chip in half-duplex when
	// negotiation failed, but we omit the step.

	// Restart Port 1 auto-negotiation
	ksz8851_reg_set(spi, ETH_P1CR, 1 << 13);

	// Configure Low Watermark to 6KByte available buffer space out of 12KByte.
	ksz8851_reg_write(spi, ETH_FCLWR, 0x0600);
	// Configure High Watermark to 4KByte available buffer space out of 12KByte.
	ksz8851_reg_write(spi, ETH_FCHWR, 0x0400);

	// Clear the interrupt status
	ksz8851_reg_write(spi, ETH_ISR, 0xFFFF);
	// Enable Link Change/Transmit/Receive interrupt
	ksz8851_reg_write(spi, ETH_IER, 0xE000);
	// Enable QMU Transmit.
	ksz8851_reg_set(spi, ETH_TXCR, 1 << 0);
	// Enable QMU Receive.
	ksz8851_reg_set(spi, ETH_RXCR1, 1 << 0);

	timer_delay(1000);

	ksz8851_dump(spi);
	netif->mtu = 1500;

	// Initialize IRQ
	ethNetif = netif;
	rv_plic_register_irq(EthIntrIrq, ksz8851_irq_handler);
	rv_plic_enable(EthIntrIrq);

#ifdef KSZ8851_DEBUG_PRINT
	puts("KSZ8851: Initialized");
#endif
	return true;
}
