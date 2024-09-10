<!--
Copyright lowRISC Contributors.
SPDX-License-Identifier: Apache-2.0
-->
# sonata-automotive-demo

## Description

This directory contains the common library code that is used to implement the
automotive demo applications for the Sonata board. The library is designed
to be hardware-independent, where access to necessary hardware, MMIO and
functionality is implemented through the use of a set of callback functions
that any implementer will provide. This allows the demo to be implemented
for two environments required for showcasing the demo - one on the Sonata
board running CHERIoT-RTOS with CHERIoT enabled, and another running
baremetal on Sonata without CHERIoT enabled, to show the difference in the
demo behaviour with CHERI running and not running.

Alongside a menu to select between them (found in `automotive_menu.h`), this
library defines four different demo applications that can be run, based on
the availability of hardware:

1. **No Pedal**: If not using a pedal or the GPIO joystick, this demo simply
implements a counter and shows some pseudocode to show how the out-of-bounds
array write bug occurs. Acceleration is constant, and the overwrite will
unexpectedly change the transmitted acceleration value.

2. **Joystick Pedal**: If no physical pedal is available, the joystick pedal
allows you to control a "simulated" pedal's acceleration value by using the
joystick. The bug is similarly manually triggered using the joystick to cause
the same type of out-of-bounds array write, causing a similar issue in the
transmitted acceleration value.

3. **Digital Pedal**: If a physical pedal is available, but an analogue input
or ADC is not, then this application can be used to treate the pedal input as
a digital input, and pass this through to the receiving board which will then
run in simulation mode to simulate the car's speed over time. The bug is the
same as in the joystick pedal, being manually triggered via the joystick.

4. **Analogue Pedal**: If a physical pedal is available and can be read via
the ADC, then this application should be used as the **primary application**.
It features a much more feature complete and aesthetic/visual example, in
which analogue pedal data is read and transmitted as expected, but is paired
with a second "volume control" task in which the user can use the joystick to
control a volume slider shown on the LCD. An intentional bug lets the volume
overflow by one, and calculation of the volume bar's colour to write into
a framebuffer overwrites into the acceleration pedal's memory, causing an
excessively large value to be transmitted and total loss of control.

## Usage

To use this library, first initialise LCD information and implement and
declare relevant callbacks, such as in the following example code.

```c
init_lcd(lcd.parent.width, lcd.parent.height);
init_callbacks((AutomotiveCallbacks){
    .uart_send           = write_to_uart,
    .wait                = wait,
    .waitTime            = 120,
    .time                = get_elapsed_time,
    .loop                = null_callback,
    .start               = null_callback,
    .joystick_read       = read_joystick,
    .digital_pedal_read  = read_pedal_digital,
    .analogue_pedal_read = read_pedal_analogue,
    .ethernet_transmit   = send_ethernet_frame,
    .lcd =
        {
            .draw_str        = lcd_draw_str,
            .clean           = lcd_clean,
            .fill_rect       = lcd_fill_rect,
            .draw_img_rgb565 = lcd_draw_img,
        },
});
```

After this, the `select_demo()` function in `automotive_menu.h` can be used to
display a menu on the LCD to allow the user to select a demo application, which
will be returned.

Each demo application offers a function like `init_$APP_NAME_mem` and
`run_$APP_NAME`. Before running an application you must first initialise its
memory, creating the required structs that it will use and passing them to the
former function. For demo purposes, you should ensure that these structs are
**contiguous in memory**, such that task two's memory is directly before task
one's memory, in each of the demo instances. After doing this, you can then
call the corresponding `run` function, which takes the current time as an
input as defined relative to your `time` callback function.
