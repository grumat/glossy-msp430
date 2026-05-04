> STM32F103 question and Bare metal register programming. I need to create a pulse sequence using 1 output clock pin and 1 data pin (in/out) using a regular GPIO. I want to send and receive bits to form up to 48 cycles of data. I don't want to use polling, because it is CPU intensive and has big timing issues. I wanted a way using timers/DMA or whatever the hardware offers me to automate the process. Lets describe each cycle. Put a bit from source buffer A on data pin during the rising edge of the clock. THis first rising edge should also se the daa pin in output mode. Put the next data bit from buffer B on the next rising edge of the clock. On the third and last clock cycle, set on the next rising edge the direction of data pin as input and read the data pin on the falling edge and store it in the buufer C. This s actually the Spy-Bi-Wire protocol used by Texas Instruments for the MSP430. Details about the buffer. Data layout can be flexible enough so that hardware constrains can be bypassed. Buffers can also be joined for the same reason. So the CPU can have a "data prepare phase" before a complete data cycle starts, which runs at full speed. My target is that hardware flushes data automatically, while CPU prepares the next phase on a ping/pong basis. Clock speed should be adjustable up to the hardware supported limit, before overruns can happen (DMA, etc).

* * *

You’re basically trying to build a tiny, deterministic “bit engine” out of timers + DMA so the CPU just feeds phases—nice problem.

### High‑level idea

You can do this on an F103 with:

- **One timer** generating the clock on a GPIO pin (PWM).
- **DMA (memory→peripheral)** writing:

    - the data bit to the GPIO **BSRR** (set/reset) at each rising edge,
    - the data pin **direction** (input/output) via GPIO **CRL/CRH** when needed.
- **DMA (peripheral→memory)** reading the data pin from **GPIOx\_IDR** at the falling edge where you want to sample. [Stack Overflow](https://stackoverflow.com/questions/68476220/stm32-use-dma-to-generate-bit-pattern-on-gpio-pin?utm_source=copilot.com)

The CPU’s job becomes: prepare three small “scripts” (A/B/C phase buffers) in RAM, then arm DMA + timer and let the hardware flush them out. While one script is running, you prepare the next (ping‑pong).

### Hardware blocks and roles

**1. Timer (e.g. TIM2 or TIM3)**

- **CH1** in PWM mode, output on a GPIO pin = your **clock**.
- Period = one bit time, 50% duty cycle.
- Use **update event or compare events** to trigger DMA requests at:

    - rising edge (for “put bit / change direction”),
    - falling edge (for “sample bit”).
	
**GPIO for data pin (e.g. PA0)**

- Data pin is a normal GPIO, not alternate function.
- You will:

    - write to **GPIOx\_BSRR** to set/reset the data bit,
    - write to **GPIOx\_CRL/CRH** to switch between input and output mode,
    - read **GPIOx\_IDR** to sample the incoming bit.

**3. DMA channels**

- **DMA channel A (mem→periph)**

    - Peripheral address: `&GPIOx->BSRR` (and optionally `&GPIOx->CRL/CRH` via a second channel).
    - Trigger: timer event at **rising edge**.
    - Source buffer: precomputed words that encode “set data high/low” and, at the right cycle, “change direction to input/output”.
- **DMA channel B (periph→mem)**

    - Peripheral address: `&GPIOx->IDR`.
    - Trigger: timer event at **falling edge** (only for the cycle where you want to read).
    - Destination buffer: buffer C (or a small staging buffer you then pack into C).

### Mapping to your 3‑cycle Spy‑Bi‑Wire frame

You described one “frame” as 3 clock cycles:

1. **Cycle 1 (rising edge)**

    - Set data pin to **output**.
    - Output bit from **buffer A** on data pin.
2. **Cycle 2 (rising edge)**

    - Keep data pin as output.
    - Output bit from **buffer B**.
3. **Cycle 3 (rising edge + falling edge)**

    - On rising edge: switch data pin to **input**.
    - On falling edge: **sample** data pin and store in **buffer C**.

You can implement that as:

- **Timer period** = one full clock cycle.
- **CCR1** = 50% of period → rising edge at start, falling edge mid‑period.
- **DMA trigger 1 (rising edge)**: TIMx\_CC1 event → mem→periph DMA to:

    - write BSRR (set/reset data bit),
    - optionally write CRL/CRH when you need to flip direction.
- **DMA trigger 2 (falling edge)**: TIMx update or another compare (e.g. CCR2 at 75% of period) → periph→mem DMA from IDR into buffer C.

### Data layout in RAM

Because DMA writes full 32‑bit registers, you precompute everything:

#### 1. Data output script (for BSRR)

For each **rising edge** you want to drive a bit:

- **One 32‑bit word** per edge:

    - For data = 1: `BSRR = (1 << DATA_PIN)`
    - For data = 0: `BSRR = (1 << (DATA_PIN + 16))` (reset bit)
- For edges where you don’t want to change the data pin, you can either:

    - repeat the same value, or
    - write a “no‑effect” word (e.g. 0) if you don’t need a change.

So for 48 cycles, you have an array of 48 words that DMA will stream into `GPIOx->BSRR`.

#### 2. Direction script (for CRL/CRH)

To change direction only at specific edges (e.g. at the 3rd rising edge):

- Precompute full **CRL/CRH** values for each point where direction changes.
- Use a **second DMA channel**:

    - Peripheral address: `&GPIOx->CRL` (or CRH).
    - Memory buffer: small array with the CRL value for “output mode” and “input mode”.
    - Number of transfers: equal to number of direction changes (e.g. 2 per frame: output at cycle 1, input at cycle 3).
    - Trigger: same timer event as BSRR DMA, but only enabled for those edges (you can do this by:

        - using a separate timer channel with a different compare schedule, or
        - enabling/disabling the DMA channel in software between phases if the pattern is fixed).

Because your protocol is simple (only one direction change per 3‑cycle frame), you can keep this small and deterministic.

#### 3. Input sampling buffer (for IDR → C)

For the **falling edge** of the 3rd cycle:

- Configure a DMA channel:

    - Peripheral address: `&GPIOx->IDR`.
    - Memory address: `&bufferC[i]` (or a small staging buffer).
    - Number of transfers: number of bits you expect to read (e.g. 1 per frame).
    - Trigger: timer event at the desired falling edge (e.g. CCR2).

You’ll get the full IDR word; you then mask the data bit in software after the transfer.

### Ping‑pong / double buffering

To let the CPU “prepare the next phase” while hardware is flushing the current one:

- Use **DMA double buffer** style (if you’re using HAL/LL, that’s often called “double buffer mode”; bare metal, you can emulate with:

    - **Normal mode DMA** with:

        - **Transfer complete interrupt** → swap buffer pointers and re‑arm DMA.
        - Or **half‑transfer interrupt** → treat first half as buffer 0, second half as buffer 1.
- Layout example:

    - `bsrr_script[2][MAX_EDGES]` → two banks (ping/pong).
    - `dir_script[2][MAX_DIR_CHANGES]`.
    - `input_buf[2][MAX_INPUT_BITS]`.

Flow:

1. CPU fills **ping** scripts (A/B/C for first 48 cycles).
2. Start timer + DMA with ping buffers.
3. While DMA is running, CPU fills **pong** scripts for the next 48 cycles.
4. On DMA transfer complete interrupt:

    - Switch DMA memory address to pong buffers.
    - Optionally adjust timer period if you want a different clock speed for the next phase.
5. Repeat.

Clock speed is then limited by:

- Timer max frequency,
- DMA bandwidth (number of transfers per edge),
- GPIO speed (up to 50 MHz on F1, but practical bit rates will be lower).

* * *

### Bare‑metal register sketch (very simplified)

This is not full code, just to show the structure:

```c
// 1. Enable clocks
RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;

// 2. Configure clock pin (e.g. PA1) as AF push-pull
GPIOA->CRL &= ~(0xF << (1 * 4));
GPIOA->CRL |=  (0xB << (1 * 4)); // MODE=11 (50MHz), CNF=10 (AF PP)

// 3. Configure data pin (e.g. PA0) initially as output
GPIOA->CRL &= ~(0xF << (0 * 4));
GPIOA->CRL |=  (0x3 << (0 * 4)); // MODE=11 (50MHz), CNF=00 (GP PP)

// 4. Timer setup for clock
TIM2->PSC = prescaler;      // sets base frequency
TIM2->ARR = period;         // one bit time
TIM2->CCR1 = period / 2;    // 50% duty
TIM2->CCMR1 = (6 << 4);     // PWM mode 1 on CH1
TIM2->CCER  = TIM_CCER_CC1E;
TIM2->DIER |= TIM_DIER_CC1DE | TIM_DIER_CC2DE; // DMA on CC1, CC2
TIM2->CR1  |= TIM_CR1_CEN;

// 5. DMA for BSRR (mem -> periph on CC1)
DMA1_ChannelX->CPAR = (uint32_t)&GPIOA->BSRR;
DMA1_ChannelX->CMAR = (uint32_t)bsrr_script_ping;
DMA1_ChannelX->CNDTR = num_edges;
DMA1_ChannelX->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;
DMA1_ChannelX->CCR |= DMA_CCR_EN;

// 6. DMA for IDR (periph -> mem on CC2)
DMA1_ChannelY->CPAR = (uint32_t)&GPIOA->IDR;
DMA1_ChannelY->CMAR = (uint32_t)input_buf_ping;
DMA1_ChannelY->CNDTR = num_samples;
DMA1_ChannelY->CCR = DMA_CCR_MINC | DMA_CCR_TCIE; // DIR=0 => P2M
DMA1_ChannelY->CCR |= DMA_CCR_EN;
```

Direction changes via CRL/CRH would be done with another DMA channel in a similar way, or—if you only have a couple of changes—via a very short ISR at the right timer event.

### Practical suggestion

Given your frame is only 3 cycles and up to 48 such cycles:

- If you want **maximum simplicity**, you can:

    - Use **timer + DMA** only for:

        - clock generation,
        - data output (BSRR).
    - Handle the **single direction change + input sample** with a **timer interrupt** at the 3rd rising/falling edge—this is still very light on CPU and timing is tight because it’s timer‑driven, not polling.

If you want it **fully autonomous**, extend to the CRL/CRH + IDR DMA scheme above.

* * *

> Considering a 72 MHz clock (Wait states apply), what is the max DMA transfers that saturates the internal bus?

Short answer: you won’t get a clean “N transfers per second” number, but you can bound it pretty well.

At 72 MHz on an F103:

- **AHB bus clock:** 72 MHz
- **DMA transfer = 1 read + 1 write on AHB** (e.g. SRAM→peripheral or peripheral→SRAM), so **2 bus beats per transfer**.
- **Theoretical upper bound:**

    - Max AHB beats: ~72 M beats/s
    - So max DMA transfers if the bus were 100 % dedicated to DMA:

<math xmlns="http://www.w3.org/1998/Math/MathML" display="block"><mrow><mfrac><mrow><mn>72</mn><mtext>&nbsp;M&nbsp;beats/s</mtext></mrow><mrow><mn>2</mn><mtext>&nbsp;beats/transfer</mtext></mrow></mfrac><mo>≈</mo><mn>36</mn><mtext>&nbsp;M&nbsp;transfers/s</mtext></mrow></math>

- For 32‑bit transfers, that’s ~36 M × 4 B ≈ **144 MB/s** of DMA payload (again, purely theoretical).

But:

- **Flash wait states** mainly hurt **CPU instruction fetch**, not DMA from SRAM.
- The **CPU, DMA, and other bus masters share the same AHB**, so you’ll never actually give DMA 100 % of the bus.
- Arbitration and peripheral side limits (APB bridges, FIFO, etc.) usually cap you well below that.

So: **around 36 M 32‑bit DMA transfers per second is the hard theoretical ceiling at 72 MHz**, and in any real design you should assume noticeably less if the CPU is doing anything non‑trivial at the same time.

* * *

> MSP430 chips are usually limted to 5MHz clock for the Spi-Bi-Wire bus, which is impossible to achieve without a dedicated logic. But I was thinking of 1 MHz. I don't see a way to selectively read input every 3rd cycle, so at the end I will have to read each cycle and discard useless cycles. Is there a hardware way to select each 3rd CC2 cycle?

You’re right: at 1 MHz bit clock you really don’t want to be “thinking” in software—this has to be timer‑driven.

Short version: **yes, there is a hardware way to get “every 3rd cycle”**, but not directly from a single CC event. You either:

- use an **advanced timer with repetition counter**, or
- use a **second timer as a /3 prescaler** for your sampling event.

Let me lay out both options.

### Option 1: Use TIM1 repetition counter (cleanest if available)

On STM32F103, **TIM1** is an advanced timer and has a **repetition counter (RCR)**. Key property:

- The **PWM output** (CC1, CC2, …) still toggles every period (every ARR).
- The **update event (UEV)** is generated only every **(RCR + 1)** periods.

So you can do:

1. Use **TIM1\_CH1** (or any channel) to generate your **1 MHz clock** (ARR set for 1 MHz, CCR for 50 % duty).
2. Set **TIM1-&gt;RCR = 2** → update event every 3 PWM periods.
3. Enable **DMA request on update event** (TIM1\_DIER |= UDE).
4. Use that **update‑triggered DMA** to:

    - read GPIOx\_IDR (sample data),
    - or do whatever “every 3rd cycle” action you need.

Result:

- Clock pin toggles at 1 MHz every cycle.
- Your DMA (or interrupt) only fires **every 3rd clock cycle**, purely in hardware.

This is almost exactly what you asked for.

### Option 2: Use a second timer as a /3 divider

If you don’t want or can’t use TIM1 for the clock:

1. **TIMx** (e.g. TIM2) generates the 1 MHz clock on CH1.
2. Configure **TIMy** (e.g. TIM3) in **external clock mode**:

    - TIMy’s clock source = TIMx’s CC event (or TRGO).
    - TIMy-&gt;PSC = 0, TIMy-&gt;ARR = 2 → TIMy overflows every 3 input pulses.
3. Enable **DMA request on TIMy update event**.

Now:

- TIMx runs at 1 MHz and drives the clock pin.
- TIMy counts those edges and overflows every 3rd one.
- TIMy’s update event is your “every 3rd cycle” trigger for DMA to sample the data pin.

### About “read every cycle and discard”

You *can* just let DMA read IDR on every falling edge and then only use every 3rd sample in software. At 1 MHz that’s still only 1 M samples/s, which is not crazy for DMA + a bit of post‑processing.

But if you want it **strictly hardware‑filtered**, the two approaches above give you a clean “every 3rd cycle” trigger with no software timing involved.

* * *

> Clock pin can be selected between PA5 (SPI1_CLK) or PA8 (TIM1_CH1). Can be selected according to operating mode: JTAG or SBW, mutually exclusive by hardware reconfiguration. PA5 was designed for JTAG use, where SPI1 and TIM1 cooperates to provide all signals. So, PA8 is currently reserved for my future SBW mode implementation. This means that TIM1 is intented for my JTAG/SBW bus timing control. PA9 selects direction of data pin. When PA9=0, levels seen on PA7 are on the data output, when PA9=1, data input levels can be read on PA6.

Nice, this pin map actually makes things cleaner—you’ve got a dedicated “bit engine” corner now:

- **PA8 → TIM1\_CH1 → SBW clock**
- **PA7 → data out (GPIO)**
- **PA6 → data in (GPIO)**
- **PA9 → direction select (GPIO, 0=out via PA7, 1=in via PA6)**

Let’s wire that into a hardware‑driven 3‑cycle SBW frame.

### 1. Use TIM1\_CH1 on PA8 as the 1 MHz clock

- **PA8 config:** AF push‑pull, 50 MHz.
- **TIM1 setup:**

    - `PSC` and `ARR` chosen so that:

<math xmlns="http://www.w3.org/1998/Math/MathML" display="block"><mrow><msub><mi>f</mi><mrow><mi>T</mi><mi>I</mi><mi>M</mi><mn>1</mn></mrow></msub><mo>=</mo><mfrac><mrow><mn>72</mn><mtext>&nbsp;MHz</mtext></mrow><mrow><mo stretchy="false">(</mo><mi>P</mi><mi>S</mi><mi>C</mi><mo>+</mo><mn>1</mn><mo stretchy="false">)</mo><mo>⋅</mo><mo stretchy="false">(</mo><mi>A</mi><mi>R</mi><mi>R</mi><mo>+</mo><mn>1</mn><mo stretchy="false">)</mo></mrow></mfrac><mo>=</mo><mn>1</mn><mtext>&nbsp;MHz</mtext></mrow></math>

- `CCR1 = ARR/2` → 50 % duty.
- CH1 in PWM mode 1, output enabled.
- This gives you a rock‑solid 1 MHz clock on PA8.

You then enable **DMA request on CC1** so that **every rising edge** can drive a DMA transfer.

### 2. Drive PA7 (data) and PA9 (direction) via DMA on each rising edge

You don’t actually need “every 3rd CC” hardware for direction—your 3‑cycle pattern is fixed, so you can encode it in the DMA script itself.

- **Data out (PA7):**

    - Use **GPIOA\_BSRR** as DMA peripheral.
    - One 32‑bit word per rising edge:

        - Bit = 1 → `1 << 7`
        - Bit = 0 → `1 << (7 + 16)`
- **Direction (PA9):**

    - Same trick, same BSRR, just different bit:

        - Output mode (PA9=0) → `1 << (9 + 16)`
        - Input mode (PA9=1) → `1 << 9`

Because BSRR is write‑only and orthogonal per bit, you can **OR** the two into a single word per edge:

- **Cycle 1 (A bit, force output):**

    - `BSRR_word[0] = (dataA0 ? (1<<7) : (1<<(7+16))) | (1<<(9+16))`
- **Cycle 2 (B bit, still output):**

    - `BSRR_word[1] = (dataB0 ? (1<<7) : (1<<(7+16)))` (no change to PA9)
- **Cycle 3 (no drive, switch to input):**

    - `BSRR_word[2] = (1<<9)` (PA9=1, PA7 can be left unchanged or driven low)

For 48 cycles (16 frames of 3 cycles), you just build a **48‑entry BSRR script** that encodes all A/B bits and the direction flips at the right edges. DMA (mem→periph) streams this into `GPIOA->BSRR` on every CC1 event.

So:

- **Clock:** PA8 from TIM1\_CH1.
- **Data out:** PA7 driven by BSRR via DMA on each rising edge.
- **Direction:** PA9 driven by BSRR via DMA only on those edges where you want to flip.

No CPU in the timing loop.

### 3. Sampling PA6 only every 3rd cycle (hardware way)

Here’s where your “every 3rd cycle” question matters. You’ve got two realistic options:

#### Option A – Simple: sample every cycle, discard 2/3 in software

- Use **TIM1 CC2** at the falling edge (e.g. `CCR2 = ARR/2`).
- Enable **DMA request on CC2**:

    - Peripheral: `GPIOA->IDR`
    - Memory: `input_samples[]`
- You get one IDR word per clock cycle; you only use indices 2, 5, 8, … (every 3rd sample) as your SBW input bits.

At 1 MHz that’s 1 M samples/s, which is well within DMA capability, and the CPU just post‑filters the buffer—no timing pressure.

#### Option B – Pure hardware “every 3rd cycle” using a second timer

If you really want **only every 3rd falling edge** to trigger DMA:

1. **TIM1** still generates the 1 MHz clock on PA8.
2. Configure **TIM1 TRGO** to emit one pulse per clock cycle:

    - e.g. TRGO = OC1REF.
3. Use **TIM2** as a /3 prescaler:

    - TIM2 in **external clock mode 1**, clocked from TIM1 TRGO.
    - `TIM2->PSC = 0`, `TIM2->ARR = 2` → overflow every 3 pulses.
4. Enable **DMA request on TIM2 update event**:

    - Peripheral: `GPIOA->IDR`
    - Memory: `bufferC[]`

Now:

- TIM1 runs the clock and your BSRR DMA script.
- TIM2 counts those cycles and overflows every 3rd one.
- On each TIM2 update, DMA grabs `GPIOA->IDR` once—exactly one sample per 3‑cycle SBW frame.

You can align the sampling instant with the **falling edge of the 3rd cycle** by choosing whether TIM2 counts rising or falling edges (via which TRGO you use) and by adding a small delay with TIM2’s own compare if needed, but for SBW at 1 MHz, sampling near the edge is usually fine.

### 4. Putting it together for your SBW mode

For SBW on PA8/PA7/PA6/PA9:

- **Clock (PA8):** TIM1\_CH1 PWM at 1 MHz.
- **Data out (PA7):** DMA→BSRR on TIM1\_CC1, script encodes A/B bits.
- **Direction (PA9):** same DMA→BSRR script, bits to flip PA9 at the right cycles.
- **Data in (PA6):**

    - Either:

        - **TIM1\_CC2 + DMA every cycle**, then keep every 3rd sample, or
        - **TIM1 TRGO → TIM2 external clock /3 → DMA on TIM2 update** for a true “every 3rd cycle” hardware trigger.

Your CPU then just:

- Prepares the next BSRR script (A/B bits + PA9 flips) and input buffer slot (C),
- Arms DMA + timers,
- Waits for “phase done” interrupt,
- Ping‑pongs to the next script.

