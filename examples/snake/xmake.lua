-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

compartment("snake") 
  add_deps("lcd", "debug")
  add_files("snake.cc")
