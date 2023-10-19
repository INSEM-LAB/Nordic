/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */
#include "uart_async_adapter.h"

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <bluetooth/services/nus.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#include <stdio.h>

#include <zephyr/logging/log.h>

#include <pm/pm.h>
#include <pm/device.h>
#include <pm/policy.h>
#include <hal/nrf_gpio.h>

#include <bluetooth/hci_vs.h>
#include <sys/byteorder.h>
#include <sys/reboot.h>

#define LOG_MODULE_NAME peripheral_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

#define CON_STATUS_LED DK_LED2

#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define UART_BUF_SIZE CONFIG_BT_NUS_UART_BUFFER_SIZE
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_WAIT_FOR_RX CONFIG_BT_NUS_UART_RX_WAIT_TIME

#define INPUT_CHAR_CNT_MAX              19
#define SLEEP_S 						2U
#define INPUT_TX_NUMBER_CNT_MAX         2
#define INPUT_NUMBER_CNT_MAX            4

static K_SEM_DEFINE(ble_init_ok, 0, 1);

static struct bt_conn *current_conn;
static struct bt_conn *auth_conn;

static const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(nordic_nus_uart));
static struct k_work_delayable uart_work;

struct uart_data_t {
	void *fifo_reserved;
	uint8_t data[UART_BUF_SIZE];
	uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static char cur_device_name[] = DEVICE_NAME;
static int cur_txpwr = (int)RADIO_TXPOWER_TXPOWER_0dBm;
static uint32_t cur_adv_interval = BT_GAP_ADV_FAST_INT_MIN_2;

//static const struct bt_data ad[] = {
//	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
//};

static const struct bt_data ad_chg[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, cur_device_name, sizeof(cur_device_name)),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

#if CONFIG_BT_NUS_UART_ASYNC_ADAPTER
UART_ASYNC_ADAPTER_INST_DEFINE(async_adapter);
#else
static const struct device *const async_adapter;
#endif

static void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl)
{
	struct bt_hci_cp_vs_write_tx_power_level *cp;
	struct bt_hci_rp_vs_write_tx_power_level *rp;
	struct net_buf *buf, *rsp = NULL;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
				sizeof(*cp));
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;
	cp->tx_power_level = tx_pwr_lvl;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
				   buf, &rsp);
	if (err) {
		uint8_t reason = rsp ?
			((struct bt_hci_rp_vs_write_tx_power_level *)
			  rsp->data)->status : 0;
		printk("Set Tx power err: %d reason 0x%02x\n", err, reason);
		return;
	}

	rp = (void *)rsp->data;
	//printk("Actual Tx Power: %d\n", rp->selected_tx_power);

	net_buf_unref(rsp);
}

#if 0
static void get_tx_power(uint8_t handle_type, uint16_t handle, int8_t *tx_pwr_lvl)
{
	struct bt_hci_cp_vs_read_tx_power_level *cp;
	struct bt_hci_rp_vs_read_tx_power_level *rp;
	struct net_buf *buf, *rsp = NULL;
	int err;

	*tx_pwr_lvl = 0xFF;
	buf = bt_hci_cmd_create(BT_HCI_OP_VS_READ_TX_POWER_LEVEL,
				sizeof(*cp));
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_TX_POWER_LEVEL,
				   buf, &rsp);
	if (err) {
		uint8_t reason = rsp ?
			((struct bt_hci_rp_vs_read_tx_power_level *)
			  rsp->data)->status : 0;
		printk("Read Tx power err: %d reason 0x%02x\n", err, reason);
		return;
	}

	rp = (void *)rsp->data;
	*tx_pwr_lvl = rp->tx_power_level;

	net_buf_unref(rsp);
}
#endif

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	static size_t aborted_len;
	struct uart_data_t *buf;
	static uint8_t *aborted_buf;
	static bool disable_req;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("UART_TX_DONE");
		if ((evt->data.tx.len == 0) ||
		    (!evt->data.tx.buf)) {
			return;
		}

		if (aborted_buf) {
			buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
					   data);
			aborted_buf = NULL;
			aborted_len = 0;
		} else {
			buf = CONTAINER_OF(evt->data.tx.buf, struct uart_data_t,
					   data);
		}

		k_free(buf);

		buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		if (!buf) {
			return;
		}

		if (uart_tx(uart, buf->data, buf->len, SYS_FOREVER_MS)) {
			LOG_WRN("Failed to send data over UART");
		}

		break;

	case UART_RX_RDY:
		LOG_DBG("UART_RX_RDY");
		buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data);
		buf->len += evt->data.rx.len;

		if (disable_req) {
			return;
		}

		if ((evt->data.rx.buf[buf->len - 1] == '\n') ||
		    (evt->data.rx.buf[buf->len - 1] == '\r')) {
			disable_req = true;
			uart_rx_disable(uart);
		}

		break;

	case UART_RX_DISABLED:
		LOG_DBG("UART_RX_DISABLED");
		disable_req = false;

		buf = k_malloc(sizeof(*buf));
		if (buf) {
			buf->len = 0;
		} else {
			LOG_WRN("Not able to allocate UART receive buffer");
			k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
			return;
		}

		uart_rx_enable(uart, buf->data, sizeof(buf->data),
			       UART_WAIT_FOR_RX);

		break;

	case UART_RX_BUF_REQUEST:
		LOG_DBG("UART_RX_BUF_REQUEST");
		buf = k_malloc(sizeof(*buf));
		if (buf) {
			buf->len = 0;
			uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
		} else {
			LOG_WRN("Not able to allocate UART receive buffer");
		}

		break;

	case UART_RX_BUF_RELEASED:
		LOG_DBG("UART_RX_BUF_RELEASED");
		buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t,
				   data);

		if (buf->len > 0) {
			k_fifo_put(&fifo_uart_rx_data, buf);
		} else {
			k_free(buf);
		}

		break;

	case UART_TX_ABORTED:
		LOG_DBG("UART_TX_ABORTED");
		if (!aborted_buf) {
			aborted_buf = (uint8_t *)evt->data.tx.buf;
		}

		aborted_len += evt->data.tx.len;
		buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
				   data);

		uart_tx(uart, &buf->data[aborted_len],
			buf->len - aborted_len, SYS_FOREVER_MS);

		break;

	default:
		break;
	}
}

static void uart_work_handler(struct k_work *item)
{
	struct uart_data_t *buf;

	buf = k_malloc(sizeof(*buf));
	if (buf) {
		buf->len = 0;
	} else {
		LOG_WRN("Not able to allocate UART receive buffer");
		k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
		return;
	}

	uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_WAIT_FOR_RX);
}

static bool uart_test_async_api(const struct device *dev)
{
	const struct uart_driver_api *api =
			(const struct uart_driver_api *)dev->api;

	return (api->callback_set != NULL);
}

static int uart_init(void)
{
	int err;
	int pos;
	struct uart_data_t *rx;
	struct uart_data_t *tx;

	if (!device_is_ready(uart)) {
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
		err = usb_enable(NULL);
		if (err) {
			LOG_ERR("Failed to enable USB");
			return err;
		}
	}

	rx = k_malloc(sizeof(*rx));
	if (rx) {
		rx->len = 0;
	} else {
		return -ENOMEM;
	}

	k_work_init_delayable(&uart_work, uart_work_handler);


	if (IS_ENABLED(CONFIG_BT_NUS_UART_ASYNC_ADAPTER) && !uart_test_async_api(uart)) {
		/* Implement API adapter */
		uart_async_adapter_init(async_adapter, uart);
		uart = async_adapter;
	}

	err = uart_callback_set(uart, uart_cb, NULL);
	if (err) {
		LOG_ERR("Cannot initialize UART callback");
		return err;
	}

	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		LOG_INF("Wait for DTR");
		while (true) {
			uint32_t dtr = 0;

			uart_line_ctrl_get(uart, UART_LINE_CTRL_DTR, &dtr);
			if (dtr) {
				break;
			}
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
		}
		LOG_INF("DTR set");
		err = uart_line_ctrl_set(uart, UART_LINE_CTRL_DCD, 1);
		if (err) {
			LOG_WRN("Failed to set DCD, ret code %d", err);
		}
		err = uart_line_ctrl_set(uart, UART_LINE_CTRL_DSR, 1);
		if (err) {
			LOG_WRN("Failed to set DSR, ret code %d", err);
		}
	}

	tx = k_malloc(sizeof(*tx));

	if (tx) {
//		pos = snprintf(tx->data, sizeof(tx->data),
//			       "Starting Nordic UART service example\r\n");
		pos = snprintf(tx->data, sizeof(tx->data), "AT command example\r\n");

		if ((pos < 0) || (pos >= sizeof(tx->data))) {
			k_free(tx);
			LOG_ERR("snprintf returned %d", pos);
			return -ENOMEM;
		}

		tx->len = pos;
	} else {
		return -ENOMEM;
	}

	err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
	if (err) {
		LOG_ERR("Cannot display welcome message (err: %d)", err);
		return err;
	}

	return uart_rx_enable(uart, rx->data, sizeof(rx->data), 50);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	uint16_t current_conn_handle;
	int ret;

	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", log_strdup(addr));

	current_conn = bt_conn_ref(conn);

	ret = bt_hci_get_conn_handle(current_conn, &current_conn_handle);
	if(ret)
	{
		printk("No connection handle (err %d)\n", ret);
		sys_reboot(SYS_REBOOT_COLD);
		return;
	}

	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN, current_conn_handle, cur_txpwr);

	dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

	if (auth_conn) {
		bt_conn_unref(auth_conn);
		auth_conn = NULL;
	}

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		dk_set_led_off(CON_STATUS_LED);
	}

	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, cur_txpwr);
}

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr),
			level, err);
	}
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected,
#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", log_strdup(addr), passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	auth_conn = bt_conn_ref(conn);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", log_strdup(addr), passkey);
	LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr),
		bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing failed conn: %s, reason %d", log_strdup(addr),
		reason);
}


static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
#endif

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	LOG_INF("Received data from: %s", log_strdup(addr));

	for (uint16_t pos = 0; pos != len;) {
		struct uart_data_t *tx = k_malloc(sizeof(*tx));

		if (!tx) {
			LOG_WRN("Not able to allocate UART send data buffer");
			return;
		}

		/* Keep the last byte of TX buffer for potential LF char. */
		size_t tx_data_size = sizeof(tx->data) - 1;

		if ((len - pos) > tx_data_size) {
			tx->len = tx_data_size;
		} else {
			tx->len = (len - pos);
		}

		memcpy(tx->data, &data[pos], tx->len);

		pos += tx->len;

		/* Append the LF character when the CR character triggered
		 * transmission from the peer.
		 */
		if ((pos == len) && (data[len - 1] == '\r')) {
			tx->data[tx->len] = '\n';
			tx->len++;
		}

		err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
		if (err) {
			k_fifo_put(&fifo_uart_tx_data, tx);
		}
	}
}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

void error(void)
{
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin for ever */
		k_sleep(K_MSEC(1000));
	}
}

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
static void num_comp_reply(bool accept)
{
	if (accept) {
		bt_conn_auth_passkey_confirm(auth_conn);
		LOG_INF("Numeric Match, conn %p", (void *)auth_conn);
	} else {
		bt_conn_auth_cancel(auth_conn);
		LOG_INF("Numeric Reject, conn %p", (void *)auth_conn);
	}

	bt_conn_unref(auth_conn);
	auth_conn = NULL;
}

void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (auth_conn) {
		if (buttons & KEY_PASSKEY_ACCEPT) {
			num_comp_reply(true);
		}

		if (buttons & KEY_PASSKEY_REJECT) {
			num_comp_reply(false);
		}
	}
}
#endif /* CONFIG_BT_NUS_SECURITY_ENABLED */

static void configure_gpio(void)
{
	int err;

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}
#endif /* CONFIG_BT_NUS_SECURITY_ENABLED */

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

void main(void)
{
	int blink_status = 0;
	int err = 0;

	configure_gpio();

	err = uart_init();
	if (err) {
		error();
	}

	if (IS_ENABLED(CONFIG_BT_NUS_SECURITY_ENABLED)) {
		err = bt_conn_auth_cb_register(&conn_auth_callbacks);
		if (err) {
			printk("Failed to register authorization callbacks.\n");
			return;
		}

		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err) {
			printk("Failed to register authorization info callbacks.\n");
			return;
		}
	}

	err = bt_enable(NULL);
	if (err) {
		error();
	}

	LOG_INF("Bluetooth initialized");

	k_sem_give(&ble_init_ok);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return;
	}

//	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
//			      ARRAY_SIZE(sd));
	err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
	   						cur_adv_interval, \
	   						cur_adv_interval, NULL), ad_chg, ARRAY_SIZE(ad_chg), sd, ARRAY_SIZE(sd));
							
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, cur_txpwr);

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}

void ble_write_thread(void)
{
	uint8_t cmd_array[UART_BUF_SIZE]={0};

	char cmd_at[4]="AT?\r";
    char cmd_ver[8]="AT+VER?\r";
    char cmd_role[9]="AT+ROLE?\r";
	char cmd_advstart[12]="AT+ADVSTART\r";
	char cmd_advstop[11]="AT+ADVSTOP\r";
    char cmd_set_advint[10]="AT+ADVINT=";
    char cmd_get_advint[11]="AT+ADVINT?\r";
    char cmd_set_txpwr[9]="AT+TXPWR=";
    char cmd_get_twpwr[10]="AT+TXPWR?\r";
    char cmd_set_devname[8]="AT+NAME=";
    char cmd_get_devname[9]="AT+NAME?\r";
	char cmd_sleep[9]="AT+SLEEP\r";

	uint8_t interval_input[4]={0};
	uint8_t tx_pwr_input[2]={0};
	int err = 0;
	int tmp;
	uint8_t i,j,k;

	/* Don't go any further until BLE is initialized */
	k_sem_take(&ble_init_ok, K_FOREVER);

	for (;;) {
		/* Wait indefinitely for data to be sent over bluetooth */
		struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
						     K_FOREVER);

		if(current_conn != NULL)
		{
			if (bt_nus_send(NULL, buf->data, buf->len)) {
				LOG_WRN("Failed to send data over BLE connection");
			}
		}
		else
		{
			memcpy(cmd_array, buf->data, sizeof(buf->data));

 			/* checks if there is any uart data for at command */
        	if(cmd_array[0]!=0)
        	{
            	if(!strncmp(cmd_at, cmd_array, sizeof(cmd_at))) /* AT?\r */
            	{
               		printk("OK\n"); 
            	}
				else if(!strncmp(cmd_ver, cmd_array, sizeof(cmd_ver)))  /* AT+VER?\r */
				{
					printk("VER 1.0.0\n");
					printk("OK\n");
				}
				else if(!strncmp(cmd_role, cmd_array, sizeof(cmd_role)))  /* AT+ROLE?\r */
				{
					printk("Peripheral\n");
					printk("OK\n");
				}
				else if(!strncmp(cmd_advstart, cmd_array, sizeof(cmd_advstart)))  /* AT+ADVSTART\r */
				{
					err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				    						cur_adv_interval, \
				    						cur_adv_interval, NULL), ad_chg, ARRAY_SIZE(ad_chg), sd, ARRAY_SIZE(sd));
					printk("OK\n"); 
				}
				else if(!strncmp(cmd_advstop, cmd_array, sizeof(cmd_advstop)))  /* AT+ADVSTOP\r */
				{
					bt_le_adv_stop();
					printk("OK\n"); 
				}
				else if(!strncmp(cmd_set_advint, cmd_array, sizeof(cmd_set_advint)))  /* AT+ADVINT= */
				{
					/* AT command sequence 1)AT+ADVSTOP 2) AT+NAME= */

					/* checks end of string '\r'*/
					for(i=0;i<INPUT_CHAR_CNT_MAX;i++)
					{
						if(cmd_array[10+i]=='\r')
						{
							break;
						}
					}

					/* checks maximum character count of number -> 0(1 character) ~ 9999(4 character) */
					if(i>INPUT_NUMBER_CNT_MAX)
					{
						/* it's only accepted up to 4 number character. */
						printk("Error\n");
					}
					else
					{
						/* copy input ASCII number string to destination array after changing real number */
						k=i;
						for(j=0;j<i;j++)
						{
							interval_input[3-j] = cmd_array[10+(k-1)] - 0x30;
							k--;
						}

						if((interval_input[0]>9) || (interval_input[1]>9) || (interval_input[2]>9) || (interval_input[3]>9))
						{
							/* array have to have values from 0 to 9. */
							printk("Error\n");
						}
						else
						{
							cur_adv_interval = interval_input[0]*1000 + interval_input[1]*100 + interval_input[2]*10 + interval_input[3];

							err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				       								cur_adv_interval, \
				       								cur_adv_interval, NULL), ad_chg, ARRAY_SIZE(ad_chg), sd, ARRAY_SIZE(sd));
							printk("OK\n"); 
						}
					}

					/* clear advertising interval input array */
					memset(interval_input, 0, sizeof(interval_input));
				}
				else if(!strncmp(cmd_get_advint, cmd_array, sizeof(cmd_get_advint)))  /* AT+ADVINT?\r */
				{
					printk("%d\n", cur_adv_interval);
					printk("OK\n");
				}
				else if(!strncmp(cmd_set_txpwr, cmd_array, sizeof(cmd_set_txpwr)))  /* AT+TXPWR= */
				{
					/* checks end of string '\r'*/
					for(i=0;i<INPUT_CHAR_CNT_MAX;i++)
					{
						if(cmd_array[10+i]=='\r')
						{
							break;
						}
					}

					/* checks maximum character count of number -> -40(2 character except minus character) ~ +4(1 character except plus character) */
					if(i>INPUT_TX_NUMBER_CNT_MAX)
					{
						printk("Error\n");
					}
					else
					{
						/* copy input ASCII number string to destination array after changing real number */
						k=i;
						for(j=0;j<i;j++)
						{
							tx_pwr_input[1-j] = cmd_array[10+(k-1)] - 0x30;
							k--;
						}

						if((tx_pwr_input[0]>9) || (tx_pwr_input[1]>9) || ((cmd_array[9]!='+') && (cmd_array[9]!='-')))
						{
							/* array have to have values from 0 to 9. */
							printk("Error\n");
						}
						else
						{
							tmp = tx_pwr_input[0]*10 + tx_pwr_input[1];

							if(cmd_array[9]=='+')
							{
								/* Supported tx_power values: 0dBm, 3dBm, 4dBm */
								if((tmp!=0) && (tmp!=3) && (tmp!=4))
								{
									printk("Error\n");
								}
								else
								{
									cur_txpwr = tmp;
									set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, cur_txpwr);
									printk("OK\n");
								}
							}
							else
							{
								tmp *= (-1);
								/* Supported tx_power values: -40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm */
								if((tmp!=-40) && (tmp!=-20) && (tmp!=-16) && (tmp!=-12) && (tmp!=-8) && (tmp!=-4))
								{
									printk("Error\n");
								}
								else
								{
									cur_txpwr = tmp;
									set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, cur_txpwr);
									printk("OK\n");
								}
							}
						}

						/* clear tx power input array */
						memset(tx_pwr_input, 0, sizeof(tx_pwr_input));
					}
				}
				else if(!strncmp(cmd_get_twpwr, cmd_array, sizeof(cmd_get_twpwr)))  /* AT+TXPWR?\r */
				{
					if(cur_txpwr>=0)
					{
						printk("+%d\r\n", cur_txpwr);
					}
					else
					{
						printk("%d\r\n", cur_txpwr);
					}
					printf("OK\r\n");
				}
				else if(!strncmp(cmd_set_devname, cmd_array, sizeof(cmd_set_devname)))  /* AT+NAME= */
				{
					/* AT command sequence 1)AT+ADVSTOP 2)AT+NAME= */

					/* checks end of string '\r'*/
					for(i=0;i<INPUT_CHAR_CNT_MAX;i++)
					{
						if(cmd_array[8+i]=='\r')
						{
							break;
						}
					}

					if((i>INPUT_CHAR_CNT_MAX) || (i==0))
					{
						printk("Error\n");
					}
					else
					{
						memset(cur_device_name, 0, sizeof(cur_device_name));

						/* copy input string to destination array */
						strncpy(cur_device_name, cmd_array+8, i);

						err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				    						cur_adv_interval, \
				    						cur_adv_interval, NULL), ad_chg, ARRAY_SIZE(ad_chg), sd, ARRAY_SIZE(sd));
						printk("OK\n"); 
					}

				}
				else if(!strncmp(cmd_get_devname, cmd_array, sizeof(cmd_get_devname)))  /* AT+NAME?\r */
				{
					printk("%s\n", cur_device_name);
					printk("OK\n");
				}
				else if(!strncmp(cmd_sleep, cmd_array, sizeof(cmd_sleep)))  /* AT+SLEEP\r */
				{
					/* v2.2.0\zephyr\samples\boards\nrf\system_off example is used. */
					uart_rx_disable(uart);

					pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
					k_sleep(K_SECONDS(SLEEP_S)); 
				}
				else
				{
					printk("Error\n");
				}

				/* clear AT command array */
            	memset(cmd_array, 0, sizeof(cmd_array));
			}
		}

		k_free(buf);
	}
}

K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL,
		NULL, PRIORITY, 0, 0);


