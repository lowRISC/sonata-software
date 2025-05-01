-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

library("lcd")
  set_default(false)
  add_files("../third_party/display_drivers/src/core/lcd_base.c")
  add_files("../third_party/display_drivers/src/core/m3x6_16pt.c")
  add_files("../third_party/display_drivers/src/core/lucida_console_10pt.c")
  add_files("../third_party/display_drivers/src/core/lucida_console_12pt.c")
  add_files("../third_party/display_drivers/src/st7735/lcd_st7735.c")
  add_files("lcd.cc")

library("sense_hat")
  set_default(false)
  add_files("sense_hat.cc")
