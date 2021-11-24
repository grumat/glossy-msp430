# Implementation Details for the JTAG over SPI interface

JTAG is a clear implementation that uses an SPI bus, with the exception that the TMS signal
is not provided by a standard SPI peripheral.

The SPI on the STM32 is not so flexible in means of bit rate. The max allowed clock 
for the JTAG bus is 10 MHz. This turns out that the max possible SPI clock is 9 MHz
(72 Mhz / 8).

Another option that we can explorer is configurable clock speeds, for the case a part is 
working at a lower supply voltages. Interesting options are 9, 4.5, 2.25 and 1.125 MHz.

## Generating the TMS signal

So the solution for this problem is to pump the TCK into a timer (TIM2) and through a
counter and compare, trigger a DMA transfer to load the next compare value.

JTAG sequences requires more bits than the data size. Typically 5 to 6 clocks added to 
the head and tail of the data. SPI on the STM32F1xx family can only handle 8 or 16-bit
transfers.

If you check the JTAG state machine you will see that depending on the TMS value the 
state machine does not change. So we can add extra clock cycles without changing the 
state machine context, and this is the solution to the limitation of the SPI transfer 
size. The payload is inserted into a buffer with the additional JTAG state machine
cycles.

For an 8-bit transfer, we write 16-bits and fill dummy bits as necessary. 16-bit 
transfers will require 24-bits and the worst case are the 20-bit transfers that will 
require 32 bits.

The timer output channel is configured in the toggle mode, so we can generate at best 
a TMS pulse with one clock period.  
But because of the hardware limits this mechanisms tops at a clock rate of 4-5 MHz, 
being impossible to generate two DMA request within two consecutive clock pulses 
and keep the TMS in sync.

The timer itself filters the input and causes a typical delay of 65 ns. On the other 
hand, the DMA needs another 65 ns, so, if programming a series pulses a latency of 
130 ns is achieved, overlapping the clock period, which causes all kind of bad things.

But if you analyze the way the TMS behaves, except for the Select-DR state TMS pulses
have 2 clock cycles.

So to overcome this situation a mix of bit banging is performed for the Select-DR 
transfers.

## Implementation details

The current pin-out used for bit-banging, shares the pins of the SPI1 device. Hardware 
changes where necessary on the original concept to provide both possibilities.

Following pin-out was established:
- **TMS:** PA3, also the TIM2CH4.
- **CLK:** PA5, also SPI1 SCK.
- **TDI:** PA7, also SPI1 MOSI.
- **TDO:** PA6, also SPI1 MISO.
- **TI2FP2:** PA1, the timer input.

### TI2FP2

This is the clock input for the TIM2. The input will be driven by the 9 MHz SPI SCK.  
The input is has no prescaler and the filter at the fastest option (2-3 CLK delay).
The timer is setup in slave mode sensitive to the rising edge of the CLK, exactly as 
the JTAG. So a transition at a specific clock rising edge refers to the next cycle on 
the JTAG state machine, since at least 65 ns will be added internally by the timer.

The timer is configured in single shot upcount mode with the MAX value, as resets 
are always done by software.

Note that timer registers may be configured to have shadow registers. These needs to
be disabled as we want everything happening on-the-fly.

The timer output is done on channel 4 in toggle mode. Values in CCR4 that matches the 
counter will cause a transition on the output to the inverse position of the current 
value.

### DMA Channel 7

The CH4 Compare triggers the Channel 7 of the DMA1 device. The idea is to set at the
source of the DMA transfer a buffer in RAM (RAM is faster than Flash) which increments
after every transfer and the target is the CCR4 register. This table contains a list
of clock cycle that causes a toggle in the TMS pulse.

The first compare value is pre-loaded by the firmware and the others are loaded every
time a hit occurs, programming the next hit.

To overcome the problem with the Select-DR state the TMS is pulled up by the firmware 
before everything starts and the first bits are all part of the JTAG state machine.
The trailing bits are then all executed in Run/Idle state, just bogus bits to align 
to the SPI transfer.

## Performance

The table below compares the duration of the initialization stage of the firmware.
Values were taken from a logic analyzer.

Step                        | Bit-Bang | JTAG over SPI
----------------------------|:--------:|:------------:
Reset TAP to device id read |   810 µs |     540 µs
Ready to Connect            |  7.25 ms |    6.84 ms

> **Note:** Database lookup takes typical 6.1 ms, that represents most of the time until 
> the firmware is ready to connect.  
> SPI communication proves to be the best option, even with the additional bogus bits
> required for alignment.
> Note that ST-Link clones cannot be used this way as there are no internal connection
> required for the timer.

