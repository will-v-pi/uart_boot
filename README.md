Compile this, then do these (or drag & drop in same order)
```
picotool load uart_boot.uf2
picotool reboot
picotool load blink.uf2
picotool reboot
```

Then attach another RP2350 device to UART1 (TX 4, RX 5) according to the UART boot datasheet section (with appropriate pulls etc), and reset it - it should be booted with a blink binary blinking GPIO 25