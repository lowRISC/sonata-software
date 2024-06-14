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

compartment("gpiolib")
    -- This compartment uses C++ thread-safe static initialisation and so
    -- depends on the C++ runtime.
    add_deps("cxxrt")
    add_files("gpiolib.cc")

compartment("led_walk")
    add_files("led_walk.cc")

compartment("proximity_sensor_example")
    add_deps("debug")
    add_files("proximity_sensor_example.cc")
