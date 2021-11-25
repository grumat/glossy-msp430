# Implementation Details for the JTAG over SPI interface

This is an example of a 8-bit Shift-IR transfer, using bit-banging:

![Shift IR Command using bit-bang (8-bit)](bit-bang-shift-ir.png "Shift IR Command using bit-bang (8-bit)")

By just seeing the TCK, TDI and TDO signal we can easy see that JTAG is strongly based
implementation of an SPI bus, with the exception that the TMS signal is not provided by 
any standard SPI peripheral.

The idea was to develop a way to emulate the TMS signal using peripherals provided
by the µC.

On the STM32 SPI is not so flexible in means of bit-size and bit rate. Bit sizes on 
the older implementations are 8 or 16 bit and bit rate has not the granularity as the
USART.

The datasheet of the MSP430 states that the max allowed clock for the JTAG bus is 
10 MHz and probably will decrease as supply voltage is reduced.  
This turns out that the max possible SPI clock using the STM32F1xx family is 9 MHz 
(72 Mhz / 8) which is not optimal, but it is far better what we have achieved with 
bit banging.

Because of the diversity of scenarios, another option that we can explore is 
configurable clock speeds, assuming that cabling and supply voltages may cause issues
with a 9 MHz speed. So other interesting options are 4.5, 2.25 and 1.125 MHz. Such
an option using bit banging would be really hard to accomplish.

## Generating the TMS signal

So the solution for this problem is to pump the TCK into a timer (TIM2) and through 
the count/compare feature, trigger a DMA transfer to load the next compare value,
while using the output channel in toggle mode.

JTAG sequences requires more bits than the data size. Typically 5 to 6 bits added to 
the head and tail of the data until the state machine is ready to shift data.  
As said SPI on the STM32F1xx family can only handle 8 or 16-bit transfers.

If you check the JTAG state machine you will see that depending on the TMS value you
can keep the state machine in the same context. So we can add extra clock cycles 
without changing the state machine context and this is the solution to the limitation 
of the SPI transfer size.

![JTAG State Diagram](jtag-state-machine.png "JTAG State Diagram")

So the payload bits are inserted into a larger transfer containing a mix of JTAG state
machine transition and dummy bits so the total cycle count matches two or more 8-bit
SPI transfers.

For an 8-bit transfer, 16-bits are transferred filling dummy bits as necessary. 16-bit 
transfers will require 24-bits transfers and the worst case scenario is the 20-bit 
transfer that will require a total of 32 bits.

The timer output channel is configured in the toggle mode, so we can generate at best 
a TMS pulse with one clock period.  
But because of the hardware limits this mechanisms tops at a clock rate of 4-5 MHz, 
being impossible to generate two DMA request within two consecutive clock pulses 
without causing overruns.

The reasin is that the timer itself filters the input causing a typical delay of 
65 ns. On the other hand, the DMA needs another 65 ns, even if using SRAM as the data 
source. So, a pulse will require a latency of 65 ns on normal case and 130 ns when 
overloading these peripherals, probably because of the round-robin nature of the DMA 
controller. In the overloaded scenario TMS would simply shift and overlap the clock 
pulse at the wrong edge, which causes all kind of bad things.

But for our luck, analyzing the TMS forms, except for the Select-DR state TMS pulses
have 2 clock cycles and in this single situation this happens at the start of a 
transfer.  
So to overcome this situation a mix of bit banging and automated pulse generation is 
performed for the Select-DR transfers.

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
The input is has no pre-scaler and the filter at the fastest option (2-3 CLK delay).
The timer is setup in slave mode sensitive to the rising edge of the CLK, exactly as 
the JTAG. So a transition at a specific clock rising edge refers to the next cycle on 
the JTAG state machine, since at least 65 ns will be added internally by the timer.

The timer is configured in single shot up-count mode with the MAX value, as resets 
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

### TDI Signal (and TCLK)

SLAU320 states the following important information on page 5:

> The TCLK signal is an input clock that must be provided to the target device from 
> an external source. This clock is used internally as the system clock of the target 
> device, MCLK, to load data into memory locations and to clock the CPU. There is no 
> dedicated pin for TCLK; instead, the TDI pin is used as the TCLK input. This occurs
> while the MSP430 TAP controller is in the **Run-Test/Idle state**.

So one important requisite is to keep the TDI state while exiting and returning to the
**Run-Test/Idle state**. This is performed by reading the TDI state before starting
the transmission.

During SPI transfer all bits happening at the **Run-Test/Idle state** will be a copy
of this initial level.

## Putting it All Together

So a SPI transfer has the following steps:
- Setting a table in **RAM** with the transition points of the TMS signal.
- The table is passed to the DMA Channel 7 as source.
- DMA destination is the TIM2 CCR4 register which programs the next pulse toggle
  - So current toggle triggers the DMA that programs the next toggle
  - The duration of this cannot be shorter than 2 SPI clocks because shorter periods
  will cause overruns.
- The initial toggle is loaded into CCR4 before the transfer starts and the rest is 
done by hardware.
- For Shift IR command the first bit is a dummy bit used to start the first DMA 
transfer.
- A Word/Half Word is used as container of all the pulses and the data to be 
transferred is shifted to the correct position.
- All other bits are masked into that container according to the initial level.
- Because this ARM implementation is little-endian and the JTAG/SPI transfer is
big endian the word/half word needs to be reversed with the dedicated ARM 
instructions.
- For the case that the starting TMS pulse is 1 clock wide, all calculated bit
positions are shifted by one and the first transition is done before sending 
the SPI data, using bit-banging.
- Then, all required bytes built into the container word/half word is transmitted.
- The received data is reversed and shifted to the position and the function returns.

The code is implemented using a template class which can be applied to all required 
situations:
- 8-bit Shift-IR
- 8-bit Shift-DR
- 16-bit Shift-DR
- 20-bit Shift-DR

```cpp
enum ScanType : uint8_t
{
	kSelectDR_Scan = 1,
	kSelectIR_Scan = 2,
};


// Template class to handle IR/DR data shifts
template<
	const ScanType scan_size
	, const uint8_t payload_bitsize
	, typename arg_type
	, typename container_type = uint32_t
>
class SpiJtagDataShift
{
public:
	// Container is a POD data that MCU can optimize and fit all stuff
	typedef arg_type arg_type_t;
	// Container is a POD data that MCU can optimize and fit all stuff
	typedef container_type container_t;
	// Total container bit-size
	constexpr static uint8_t kContainerBitSize_ = sizeof(container_t) * 8;
	// Data payload bit-size
	constexpr static uint8_t kPayloadBitSize_ = payload_bitsize;
	// Number of clocks in the tail until update is complete
	constexpr static uint8_t kStartClocks_ = (scan_size > 1 && ExternJClk::kFrequency_ >= 5000000UL);
	// Number of clocks after select (Select DR/IR + Capture DR/IR)
	constexpr static uint8_t kClocksToShift_ = 2;
	// Number of clocks until we enter desired state (one TMS entry + sel + 2 required by state machine)
	constexpr static uint8_t kHeadClocks_ = kStartClocks_ + scan_size + kClocksToShift_;
	// Number of clocks in the tail until update is complete
	constexpr static uint8_t kTailClocks_ = 1;
	// Data should always be aligned to msb
	constexpr static uint8_t kDataShift_ = kContainerBitSize_ - kHeadClocks_ - kPayloadBitSize_;
	// This is the mask to isolate data payload bits
	constexpr static container_t kDataMask_ = ((1 << kPayloadBitSize_) - 1) << kDataShift_;
	// Number of necessary bytes to transfer everything (rounded up with +7/8)
	constexpr static uint32_t kStreamBytes_ = (kHeadClocks_ + kPayloadBitSize_ + kTailClocks_ + 7) / 8;

	//! Shifts data in and out of the JTAG bus
	arg_type_t Transmit(arg_type_t data)
	{
		// static buffer shall be in RAM because flash causes latencies!
		static uint16_t toggles[] =
		{
			// TMS rise (start of state machine)
			kStartClocks_
			// TMS fall Select DR / IR
			, kStartClocks_ + scan_size
			// TMS rise signals last data bit
			, kHeadClocks_ + kPayloadBitSize_ -1
			// After last bit an additional is required to update DR/IR register
			, kHeadClocks_ + kPayloadBitSize_ + kTailClocks_
			// End of sequence: no more requests needed
			, UINT16_MAX
		};

		/*
		** We need to keep TDI level stable during Run-Test/Idle state otherwise
		** it would insert CPU clocks.
		*/
		bool lvl = JTDI::Get();

		// Move bits inside container aligned to msb
		container_t w = (data << kDataShift_);
		// Current TDI level is copied to all unused bits
		if (lvl)
			w |= ~kDataMask_;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		// this is a little-endian machine... (Note: optimizing compiler clears unused conditions)
		if(sizeof(w) == sizeof(uint16_t))
			w = __REV16(w);
		else if (sizeof(w) > sizeof(uint16_t))
			w = __REV(w);
#endif
		container_t r;

		TmsShapeOutTimerChannel::SetCompare(toggles[1-kStartClocks_]);
		TableToTimerDma::Start(&toggles[2-kStartClocks_], TmsShapeOutTimerChannel::GetCcrAddress(), _countof(toggles) - 1);
		TmsShapeTimer::StartShot();

		// PSecial case as DMA cannot handle 1 clock widths in 9 MHz
		if(kStartClocks_ == 0)
		{
			TmsShapeOutTimerChannel::SetOutputMode(kTimOutHigh);
			TmsShapeOutTimerChannel::SetOutputMode(kTimOutToggle);
		}

		SpiJtagDevice::PutStream(&w, &r, kStreamBytes_);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		// this is a little-endian machine... (Note: optimizing compiler clears unused conditions)
		if (sizeof(r) == sizeof(uint16_t))
			r = __REV16(r);
		else if (sizeof(r) > sizeof(uint16_t))
			r = __REV(r);
#endif
		// If payload fits data-type, then cast will mask bits out for us
		if(sizeof(arg_type_t)*8 == kPayloadBitSize_)
			return (arg_type_t)(r >> kDataShift_);
		else
			return (arg_type_t)((r & kDataMask_) >> kDataShift_);
		/* JTAG state = Run-Test/Idle */
	}
};
```

As previously stated, the template can handle all types of transfers. The 8-bit
Shift-IR instance is done this way:

```cpp
uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
	SpiJtagDataShift
	<
		kSelectIR_Scan		// Select IR-Scan JTAG register
		, 8					// 8 bits data
		, uint8_t			// 8 bits data-type fits perfectly
	> jtag;
	uint8_t val = jtag.Transmit(instruction);
	return val;
	// JTAG state = Run-Test/Idle
}
```

The template class looks very complex, but because of the extensive use of the 
`constexpr` keyword compiler optimizes out all unnecessary parts and keeps only
what belongs to that specific case.

A readable assembly listing is done below, and shows haw all hardware stuff and
other exceptions that dos not belong to the case was filtered out by the optimizer 
thanks to the constexpr that was used extensively and allows compiler decide in 
compile time lines that does not apply:

```armasm
	.global	_ZN7JtagDev9OnIrShiftEh
_ZN7JtagDev9OnIrShiftEh:
	push	{lr}	@
	sub	sp, sp, #12	@,,
@ bmt\gpio.h:179: 		return (port->IDR & kBitValue_) != 0;
	ldr	r3, .L130	@ tmp136,
	ldr	r3, [r3, #8]	@ _3, MEM[(volatile struct GPIO_TypeDef *)1073809408B].IDR
@ drivers/JtagDev.spi.cpp:271: 		container_t w = (data << kDataShift_);
	lsls	r1, r1, #19	@ _7, tmp163,
	tst	r3, #128	@ _3,
@ drivers/JtagDev.spi.cpp:274: 			w |= ~kDataMask_;
	it	ne
	ornne	r1, r1, #133693440	@ tmp138, _7,
	str	r1, [sp]	@ tmp138, w
	ldr	r3, [sp]	@ w, w
	rev	r3, r3	@ _10, w
@ drivers/JtagDev.spi.cpp:280: 			w = __REV(w);
	str	r3, [sp]	@ _10, w
@ bmt\timer.h:237: 		case kTimCh4: timer->CCR4 = ccr; break;
	ldr	r1, .L130+4	@ addr.13_14,
	ldrh	r3, [r1], #2	@ _12, toggles[0]
@ bmt\timer.h:237: 		case kTimCh4: timer->CCR4 = ccr; break;
	mov	r2, #1073741824	@ tmp141,
	str	r3, [r2, #64]	@ _12, MEM[(struct TIM_TypeDef *)1073741824B].CCR4
@ bmt\dma.h:253: 		dma->CCR = tmp;
	ldr	r3, .L130+8	@ tmp142,
	movw	r0, #9616	@ tmp143,
	str	r0, [r3, #128]	@ tmp143, MEM[(struct DMA_Channel_TypeDef *)1073873024B].CCR
@ bmt\dma.h:287: 			dma->CMAR = (uint32_t)addr;
	str	r1, [r3, #140]	@ addr.13_14, MEM[(struct DMA_Channel_TypeDef *)1073873024B].CMAR
@ bmt\dma.h:295: 			dma->CPAR = (uint32_t)addr;
	ldr	r1, .L130+12	@ addr.15_15,
	str	r1, [r3, #136]	@ addr.15_15, MEM[(struct DMA_Channel_TypeDef *)1073873024B].CPAR
@ bmt\dma.h:274: 		dma->CNDTR = cnt;
	movs	r1, #4	@ tmp147,
	str	r1, [r3, #132]	@ tmp147, MEM[(struct DMA_Channel_TypeDef *)1073873024B].CNDTR
@ bmt\dma.h:260: 		dma->CCR |= DMA_CCR_EN;
	ldr	r1, [r3, #128]	@ _16, MEM[(struct DMA_Channel_TypeDef *)1073873024B].CCR
	orr	r1, r1, #1	@ _17, _16,
	str	r1, [r3, #128]	@ _17, MEM[(struct DMA_Channel_TypeDef *)1073873024B].CCR
@ bmt\timer.h:862: 			timer_->CNT = 0;
	movs	r3, #0	@ tmp151,
	str	r3, [r2, #36]	@ tmp151, MEM[(struct TIM_TypeDef *)1073741824B].CNT
@ bmt\timer.h:863: 		timer_->EGR = 1;
	.loc 3 863 15 view .LVU675
	movs	r3, #1	@ tmp153,
	str	r3, [r2, #20]	@ tmp153, MEM[(struct TIM_TypeDef *)1073741824B].EGR
@ bmt\timer.h:864: 		timer_->CR1 |= TIM_CR1_CEN;
	.loc 3 864 15 view .LVU676
	ldr	r3, [r2]	@ _18, MEM[(struct TIM_TypeDef *)1073741824B].CR1
	orr	r3, r3, #1	@ _19, _18,
	str	r3, [r2]	@ _19, MEM[(struct TIM_TypeDef *)1073741824B].CR1
@ drivers/JtagDev.spi.cpp:295: 		SpiJtagDevice::PutStream(&w, &r, kStreamBytes_);
	.loc 2 295 27 view .LVU677
	movs	r2, #2	@,
	add	r1, sp, #4	@,,
	mov	r0, sp	@,
	bl	SpiTemplate::PutStream		@
@ CMSIS_HAL/Core/Include/cmsis_gcc.h:903:   return __builtin_bswap32(value);
	.loc 9 903 27 is_stmt 0 view .LVU680
	ldr	r0, [sp, #4]	@ r, r
	rev	r0, r0	@ _21, r
@ drivers/JtagDev.spi.cpp:323: }
	ubfx	r0, r0, #19, #8	@, _21,,
	add	sp, sp, #12	@,,
	ldr	pc, [sp], #4	@

.L130:
	.word	1073809408
	.word	_ZZN16SpiJtagDataShiftIL8ScanType2ELh8EhmE8TransmitEhE7toggles
	.word	1073872896
	.word	1073741888
```

Such a grade of optimization cannot be achieved with C libraries (maybe using LTGO
this is possible).

This is the resulting signal of the function shown above:

![Shift IR Command (8-bit)](shift-ir.png "Shift IR Command (8-bit)")

Note that 14 bits are actually necessary for this typical transfer, but the code here
generates 16 bits, as described before to align to SPI device limitations.

In this example is possible to see a dummy start clock, with TMS in low state, keeping 
the JTAG interface in **Run/Idle State**. Another one is at the tail of the transfer.

Pn the illustration, the dot marks indicates JTAG state transitions, and the data bits 
are on the lower rows. As represented by the Logic Analyzer, the first and the last 
pulses did not affect JTAG state machine and are also not considered as data.  
Very convenient!

> Regardless the additional bits, compare the picture with the first illustration. Bit 
> bang needs more than 7 µs for a whole transfer, while SPI did it with less than 2 µs.

The next example shows the 16-bit Shift-DR transfer:

![Shift DR Command (16-bit)](shift-dr.png "Shift IR Command (16-bit)")

In this illustration you are able to see the bit banging mode issued before the SPI
transfer occurs, rising TMS at the high level (and the huge performance cost to 
accomplish this). At the clock edge, 3 JTAG transitions happens (1-0-0) and are marked 
with dots, then 16 data bits, with the last marked by the rise of TMS, then 2 more 
state transitions. The remainder are the dummy pulses, required for SPI device 
alignment.

## Performance

The table below compares the duration of the initialization stage of the firmware.
Values were taken from a logic analyzer.

Step                        | Bit-Bang | JTAG over SPI
----------------------------|:--------:|:------------:
Reset TAP to device id read |   810 µs |     540 µs
Ready to Connect            |  7.25 ms |    6.84 ms

> **Note:** Database lookup takes typical 6.1 ms, that represents most of the consumed 
> time until the firmware is ready to connect.  
> SPI communication proves to be the best option, even with the additional bogus bits
> required for alignment.
> Note that ST-Link clones cannot be used this way as there are no internal connection
> required for the timer.

> **Note 2:** I will develop a read memory benchmark routine to obtain a more 
> expressive result.
