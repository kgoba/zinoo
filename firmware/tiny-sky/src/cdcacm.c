/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>

#include "cdcacm.h"

// #define CRITICAL_STORE bool irq_flags___;
// #define CRITICAL_START irq_flags___ = cm_mask_interrupts(true);
// #define CRITICAL_END   (void) cm_mask_interrupts(irq_flags___);

#define CRITICAL_START { uint8_t irq_state___ = nvic_get_irq_enabled(NVIC_USB_IRQ); nvic_disable_irq(NVIC_USB_IRQ);
#define CRITICAL_END   if (irq_state___) nvic_enable_irq(NVIC_USB_IRQ); }

#define QUEUE_SIZE 128

typedef struct {
  uint32_t  _size;
  uint32_t  _head;
  uint8_t   _buffer[QUEUE_SIZE];
} queue_t;

static void q_clear(queue_t *q) {
	q->_size = 0;
	q->_head = 0;
}

static uint8_t q_push(queue_t *q, uint8_t item) {
	if (q->_size < QUEUE_SIZE) {
		uint32_t tail = q->_head + q->_size;
		if (tail >= QUEUE_SIZE) tail -= QUEUE_SIZE;
		q->_buffer[tail] = item;
		q->_size++;      
		return 1;
	}
	return 0;
}

static uint8_t q_pop(queue_t *q, uint8_t *item) {
	if (q->_size > 0) {
		*item = q->_buffer[q->_head];
		q->_head++;
		if (q->_head == QUEUE_SIZE) q->_head = 0;
		q->_size--;
		return 1;
	}
	return 0;
}

static uint32_t q_used(const queue_t *q) {
	return q->_size;
}

static uint32_t q_free(const queue_t *q) {
	return QUEUE_SIZE - q->_size;
}

static queue_t rx_queue;
static queue_t tx_queue;


static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
} };

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
} };

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 }
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors)
} };

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
} };

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
} };

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char * usb_strings[] = {
	"Black Sphere Technologies",
	"CDC-ACM Demo",
	"DEMO",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

volatile static uint8_t cdc_configured;

static enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev,
	struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
        if(req->wValue){ // terminal is opened
            cdc_configured = 1;
        } else{ // terminal is closed
            cdc_configured = 0;
        }
		return USBD_REQ_HANDLED;
	}
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding)) {
			return USBD_REQ_NOTSUPP;
		}

		return USBD_REQ_HANDLED;
	}
	return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	//if (!cdc_configured) return;

	char buf[64];
	int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

	if (len) {
		char *ptr = buf;
		while (len > 0) {
			if (!q_push(&rx_queue, *ptr)) break;
			ptr++;
			len--;
		}
		//while (usbd_ep_write_packet(usbd_dev, 0x82, buf, len) == 0);
	}
}

static volatile uint8_t tx_done;

static void cdcacm_data_tx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	//if (!cdc_configured) return;

	unsigned len = 0;
	uint8_t buf[32] __attribute__ ((aligned(4)));

	CRITICAL_START {
		uint8_t *buf_ptr = buf;
		while (len < 32 && q_pop(&tx_queue, buf_ptr)) {
			buf_ptr++;
			len++;
		}
	} CRITICAL_END

	// For some reason VCP misses characters when we send ZLP (zero length packets)
	if (len > 0) {
		// So we avoid it
		usbd_ep_write_packet(usbd_dev, 0x82, buf, len);
	}
	else {
		// Only after we have received a callback and there was no new data to be sent, 
		// we can declare that TX is complete. 
		tx_done = 1;
	}
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	// Set up EP callbacks
	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_tx_cb);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);

	cdc_configured = 1;
}

static usbd_device *usbd_dev;

static void usb_clock_enable() {
	// Setup USB clock
	SYSCFG_CFGR3 |= SYSCFG_CFGR3_ENREF_HSI48 | SYSCFG_CFGR3_EN_VREFINT;
	while (!(SYSCFG_CFGR3 & SYSCFG_CFGR3_VREFINT_RDYF)) {
		// Wait until VREFINT becomes ready
	}
	rcc_osc_on(RCC_HSI48);
	rcc_wait_for_osc_ready(RCC_HSI48);
	rcc_set_hsi48_source_rc48();
}

void usb_setup() {
	usb_clock_enable();

	// Enable peripheral clocks
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USB);

	q_clear(&tx_queue);
	q_clear(&rx_queue);
	tx_done = 1;

	usbd_dev = usbd_init(&st_usbfs_v2_usb_driver, &dev, &config,
			usb_strings, 3,
			usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
}

void usb_enable_interrupts() {
	nvic_enable_irq(NVIC_USB_IRQ);	
}

static int usb_cdc_write_blocking(const uint8_t *buf, int len) {
	while (usbd_ep_write_packet(usbd_dev, 0x82, buf, len) == 0 && len > 0) {
		;
	}
	if (len == 64) {
		usbd_ep_write_packet(usbd_dev, 0x82, buf, 0);
	}
	return 0;
}

static int usb_cdc_write_queue(const uint8_t *buf, int len) {
	CRITICAL_START 
		while (len) {
			if (!q_push(&tx_queue, *buf)) break;
			buf++;
			len--;
		}
	CRITICAL_END
	if (tx_done) {
		tx_done = 0;
		cdcacm_data_tx_cb(usbd_dev, 0x82);
	}
	return 0;
}

int usb_cdc_write(const uint8_t *buf, int len) {
	if (!cdc_configured) return -1;
	//return usb_cdc_write_blocking(buf, len);
	return usb_cdc_write_queue(buf, len);
}

int usb_cdc_read(uint8_t *buf, int len) {
	//if (len > 64) len = 64;
	//return usbd_ep_read_packet(usbd_dev, 0x01, buf, len);
	int n_read = 0;
	while (len > 0) {
		if (!q_pop(&rx_queue, buf)) break;
		n_read++;
		len--;
		buf++;
	}
	return n_read;
}

void usb_poll() {
	usbd_poll(usbd_dev);
}

void usb_isr() {
	usbd_poll(usbd_dev);
}
