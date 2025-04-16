#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zmk/hid.h>

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

int uart_listener_init(void) {
    if (!device_is_ready(uart)) return -1;

    uart_irq_callback_user_data_set(uart, uart_cb, NULL);
    uart_irq_rx_enable(uart);
    return 0;
}

SYS_INIT(uart_listener_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
