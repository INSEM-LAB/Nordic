/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/adc.h>
#include "slm_util.h"
#include "slm_at_gpio.h"
#include "slm_at_host.h"
#include <hal/nrf_saadc.h>

LOG_MODULE_REGISTER(slm_adc, CONFIG_SLM_LOG_LEVEL);

/* global variable defined in different resources */
extern struct at_param_list at_param_list;
extern char rsp_buf[SLM_AT_CMD_RESPONSE_MAX_LEN];

static const struct device *adc_dev;

/* global variable defined in different files */
extern struct k_work_q slm_work_q;

#define ADC_DEVICE_NAME DT_ADC_0_NAME
#define ADC_RESOLUTION 10
#define ADC_GAIN ADC_GAIN_1_6
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID 0
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN0
#define ADC_2ND_CHANNEL_ID 1
#define ADC_2ND_CHANNEL_INPUT NRF_SAADC_INPUT_AIN1
#define ADC_3RD_CHANNEL_ID 2
#define ADC_3RD_CHANNEL_INPUT NRF_SAADC_INPUT_AIN2
#define ADC_4TH_CHANNEL_ID 3
#define ADC_4TH_CHANNEL_INPUT NRF_SAADC_INPUT_AIN3
#define ADC_5TH_CHANNEL_ID 4
#define ADC_5TH_CHANNEL_INPUT NRF_SAADC_INPUT_AIN4
#define ADC_6ST_CHANNEL_ID 5
#define ADC_6ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN5
#define BUFFER_SIZE 1*5

static const struct adc_channel_cfg m_4th_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_4TH_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_4TH_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg m_3rd_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_3RD_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_3RD_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg m_2nd_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_2ND_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_2ND_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};

static int16_t m_sample_buffer[BUFFER_SIZE];

static int adc_sample(void)
{
	int ret;

	const struct adc_sequence sequence = {
		.channels = BIT(ADC_1ST_CHANNEL_ID) | BIT(ADC_2ND_CHANNEL_ID) | BIT(ADC_3RD_CHANNEL_ID) | BIT(ADC_4TH_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	if (!adc_dev) {
		return -1;
	}

	ret = adc_read(adc_dev, &sequence);
	LOG_DBG("ADC read err: %d\n", ret);

	/* Print the AIN0 values */
	for (int i = 0; i < BUFFER_SIZE; i++) {
		float adc_voltage = 0;
		adc_voltage = (float)(((float)m_sample_buffer[i] / 1023.0f) *
				      3600.0f);
		LOG_DBG("ADC raw value: %d\n", m_sample_buffer[i]);

		sprintf(rsp_buf, "Measured voltage: %f mV\n", adc_voltage);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return ret;
}

/**@brief handle AT#XADC commands
 *  AT#XADC
 *  AT#XADC? READ command not supported
 *  AT#XADC=? TEST command not supported
 */
int handle_at_adc_read(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		adc_sample();
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

int slm_at_adc_init(void)
{
	int err = 0;

	adc_dev = device_get_binding("ADC_0");
	if (!adc_dev) {
		LOG_DBG("device_get_binding ADC_0 failed\n");
	}
	err = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
	if (err) {
		LOG_DBG("Error in adc setup: %d\n", err);
	}
	err = adc_channel_setup(adc_dev, &m_2nd_channel_cfg);
	if (err) {
		LOG_DBG("Error in adc setup: %d\n", err);
	}
	err = adc_channel_setup(adc_dev, &m_3rd_channel_cfg);
	if (err) {
		LOG_DBG("Error in adc setup: %d\n", err);
	}

	err = adc_channel_setup(adc_dev, &m_4th_channel_cfg);
	if (err) {
		LOG_DBG("Error in adc setup: %d\n", err);
	}			

	/* Trigger offset calibration
	 * As this generates a _DONE and _RESULT event
	 * the first result will be incorrect.
	 */
	NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;

	return err;
}

int slm_at_adc_uninit(void)
{
	return 0;
}
