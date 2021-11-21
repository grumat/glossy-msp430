# Implementation Details for the JTAG over SPI interface


## Performance

Step                        | Bit-Bang | JTAG over SPI
----------------------------|:--------:|:------------:
Reset TAP to device id read |   810 µs |     540 µs
Ready to Connect            |  7.25 ms |    6.84 ms

> **Note:** Database lookup takes typical 6.1 ms, that represents most of the time until 
> the bridge is ready to connect.

