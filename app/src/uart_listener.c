#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/led_indicator_changed.h>

// test code
#include <logging/log.h>
LOG_MODULE_DECLARE(main);

static void uart_cb(const struct device *dev, void *user_data) {
    uart_irq_update(dev);
    while (uart_irq_rx_ready(dev)) {
        uart_fifo_read(dev, buf, 1);
        LOG_INF("Got UART byte: 0x%02X", buf[0]);

        uint8_t code = buf[0];
        if (code & 0x80) {
            zmk_hid_release(code & 0x7F);
        } else {
            zmk_hid_press(code);
        }
    }
}

// end of test code 

#define UART_NODE DT_NODELABEL(uart0)
const struct device *uart = DEVICE_DT_GET(UART_NODE);

static uint8_t buf[1];

static void uart_cb(const struct device *dev, void *user_data) {
    uart_irq_update(dev);
    while (uart_irq_rx_ready(dev)) {
        uart_fifo_read(dev, buf, 1);
        uint8_t code = buf[0];

        if (code & 0x80) {
            zmk_hid_release(code & 0x7F);
        } else {
            zmk_hid_press(code);
        }
    }
}

static int led_event_listener(const struct zmk_event_header *eh) {
    const struct zmk_led_indicator_changed *ev = as_zmk_led_indicator_changed(eh);

    uint8_t report = 0;

    if (ev->caps_lock) report |= 0x01;
    if (ev->num_lock)  report |= 0x02;
    if (ev->scroll_lock) report |= 0x04;

    uart_poll_out(uart, 0xC0 | report);

    return 0;
}
ZMK_LISTENER(led_uart, led_event_listener);
ZMK_SUBSCRIPTION(led_uart, zmk_led_indicator_changed);


int uart_listener_init(void) {
    if (!device_is_ready(uart)) return -1;

    uart_irq_callback_user_data_set(uart, uart_cb, NULL);
    uart_irq_rx_enable(uart);
    return 0;
}

SYS_INIT(uart_listener_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
