
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <device.h>

#ifndef CONFIG_NRF52840_RESET_STATE
#define CONFIG_NRF52840_RESET_STATE 0
#endif

#define NRF52840_RESET_STATE CONFIG_NRF52840_RESET_STATE

int bt_hci_transport_setup(struct device *h4)
{
	int err;
	char c;
	const struct device *port;

	port = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	if (!port) {
		return -EIO;
	}

	/* Configure pin as output and initialize it to low. */
	err = gpio_pin_configure(port, CONFIG_NRF52840_RESET_PIN, NRF52840_RESET_STATE ? GPIO_OUTPUT_LOW : GPIO_OUTPUT_HIGH);
	if (err) {
		return err;
	}

	/* Reset the nRF52840 and let it wait until the pin is
	 * pulled low again before running to main to ensure
	 * that it won't send any data until the H4 device
	 * is setup and ready to receive.
	 */
	err = gpio_pin_set(port, CONFIG_NRF52840_RESET_PIN, NRF52840_RESET_STATE);
	if (err) {
		return err;
	}

	/* Wait for the nRF52840 peripheral to stop sending data.
	 *
	 * It is critical (!) to wait here, so that all bytes
	 * on the lines are received and drained correctly.
	 */
	k_sleep(K_MSEC(10));

	/* Drain bytes */
	while (uart_fifo_read(h4, &c, 1)) {
		continue;
	}

	/* We are ready, let the nRF52840 run to main */
	err = gpio_pin_set(port, CONFIG_NRF52840_RESET_PIN, !NRF52840_RESET_STATE);
	if (err) {
		return err;
	}

	return 0;
}
