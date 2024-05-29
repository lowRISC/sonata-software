// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
#include <ctype.h>
#include <debug.hh>
#include <platform-i2c.hh>
#include <platform-rgbctrl.hh>
#include <thread.h>

const uint8_t APDS9960_ENABLE = 0x80;
const uint8_t APDS9960_ID     = 0x92;
const uint8_t APDS9960_PPC    = 0x8E;
const uint8_t APDS9960_CR1    = 0x8F;
const uint8_t APDS9960_PDATA  = 0x9C;

const uint8_t APDS9960_ID_EXP = 0xAB;
const uint8_t APDS9960_I2C_ADDRESS = 0x39;

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "proximity sensor example">;

template<class T>
using Mmio = volatile T *;

static void setup_proximity_sensor(Mmio<OpenTitanI2c> i2c, const uint8_t addr) {
  uint8_t buf[2];

  buf[0] = APDS9960_ID;

  i2c->write(APDS9960_I2C_ADDRESS, buf, 1, false);
  bool success = i2c->read(APDS9960_I2C_ADDRESS, buf, 1);
  Debug::Assert(success, "Failed to read proximity sensor ID");

	Debug::log("Proximity sensor ID {}", buf[0]);

  Debug::Assert(buf[0] == APDS9960_ID_EXP,
      "Proximity sensor ID was not expected value of {}, saw {}",
      APDS9960_ID_EXP, buf[0]);

  // Disable everything
  buf[0] = APDS9960_ENABLE;
  buf[1] = 0x0;
  i2c->write(APDS9960_I2C_ADDRESS, buf, 2, true);
  // Wait for all engines to go idle
  thread_millisecond_wait(25);

  // Set PEN (proximity enable) and PON (power on)
  buf[0] = APDS9960_ENABLE;
  buf[1] = 0x5;
  i2c->write(APDS9960_I2C_ADDRESS, buf, 2, true);
  // Wait for power on
  thread_millisecond_wait(10);

  // Set proximity gain to 8x
  buf[0] = APDS9960_CR1;
  buf[1] = 0x0c;
  i2c->write(APDS9960_I2C_ADDRESS, buf, 2, true);

  // Set proximity pulse length to 4us and pulse count to 16us (experimentially
  // determined, other values may work better!)
  buf[0] = APDS9960_PPC;
  buf[1] = 0x04;
  i2c->write(APDS9960_I2C_ADDRESS, buf, 2, true);
}

static uint8_t read_proximity_sensor(Mmio<OpenTitanI2c> i2c) {
  uint8_t buf[1];

  buf[0] = APDS9960_PDATA;
  i2c->write(APDS9960_I2C_ADDRESS, buf, 1, false);
  if (!i2c->read(APDS9960_I2C_ADDRESS, buf, 1)) {
    Debug::log("Failed to read proximity sensor value");
    return 0;
  }


  return buf[0];
}

[[noreturn]] void __cheri_compartment("proximity_sensor_example") run() {
	auto i2cSetup = [](Mmio<OpenTitanI2c> i2c) {
		i2c->reset_fifos();
		i2c->set_host_mode();
		i2c->set_speed(1);
	};

	auto i2c0 = MMIO_CAPABILITY(OpenTitanI2c, i2c0);
	i2cSetup(i2c0);
  uint8_t addr;

  auto rgbled = MMIO_CAPABILITY(SonataRGBLEDCtrl, rgbled);

  setup_proximity_sensor(i2c0, APDS9960_I2C_ADDRESS);

  while(true) {
    uint8_t prox = read_proximity_sensor(i2c0);
    Debug::log("Proximity is {}\r", prox);
    rgbled->set_rgb(((prox) >> 3), 0, 0, 0);
    rgbled->set_rgb(0, (255 - prox) >> 3, 0, 1);
    rgbled->update();

    thread_millisecond_wait(100);
  }
}
