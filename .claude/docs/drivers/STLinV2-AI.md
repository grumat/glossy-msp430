Your goal is to **offload JTAG bit-banging to hardware** (DMA + Timer) for **stable, high-speed transfers** with configurable bit rates. This is a classic **double-buffering + DMA + Timer** approach, which is ideal for precise, jitter-free timing. Here’s how to implement it on the **STM32F103**:

* * *

## **1. Hardware Setup**

### **Pins**

- **TCK (PA5)**: Timer output (for clock generation).
- **TDO (PA6)**: GPIO output (data out, driven by DMA).
- **TDI (PA7)**: GPIO input (data in, sampled by DMA).
- **TMS (PB12)**: GPIO output (state machine control, driven by DMA).
- **TRST (PB1)**, **NRST (PB0)**: GPIO outputs (manual control, not part of DMA transfers).

### **Peripherals**

- **Timer**: Use **TIM2** (32-bit, easy to configure, supports DMA).
- **DMA**: Use **DMA1 Channel 2** (for TIM2 UP event) and **DMA1 Channel 5** (for TIM2 CC1/CC2).
- **GPIO**: Configure PA5 (TCK), PA6 (TDO), PA7 (TDI), PB12 (TMS) as push-pull outputs (except TDI, which is input).

* * *

## **2. Timer Configuration (TIM2)**

- **Clock Source**: Use the internal clock (e.g., 72 MHz).
- **Mode**: PWM mode (for TCK generation).
- **Prescaler**: Set to achieve desired JTAG clock rate (e.g., for 1 MHz TCK, prescaler = 72 - 1).
- **Auto-Reload Register (ARR)**: Set to `1` (toggle TCK every timer update).
- **DMA Trigger**: Enable **Update Event (UEV)** to trigger DMA transfers.

```cpp
// Example: 1 MHz TCK (72 MHz / 72 = 1 MHz)
TIM2->PSC = 72 - 1;  // Prescaler
TIM2->ARR = 1;       // Auto-reload
TIM2->CCMR1 = 0x60;  // PWM mode 1 for CC1 (PA5/TCK)
TIM2->CCER = 0x01;   // Enable CC1 output
TIM2->DIER = 0x01;   // Enable Update DMA request
TIM2->CR1 = 0x81;    // Enable counter, auto-reload preload
```

* * *

## **3. DMA Configuration**

### **DMA for TDO (Output)**

- **DMA1 Channel 5**: Configured to transfer data from a buffer to **GPIOA-&gt;ODR** (for PA6/TDO).
- **Mode**: Circular, memory-to-peripheral, 8/16/32-bit transfers.
- **Trigger**: TIM2 Update Event.

### **DMA for TDI (Input)**

- **DMA1 Channel 2**: Configured to sample **GPIOA-&gt;IDR** (for PA7/TDI) into a buffer.
- **Mode**: Circular, peripheral-to-memory, 8/16/32-bit transfers.
- **Trigger**: TIM2 Update Event.

```cpp
// DMA1 Channel 5: TDO (output)
DMA1_Channel5->CPAR = (uint32_t)&(GPIOA->ODR);  // Peripheral address (TDO)
DMA1_Channel5->CMAR = (uint32_t)tx_buffer;      // Memory address
DMA1_Channel5->CNDTR = buffer_size;             // Buffer size
DMA1_Channel5->CCR = 0x58B2;                    // Mem2Per, circular, 8/16/32-bit, TIM2_UP

// DMA1 Channel 2: TDI (input)
DMA1_Channel2->CPAR = (uint32_t)&(GPIOA->IDR);  // Peripheral address (TDI)
DMA1_Channel2->CMAR = (uint32_t)rx_buffer;      // Memory address
DMA1_Channel2->CNDTR = buffer_size;             // Buffer size
DMA1_Channel2->CCR = 0x5852;                    // Per2Mem, circular, 8/16/32-bit, TIM2_UP
```

* * *

## **4. Double Buffering**

- Use **two buffers**for TX and RX:
    - While DMA transmits one buffer, the MCU prepares the next.
    - Swap buffers using a **ping-pong** approach (e.g., toggle `CMAR` when DMA completes a half-transfer).
- **Interrupt**: Enable **DMA Half-Transfer** and **Transfer Complete** interrupts to swap buffers.

```cpp
// Example buffer swap in DMA1_Channel5 IRQ
if (DMA1->ISR & DMA_ISR_HTIF5) {
    DMA1_Channel5->CMAR = (uint32_t)(active_tx_buffer ? tx_buffer2 : tx_buffer1);
    active_tx_buffer ^= 1;
}
```


* * *

## **5. TMS Control**

- **TMS (PB12)**is part of the JTAG state machine. Include it in the TX buffer as a bit flag:
    - Pack **TDO + TMS** into a single byte/word (e.g., bit 0 = TDO, bit 1 = TMS).
    - Use a **bitmask** to extract TMS and TDO during DMA transfer.

```cpp
// Example: Pack TDO (bit 0) and TMS (bit 1) into a byte
tx_buffer[i] = (tdo_bit << 0) | (tms_bit << 1);
```

## **6. Flow Control**

- **Speed Options**: Adjust `TIM2->PSC` to change TCK frequency (e.g., 72-1 for 1 MHz, 36-1 for 2 MHz).
- **Start/Stop**: Enable/disable TIM2 and DMA to control transfers.

```cpp
// Start transfer
TIM2->CR1 |= 0x01;          // Enable timer
DMA1_Channel5->CCR |= 0x01; // Enable DMA TX
DMA1_Channel2->CCR |= 0x01; // Enable DMA RX

// Stop transfer
TIM2->CR1 &= ~0x01;         // Disable timer
DMA1_Channel5->CCR &= ~0x01;
DMA1_Channel2->CCR &= ~0x01;
```

## **7. JTAG State Machine**

- Precompute the **TMS sequence** for JTAG state transitions (e.g., IDLE ? SHIFT-DR ? EXIT1-DR).
- Include TMS bits in the TX buffer alongside TDO data.

* * *

## **8. Example Buffer Layout**

For an **8-bit transfer**:

- `tx_buffer[0] = (TDO_bit0 << 0) | (TMS_bit0 << 1);`
- `tx_buffer[1] = (TDO_bit1 << 0) | (TMS_bit1 << 1);`
- ...
- `tx_buffer[7] = (TDO_bit7 << 0) | (TMS_bit7 << 1);`

* * *

## **9. Logic Analyzer Verification**

- Use a logic analyzer to verify:
    - **TCK stability** (should be rock-solid).
    - **TDO/TDI timing** (aligned with TCK edges).
    - **TMS transitions** (correct state machine flow).

* * *

## **10. Code Outline**

```cpp
// Initialize GPIO, Timer, DMA
void JTAG_Init(void) {
    // GPIO setup (PA5, PA6, PA7, PB12)
    // Timer setup (TIM2)
    // DMA setup (DMA1 Ch2/Ch5)
}

// Start transfer
void JTAG_Start(uint8_t* tx_data, uint8_t* rx_data, uint16_t length) {
    // Set buffer pointers/lengths
    // Enable DMA/TIM2
}

// Stop transfer
void JTAG_Stop(void) {
    // Disable DMA/TIM2
}

// Buffer swap ISR
void DMA1_Channel5_IRQHandler(void) {
    // Swap buffers on half/full transfer
}
```


## **Advantages**

- **Precise timing**: No jitter (hardware-driven).
- **Configurable speed**: Adjust `TIM2->PSC`.
- **Efficient**: MCU is free to prepare the next buffer.

* * *

# **Transfer Functions**

Here’s a **practical implementation** for your STM32F103-based JTAG interface, supporting **8/16/20/32-bit payloads** and a **busy-check function** for the Timer/DMA. This example assumes you’re using **TIM2**, **DMA1 Channels 2/5**, and the GPIO pins you specified.

* * *

## **1. Hardware Definitions**

```cpp
// GPIO Definitions
#define TCK_PIN   GPIO_Pin_5   // PA5
#define TDO_PIN   GPIO_Pin_6   // PA6
#define TDI_PIN   GPIO_Pin_7   // PA7
#define TMS_PIN   GPIO_Pin_12  // PB12
#define TRST_PIN  GPIO_Pin_1   // PB1
#define NRST_PIN  GPIO_Pin_0   // PB0

// Buffer and DMA Definitions
#define MAX_BUFFER_SIZE  64  // Adjust as needed
uint8_t tx_buffer1[MAX_BUFFER_SIZE];
uint8_t tx_buffer2[MAX_BUFFER_SIZE];
uint8_t rx_buffer1[MAX_BUFFER_SIZE];
uint8_t rx_buffer2[MAX_BUFFER_SIZE];

volatile uint8_t active_tx_buffer = 0;
volatile uint8_t active_rx_buffer = 0;
```

## **2. Initialization**

### **GPIO Setup**

```cpp
void JTAG_GPIO_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    // PA5 (TCK): Alternate Function Push-Pull (Timer2 CH1)
    GPIOA->CRL &= ~(0xF << (5 * 4));
    GPIOA->CRL |= (0xB << (5 * 4));  // AF Push-Pull, 50MHz

    // PA6 (TDO): Push-Pull Output
    GPIOA->CRL &= ~(0xF << (6 * 4));
    GPIOA->CRL |= (0x3 << (6 * 4));  // Push-Pull, 50MHz

    // PA7 (TDI): Floating Input
    GPIOA->CRL &= ~(0xF << (7 * 4));
    GPIOA->CRL |= (0x4 << (7 * 4));  // Floating Input

    // PB12 (TMS): Push-Pull Output
    GPIOB->CRH &= ~(0xF << (12 - 8) * 4);
    GPIOB->CRH |= (0x3 << (12 - 8) * 4);

    // PB1 (TRST), PB0 (NRST): Push-Pull Output
    GPIOB->CRL &= ~(0xFF << 0);
    GPIOB->CRL |= (0x33 << 0);
}
```

* * * 

### **Timer Setup (TIM2)**

```cpp
void JTAG_Timer_Init(uint16_t prescaler) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->PSC = prescaler - 1;  // e.g., 72-1 for 1MHz TCK
    TIM2->ARR = 1;              // Toggle TCK every 1us
    TIM2->CCMR1 = 0x60;         // PWM Mode 1 for CH1 (PA5/TCK)
    TIM2->CCER = 0x01;          // Enable CH1 output
    TIM2->DIER = 0x01;          // Enable Update DMA request
    TIM2->CR1 = 0x00;           // Initially disabled
}
```

* * * 

### **DMA Setup**

```cpp
void JTAG_DMA_Init(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    // DMA1 Channel 5: TX (TDO + TMS)
    DMA1_Channel5->CPAR = (uint32_t)&(GPIOA->ODR);
    DMA1_Channel5->CMAR = (uint32_t)tx_buffer1;
    DMA1_Channel5->CNDTR = MAX_BUFFER_SIZE;
    DMA1_Channel5->CCR = 0x58B2;  // Mem2Per, circular, 8-bit, TIM2_UP

    // DMA1 Channel 2: RX (TDI)
    DMA1_Channel2->CPAR = (uint32_t)&(GPIOA->IDR);
    DMA1_Channel2->CMAR = (uint32_t)rx_buffer1;
    DMA1_Channel2->CNDTR = MAX_BUFFER_SIZE;
    DMA1_Channel2->CCR = 0x5852;  // Per2Mem, circular, 8-bit, TIM2_UP

    // Enable DMA interrupts for buffer swapping
    DMA1_Channel5->CCR |= 0x04;  // Enable Half-Transfer interrupt
    DMA1_Channel2->CCR |= 0x04;
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}
```

* * * 

## **3. Buffer Packing for 8/16/20/32-Bit Payloads**

### **Packing TDO + TMS**

For each bit, pack **TDO** (bit 0) and **TMS** (bit 1) into a byte:

```cpp
void JTAG_PackBuffer(uint8_t* buffer, uint32_t tdo_data, uint32_t tms_data, uint8_t bit_length) {
    for (uint8_t i = 0; i < bit_length; i++) {
        buffer[i] = ((tdo_data >> i) & 0x01) | (((tms_data >> i) & 0x01) << 1);
    }
}
```

### **Example: 16-Bit Transfer**

```cpp
uint16_t tdo_data = 0xA5A5;
uint16_t tms_data = 0x0003;  // Example: TMS transitions for state machine
JTAG_PackBuffer(tx_buffer1, tdo_data, tms_data, 16);
```

* * *

**4. Busy-Check Function**

```cpp
uint8_t JTAG_IsBusy(void) {
    // Check if TIM2 is enabled (transfer in progress)
    if (TIM2->CR1 & 0x01) {
        return 1;  // Busy
    }
    // Check if DMA is enabled
    if (DMA1_Channel5->CCR & 0x01) {
        return 1;  // Busy
    }
    return 0;  // Not busy
}
```

* * *

## **5. Start/Stop Transfers**

### **Start Transfer**

```cpp
void JTAG_StartTransfer(uint8_t bit_length) {
    if (JTAG_IsBusy()) return;  // Skip if busy

    // Set buffer length (for 8/16/20/32 bits)
    DMA1_Channel5->CNDTR = bit_length;
    DMA1_Channel2->CNDTR = bit_length;

    // Enable DMA and Timer
    DMA1_Channel5->CCR |= 0x01;
    DMA1_Channel2->CCR |= 0x01;
    TIM2->CR1 |= 0x01;
}
```

### **Stop Transfer**

```cpp
void JTAG_StopTransfer(void) {
    TIM2->CR1 &= ~0x01;        // Disable timer
    DMA1_Channel5->CCR &= ~0x01;
    DMA1_Channel2->CCR &= ~0x01;
}
```

## **6. DMA Interrupt Handlers**

### **DMA1 Channel 5 (TX)**

```cpp
void DMA1_Channel5_IRQHandler(void) {
    if (DMA1->ISR & DMA_ISR_HTIF5) {  // Half-transfer
        DMA1_Channel5->CMAR = (uint32_t)(active_tx_buffer ? tx_buffer2 : tx_buffer1);
        active_tx_buffer ^= 1;
    }
    DMA1->IFCR = DMA_IFCR_CHTIF5;  // Clear interrupt flag
}
```

### **DMA1 Channel 2 (RX)**

```cpp
void DMA1_Channel2_IRQHandler(void) {
    if (DMA1->ISR & DMA_ISR_HTIF2) {  // Half-transfer
        DMA1_Channel2->CMAR = (uint32_t)(active_rx_buffer ? rx_buffer2 : rx_buffer1);
        active_rx_buffer ^= 1;
    }
    DMA1->IFCR = DMA_IFCR_CHTIF2;  // Clear interrupt flag
}
```

## **7. Example Usage**

```cpp
// Configure for 16-bit transfer at 1MHz TCK
JTAG_GPIO_Init();
JTAG_Timer_Init(72);  // 72MHz / 72 = 1MHz TCK
JTAG_DMA_Init();

// Pack 16-bit data
uint16_t tdo_data = 0xA5A5;
uint16_t tms_data = 0x0003;
JTAG_PackBuffer(tx_buffer1, tdo_data, tms_data, 16);

// Start transfer
JTAG_StartTransfer(16);

// Wait for completion (or poll JTAG_IsBusy())
while (JTAG_IsBusy());

// Stop transfer
JTAG_StopTransfer();
```

* * *

* * *

## **1. Problem with Double Buffering for JTAG**

- Double buffering assumes **fixed-size, back-to-back transfers**.
- JTAG often requires **single, variable-length transfers** (e.g., an 8-bit IR scan followed by a 32-bit DR scan).
- **Solution**: Use **single-buffer DMA transfers** with **interrupt-driven completion** and **manual reconfiguration** for each payload.

* * *

## **2. Revised Approach**

### **Key Changes**

1. **Disable circular mode**: Use **single-shot DMA transfers** for each payload.
2. **Use DMA Transfer Complete (TCIF) interrupt**: Signal when the current payload is done.
3. **Reconfigure DMA for each transfer**: Update buffer address and size before starting.
4. **Manual buffer management**: No automatic switching; the CPU prepares the next payload after the current one finishes.

* * *

## **3. Hardware Setup (Same as Before)**

- **Timer (TIM2)**: Generates TCK.
- **DMA (DMA1 Ch5 for TX, DMA1 Ch2 for RX)**: Transfers TDO/TMS and samples TDI.
- **GPIO**: PA5 (TCK), PA6 (TDO), PA7 (TDI), PB12 (TMS).

* * *

## **4. DMA Configuration (Non-Circular)**

```cpp
void JTAG_DMA_Init(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    // DMA1 Channel 5: TX (TDO + TMS)
    DMA1_Channel5->CPAR = (uint32_t)&(GPIOA->ODR);
    DMA1_Channel5->CMAR = (uint32_t)tx_buffer;  // Single buffer
    DMA1_Channel5->CNDTR = 0;  // Set dynamically per transfer
    DMA1_Channel5->CCR = 0x58A2;  // Mem2Per, 8-bit, TIM2_UP, TCIE (Transfer Complete Interrupt)

    // DMA1 Channel 2: RX (TDI)
    DMA1_Channel2->CPAR = (uint32_t)&(GPIOA->IDR);
    DMA1_Channel2->CMAR = (uint32_t)rx_buffer;  // Single buffer
    DMA1_Channel2->CNDTR = 0;  // Set dynamically per transfer
    DMA1_Channel2->CCR = 0x5842;  // Per2Mem, 8-bit, TIM2_UP, TCIE

    // Enable DMA interrupts
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}
```

## **5. Start a Single Transfer**

```cpp
void JTAG_StartTransfer(uint8_t* tx_data, uint8_t* rx_data, uint16_t bit_length) {
    // Wait if busy
    while (JTAG_IsBusy());

    // Pack data into tx_buffer (TDO + TMS)
    JTAG_PackBuffer(tx_data, rx_data, bit_length);

    // Configure DMA for this transfer
    DMA1_Channel5->CMAR = (uint32_t)tx_buffer;
    DMA1_Channel5->CNDTR = bit_length;
    DMA1_Channel2->CMAR = (uint32_t)rx_buffer;
    DMA1_Channel2->CNDTR = bit_length;

    // Enable DMA and Timer
    DMA1_Channel5->CCR |= 0x01;
    DMA1_Channel2->CCR |= 0x01;
    TIM2->CR1 |= 0x01;
}
```

* * *

## **6. DMA Transfer Complete Interrupt**

```cpp
void DMA1_Channel5_IRQHandler(void) {
    if (DMA1->ISR & DMA_ISR_TCIF5) {
        // Transfer complete: disable DMA and Timer
        DMA1_Channel5->CCR &= ~0x01;
        DMA1_Channel2->CCR &= ~0x01;
        TIM2->CR1 &= ~0x01;

        // Clear interrupt flag
        DMA1->IFCR = DMA_IFCR_CTCIF5 | DMA_IFCR_CTCIF2;

        // Optional: Call a callback or set a flag
        jtag_transfer_complete = 1;
    }
}
```

* * *

## **7. Busy-Check Function**

```cpp
uint8_t JTAG_IsBusy(void) {
    // Check if Timer or DMA is running
    return (TIM2->CR1 & 0x01) || (DMA1_Channel5->CCR & 0x01);
}
```

* * *

## **8. Packing Data for Variable-Length Transfers**

```cpp
void JTAG_PackBuffer(uint8_t* buffer, uint32_t tdo_data, uint32_t tms_data, uint8_t bit_length) {
    for (uint8_t i = 0; i < bit_length; i++) {
        buffer[i] = ((tdo_data >> i) & 0x01) | (((tms_data >> i) & 0x01) << 1);
    }
}
```

* * *

## **9. Example Usage**

```cpp
// 8-bit IR scan
uint8_t ir_tdo = 0x01;  // Example: Shift IR with 0x01
uint8_t ir_tms = 0x03;  // TMS sequence for IR scan
JTAG_StartTransfer(ir_tdo, ir_tms, 8);

// Wait for completion (or use a callback)
while (JTAG_IsBusy());

// 32-bit DR scan
uint32_t dr_tdo = 0xA5A5A5A5;
uint32_t dr_tms = 0x00000003;
JTAG_StartTransfer(dr_tdo, dr_tms, 32);
```

## **10. Advantages of This Approach**

1. **No Mixed Payloads**: Each transfer is self-contained.
2. **Variable-Length Support**: Works for 8/16/20/32-bit payloads.
3. **No Double Buffering Overhead**: Simpler logic for JTAG’s dynamic nature.
4. **Precise Timing**: Timer and DMA ensure stable TCK and data alignment.

* * *

## **11. Handling 20-Bit Transfers**

For non-byte-aligned transfers (e.g., 20 bits):

- Pack the first 16 bits into 2 bytes, and the remaining 4 bits into a 3rd byte (pad with 0s).
- Adjust the loop in `JTAG_PackBuffer` to handle the extra bits.

