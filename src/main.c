/*			  ========================================================
**			  --------DHT11-single-wire-interface-nRF54-zephyr--------
**			  ========================================================
													
******* Author:a6a9aia *******
*** <a5a8ahaiac@proton.me> ***

***  The aim of this project is to interface a DHT11 temperature and humidity sensor ***
***  with nRF45DK board and zephyr RTOS ************************************************
***************************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(DHT11);

#define DHT_PORT  			DT_NODELABEL(gpio0)
#define DHT_PIN				4
#define MEASURE_FREQ_MS		500

static const struct device *dht_gpio;
static int dht11_read(uint8_t data[5]){
    int i, j;
    uint32_t start;

    for (i = 0; i < 5; i++){
		data[i] = 0;
	}
	
    gpio_pin_configure(dht_gpio, DHT_PIN, GPIO_OUTPUT);
    gpio_pin_set(dht_gpio, DHT_PIN, 0);
    k_msleep(20);
    gpio_pin_set(dht_gpio, DHT_PIN, 1);
    k_busy_wait(30);
    gpio_pin_configure(dht_gpio, DHT_PIN, GPIO_INPUT | GPIO_PULL_UP);

    start = k_cycle_get_32();
    while (gpio_pin_get(dht_gpio, DHT_PIN)) {
        if (k_cyc_to_us_floor32(k_cycle_get_32() - start) > 100) return -1;
    }

    while (!gpio_pin_get(dht_gpio, DHT_PIN));
    while (gpio_pin_get(dht_gpio, DHT_PIN));

    for (i = 0; i < 40; i++) {
        while (!gpio_pin_get(dht_gpio, DHT_PIN)); // detecting first bit
        start = k_cycle_get_32();
        while (gpio_pin_get(dht_gpio, DHT_PIN));
        uint32_t pulse = k_cyc_to_us_floor32(k_cycle_get_32() - start);

        j = i / 8;
        data[j] <<= 1;
        if (pulse > 50) {
            data[j] |= 1;
        }
    }
    if (((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4]) { // checksum
        return -2;
    }
    return 0;
}

int main(void){
    uint8_t buf[5];
    dht_gpio = DEVICE_DT_GET(DHT_PORT);
    if (!device_is_ready(dht_gpio)) {
		LOG_WRN("GPIO device not ready\n");
        return -1;
    }

    while (1) {
        int ret = dht11_read(buf);
        if (ret == 0) {
            int humidity = buf[0];
            int temperature = buf[2];
			LOG_INF("Humidity: %d %%  Temp: %d °C", humidity, temperature);
        } else {
			LOG_INF("Erreur lecture DHT11 (code %d)", ret);
        }
        k_msleep(MEASURE_FREQ_MS);
    }
}
