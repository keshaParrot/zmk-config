#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <logging/log.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/led_indicator_changed.h>

LOG_MODULE_REGISTER(uart_listener, LOG_LEVEL_INF);

#define UART_NODE DT_NODELABEL(uart0)
static const struct device *uart = DEVICE_DT_GET(UART_NODE);
static uint8_t buf[1];

static void uart_cb(const struct device *dev, void *user_data) {
    uart_irq_update(dev);
    while (uart_irq_rx_ready(dev)) {
        uart_fifo_read(dev, buf, 1);
        LOG_INF("Got UART byte: 0x%02X", buf[0]);
        uint8_t code = buf[0];
        if (code & 0x80) zmk_hid_release(code & 0x7F);
        else             zmk_hid_press(code);
    }
}

static int led_event_listener(const struct zmk_event_header *eh) {
    const struct zmk_led_indicator_changed *ev = as_zmk_led_indicator_changed(eh);
    uint8_t report = (ev->caps_lock   ? 1 : 0)
                   | (ev->num_lock    ? 2 : 0)
                   | (ev->scroll_lock ? 4 : 0);
    uart_poll_out(uart, 0xC0 | report);
    return 0;
}
ZMK_LISTENER(led_uart, led_event_listener);
ZMK_SUBSCRIPTION(led_uart, zmk_led_indicator_changed);

static int uart_listener_init(const struct device *dev) {
    ARG_UNUSED(dev);
    if (!device_is_ready(uart)) {
        LOG_ERR("UART0 not ready");
        return -ENODEV;
    }
    uart_irq_callback_user_data_set(uart, uart_cb, NULL);
    uart_irq_rx_enable(uart);
    return 0;
}
SYS_INIT(uart_listener_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
