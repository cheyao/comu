#include <cstdint>

#include "ch32fun.h"

// Example of C++ language usage
constexpr const auto delay_duration = 350;

// A tiny bit of abstraction
class GPIO {
  public:
	GPIO() = delete;
	GPIO(const uint8_t pin) : pin(pin) {
		volatile uint32_t* const pincfg = (&GpioOf(pin)->CFGLR) + ((pin & 0x8) >> 3);
		*pincfg &= ~(0xf << (4 * (pin & 0x7)));
		*pincfg |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * (pin & 0x7));
	}
	~GPIO() {
		volatile uint32_t* const pincfg = (&GpioOf(pin)->CFGLR) + ((pin & 0x8) >> 3);
		*pincfg &= ~(0xf << (4 * (pin & 0x7)));
		*pincfg |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * (pin & 0x7));
	}

	void set(bool state) { GpioOf(pin)->BSHR = (1 << (pin & 0xf)) << (state ? 0 : 16); }
	void toggle(void) { GpioOf(pin)->OUTDR ^= (1 << (pin & 0xf)); }

  protected:
	uint8_t pin;
};

class Led : public GPIO {
  public:
	Led() = delete;
	Led(const uint8_t pin) : GPIO(pin) { GPIO::set(0); }
	~Led() {}

	void blink(void) { GPIO::toggle(); }

  private:
};

int main() {
	SystemInit();
	funGpioInitAll();

	auto left_led = Led(PA4);
	auto right_led = Led(PB11);

	left_led.set(1);

	while (1) {
		left_led.blink();
		right_led.blink();

		Delay_Ms(delay_duration);
	}
}
