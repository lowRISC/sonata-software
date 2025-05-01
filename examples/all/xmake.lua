-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

compartment("led_walk_raw")
    add_deps("debug")
    add_files("led_walk_raw.cc")

compartment("echo")
    add_files("echo.cc")

compartment("lcd_test")
    add_deps("lcd")
    add_files("lcd_test.cc")

compartment("lcd_test_xl")
    add_deps("lcd")
    add_files("lcd_test_xl.cc")

compartment("i2c_example")
    add_deps("debug")
    add_files("i2c_example.cc")

compartment("rgbled_lerp")
    add_files("rgbled_lerp.cc")

compartment("proximity_sensor_example")
    add_deps("debug", "sense_hat")
    add_files("proximity_sensor_example.cc")

compartment("sense_hat_demo")
    add_deps("debug", "sense_hat")
    add_files("sense_hat_demo.cc", "../../third_party/display_drivers/src/core/m3x6_16pt.c")
