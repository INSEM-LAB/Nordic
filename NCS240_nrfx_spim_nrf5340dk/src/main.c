/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <nrfx_spim.h>

/** @brief Symbol specifying SPIM instance to be used. */
#define SPIM_INST_IDX 3

/** @brief Symbol specifying pin number for MOSI. */
#define MOSI_PIN 13

/** @brief Symbol specifying pin number for MISO. */
#define MISO_PIN 14

/** @brief Symbol specifying pin number for SCK. */
#define SCK_PIN 17

/** @brief Symbol specifying pin number for CS. */
#define CS_PIN 18

/** @brief Transmit buffer */
static uint8_t m_tx_buffer[255];

/** @brief Receive buffer */
static uint8_t m_rx_buffer[255];

void main(void)
{
	nrfx_err_t status;
	printk("Hello World! %s\n", CONFIG_BOARD);

	nrfx_spim_t spim_inst = NRFX_SPIM_INSTANCE(SPIM_INST_IDX);
	nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG(SCK_PIN, MOSI_PIN, MISO_PIN, CS_PIN);

	status = nrfx_spim_init(&spim_inst, &spim_config, NULL, NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);

	//nrfx_spim_xfer_desc_t spim_xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buffer, sizeof(m_tx_buffer), m_rx_buffer, sizeof(m_rx_buffer));

	//serial flash RDID(Read identification)	// 1 byte manufacturer ID & 2 bytes device ID
	m_tx_buffer[0] = 0x9F;
	nrfx_spim_xfer_desc_t spim_xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buffer, 1, m_rx_buffer, 4);

	status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);
	printk("%02x %02x %02x %02x\n", m_rx_buffer[0], m_rx_buffer[1], m_rx_buffer[2], m_rx_buffer[3]);

	while(1)
	{
		status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc, 0);
    	NRFX_ASSERT(status == NRFX_SUCCESS);
		printk("%02x %02x %02x %02x\n", m_rx_buffer[0], m_rx_buffer[1], m_rx_buffer[2], m_rx_buffer[3]);
		k_sleep(K_MSEC(1000));
	}
}
