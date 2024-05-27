The `make_rp2040_uf2.sh` script is temporarily included here as a convenience.
It produces a specially crafted UF2 from an already built sonata firmware UF2.
This can be used to load Sonata firmware via the RP2040 bootloader.

```
./make_rp2040_uf2 /path/to/sonata/firmware.uf2
# Will create /path/to/sonata/firmware_rp2040.uf2 which can be copied to the
# RP2040 boot loader virtual USB drive.
```

Start up in RP2040 bootload mode (press the RP2040 boot button whilst
powering-up). Copy the special UF2 to the RP2040 virtual USB drive. The RP2040
will reboot and load the Sonata firmware.

This will only work with specific versions of the RP2040 firmware
(https://github.com/GregAC/sonata-rp2040/tree/firmware_preload)
