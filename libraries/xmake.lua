-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

library("lcd")
  set_default(false)
  add_files("../third_party/display_drivers/core/lcd_base.c")
  add_files("../third_party/display_drivers/core/m3x6_16pt.c")
  add_files("../third_party/display_drivers/st7735/lcd_st7735.c")
  add_files("lcd.cc")
