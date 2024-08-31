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

compartment("i2c_example")
    add_deps("debug")
    add_files("i2c_example.cc")

compartment("proximity_sensor_example")
    add_deps("debug")
    add_files("proximity_sensor_example.cc")
