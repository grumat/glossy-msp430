﻿# EEM Documentation

This documentation was taking from the TI SLAU2008Q datasheet and properly extended 
to improve contents.


## (1) Embedded Emulation Module (EEM) Introduction

Every MSP430 microcontroller implements an EEM. It is accessed and 
controlled through either 4-wire JTAG mode or Spy-Bi-Wire mode. Each 
implementation is device-dependent and is described in Section 3, the EEM 
Configurations section, and the device-specific data sheet.

In general, the following features are available:
- Nonintrusive code execution with real-time breakpoint control
- Single-step, step-into, and step-over functionality
- Full support of all low-power modes
- Support for all system frequencies, for all clock sources
- Up to eight (device-dependent) hardware triggers or breakpoints on 
memory address bus (MAB) or memory data bus (MDB)
- Up to two (device-dependent) hardware triggers or breakpoints on CPU 
register write accesses
- MAB, MDB, and CPU register access triggers can be combined to form up 
to ten (device-dependent) complex triggers or breakpoints
- Up to two (device-dependent) cycle counters
- Trigger sequencing (device-dependent)
- Storage of internal bus and control signals using an integrated trace 
buffer (device-dependent)
- Clock control for timers, communication peripherals, and other modules 
on a global device level or on a per-module basis during an emulation 
stop.

Figure 1 shows a simplified block diagram of the largest 
currently-available EEM implementation.

![Large Implementation of EEM](images/eem-fs8.png)  

For more details on how the features of the EEM can be used together with 
the IAR Embedded Workbench™ debugger or with Code Composer Studio (CCS), 
see Advanced Debugging Using the Enhanced Emulation Module 
([SLAA393](www.ti.com/lit/pdf/SLAA393)) at www.msp430.com. Most other 
debuggers supporting the MSP430 devices have the same or a similar 
feature set. For details, see the user's guide of the applicable debugger.


## (2) EEM Building Blocks


### Triggers

The event control in the EEM of the MSP430 system consists of triggers, 
which are internal signals indicating that a certain event has happened. 
These triggers may be used as simple breakpoints, but it is also possible 
to combine two or more triggers to allow detection of complex events and 
cause various reactions other than stopping the CPU.

In general, the triggers can be used to control the following functional 
blocks of the EEM:
- Breakpoints (CPU stop)
- State storage
- Sequencer
- Cycle counter

There are two different types of triggers – the memory trigger and the 
CPU register write trigger.

Each memory trigger block can be independently selected to compare either 
the MAB or the MDB with a given value. Depending on the implemented EEM, 
the comparison can be =, ≠, ≥, or ≤. The comparison can also be limited 
to certain bits with the use of a mask. The mask is either bit-wise or 
byte-wise, depending upon the device. In addition to selecting the bus 
and the comparison, the condition under which the trigger is active can 
be selected. The conditions include read access, write access, DMA 
access, and instruction fetch.

Each CPU register write trigger block can be independently selected to 
compare what is written into a selected register with a given value. The 
observed register can be selected for each trigger independently. The 
comparison can be =, ≠, ≥, or ≤. The comparison can also be limited to 
certain bits with the use of a bit mask.

Both types of triggers can be combined to form more complex triggers. For 
example, a complex trigger can signal when a particular value is written 
into a user-specified address.


### Trigger Sequencer

The trigger sequencer allows the definition of a certain sequence of 
trigger signals before an event is accepted for a break or state storage 
event. Within the trigger sequencer, it is possible to use the following 
features:

- Four states (State 0 to State 3)
- Two transitions per state to any other state
- Reset trigger that resets the sequencer to State 0.

The trigger sequencer always starts at State 0 and must execute to State 
3 to generate an action. If State 1 or State 2 are not required, they can 
be bypassed.


### State Storage (Internal Trace Buffer)

The state storage function uses a built-in buffer to store MAB, MDB, and 
CPU control signal information (that is, read, write, or instruction 
fetch) in a nonintrusive manner. The built-in buffer can hold up to eight 
entries. The flexible configuration allows the user to record the 
information of interest very efficiently.


### Cycle Counter

The cycle counter provides one or two 40-bit counters to measure the 
cycles used by the CPU to execute certain tasks. On some devices, the 
cycle counter operation can be controlled using triggers. This allows, 
for example, conditional profiling, such as profiling a specific section 
of code.


### Clock Control

The EEM provides device-dependent flexible clock control. This is useful 
in applications where a running clock is needed for peripherals after the 
CPU is stopped (for example, to allow a UART module to complete its 
transfer of a character or to allow a timer to continue generating a PWM 
signal).

The clock control is flexible and supports both modules that need a 
running clock and modules that must be stopped when the CPU is stopped 
due to a breakpoint.


## (3) EEM Configurations

Table 1 gives an overview of the EEM configurations. The implemented 
configuration is device-dependent, and details can be found in the 
device-specific data sheet and these documents:

- Advanced Debugging Using the Enhanced Emulation Module (EEM) With CCS 
([SLAA393](www.ti.com/lit/pdf/SLAA393))
- IAR Embedded Workbench Version 3+ for MSP430 User's Guide 
([SLAU138](www.ti.com/lit/pdf/SLAU138))
- Code Composer Studio for MSP430 User’s Guide 
([SLAU157](www.ti.com/lit/pdf/SLAU157))

**Table 1 - EEM Configurations**

| Feature                  |  XS  |   S  |   M  |   L  |
|:-------------------------|:----:|:----:|:----:|:----:|
| Memory bus triggers  | 2 (=, ≠) |   3  |   5  |   8  |
| Memory bus trigger mask for | 1) Low byte<br/>2) High byte<br/>3) Four upper addr bits | 1) Low byte<br/>2) High byte<br/>3) Four upper addr bits | 1) Low byte<br/>2) High byte<br/>3) Four upper addr bits | All 16 or 20 bits |
| CPU register write triggers | 0 |   1  |   1  |   2  |
| Combination triggers     |  2   |   4  |   6  |  10  |
| Sequencer                |  No  |  No  |  Yes |  Yes |
| State storage            |  No  |  No  |  No  |  Yes |
| Cycle counter            |  0   |   1  |   1  | 2<br/>(including triggered start or stop) |

In general, the following features can be found on any device:
- At least two MAB or MDB triggers supporting:
  - Distinction between CPU, DMA, read, and write accesses
  - =, ≠, ≥, or ≤ comparison (in XS, only =, ≠)
- At least two trigger combination registers
- Hardware breakpoints using the CPU stop reaction
- At least one 40-bit cycle counter
- Enhanced clock control with individual control of module clocks


## (4) Emulation Hardware Registers

A set of hardware registers are provided to control emulation. These are 
accessible through the JTAG/SBW emulation port.

The typical for to address these hardware registers is through the 
`IR_EMEX_DATA_EXCHANGE` or `IR_EMEX_DATA_EXCHANGE32` instruction byte,
which should be written to the JTAG instruction register.
When sending one of this instruction byte, EEM is activated in either 
16-bit or 32-bit mode.

Transfer are then performed using the JTAG DR frame using a fixed size, 
16 or 32-bit, depending on the command that started the mode. Devices 
that implements a 16-bit CPU should be accessed using `IR_EMEX_DATA_EXCHANGE`, 
while `IR_EMEX_DATA_EXCHANGE32` shall be used for devices implementing a 
20-bit address bus (CPUX and CPUXv2).

After this mode is activated, DR (Data register) can be repeteadly 
acessed in cycles of two transfers each. So each cycle is a DR pair. The 
first DR is the address of the EEM register and the second DR is the data.

After each pair one can start the next in a bulk until all transfers are 
performed. The mode will end as soon as a new IR is started.

The advantage of this algorithm is that all registers can be adjusted in 
a bulk.

<div hidden>

```
@startuml emex_exchange_flow

(*) --> "IR(IR_EMEX_DATA_EXCHANGE32)"
--> "DR32(Address)" as addr
--> "DR32(Data)"
--> if "more transfers?"
  ----> addr
else
  --> [n] "IR(<any>)"
  --> (*)
endif

@enduml
```

</div>

![EEM Data Exchange](images/emex_exchange_flow.svg)


### Read or Write Operation

When accessing a register on the EEM, to specify a read operation it is 
necessary to add 1 to the register base address. In other words, each 
register repeats twice in the address space. even addresses are used to 
perform a write operation, while odd values retrieves register contents.


### GENCTRL: General Debug Control Register (`0x82`)

> The `MX_GENCNTRL` is an alias for `GENCNTRL`.

This is the general debug control register for the EEM module:

|  15 |  14 |  13 |  12 |  11 |  10 |  9  |  8  |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|  x  |  x  |  x  |  x  |  x  |  x  |  x  |  x  |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
| STP | RST |  x  | TRG | FEA | CLK | CST |  EN |

Thease are the values:
- **EEM_EN (`0x0001`)**: Enables the EEM module (`GCC_EXTENDED` only?)
- **CLEAR_STOP (`0x0002`)**: This signal is pulse once during the halt of 
the MCU  (`GCC_EXTENDED` only?)
- **EMU_CLK_EN (`0x0004`)**: (`GCC_EXTENDED` only?)
- **EMU_FEAT_EN (`0x0008`)**: TODO
- **DEB_TRIG_LATCH (`0x0010`)**: TODO
- **EEM_RST (`0x0040`)**: TODO: Resets the EEM.
- **E_STOPPED (`0x0080`)**: TODO: Indicates the CPU has stopped

**TODO**: This has also effect on `IR_EMEX_WRITE_CONTROL`. (A mirror?) 
(CPUXv2 only?)


### MODCLKCTRL0: EEM Module Clock Control Register 0 (`0x8A`)

|   15  |   14  |   13  |   12  |   11  |   10  |   9   |   8   |
|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|
|   x   |  MCLK | SMCLK |  ACLK |  ADC  | FLASH |  SER1 |  SER0 |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
| TPORT |  TCNT |  LCDF | BASTM |  TIMB |  TIMA |  WDT  |   x   |


### Trigger blocks TB0 to TB9

Trigger blocks are used as inputs to the **Combination Trigger AND Matrix**, 
sequentially organized in a series of four registers each.

The following table lists addresses for each register.

| Trigger |   MBTRIGxVAL  |   MBTRIGxCTL  |   MBTRIGxMSK  |   MBTRIGxCMB  |
|:-------:|:-------------:|:-------------:|:-------------:|:-------------:|
|   TB0   | `0x00`/`0x01` | `0x02`/`0x03` | `0x04`/`0x05` | `0x06`/`0x07` |
|   TB1   | `0x08`/`0x09` | `0x0A`/`0x0B` | `0x0C`/`0x0D` | `0x0E`/`0x0F` |
|   TB2   | `0x10`/`0x11` | `0x12`/`0x13` | `0x14`/`0x15` | `0x16`/`0x17` |
|   TB3   | `0x18`/`0x19` | `0x1A`/`0x1B` | `0x1C`/`0x1D` | `0x1E`/`0x1F` |
|   TB4   | `0x20`/`0x21` | `0x22`/`0x23` | `0x24`/`0x25` | `0x26`/`0x27` |
|   TB5   | `0x28`/`0x29` | `0x2A`/`0x2B` | `0x2C`/`0x2D` | `0x2E`/`0x2F` |
|   TB6   | `0x30`/`0x31` | `0x32`/`0x33` | `0x34`/`0x35` | `0x36`/`0x37` |
|   TB7   | `0x38`/`0x39` | `0x3A`/`0x3B` | `0x3C`/`0x3D` | `0x3E`/`0x3F` |
|   TB8   | `0x40`/`0x41` | `0x42`/`0x43` | `0x44`/`0x45` | `0x46`/`0x47` |
|   TB9   | `0x48`/`0x49` | `0x4A`/`0x4B` | `0x4C`/`0x4D` | `0x4E`/`0x4F` |

> As already mentioned before the even addresses access the register in
> write mode, while odd values are used to retrieve contents.

Note that **TB8** and **TB9** refers to the CPU register.

#### MBTRIGxVAL Register (`TB`*n*+`0x0000`)

This is the reference value to be used for comparison. In case of a 
breakpoint here you would put the address where the CPU has to stop.

A trigger obtain a source value based on the configuration of the 
`MBTRIGxCTL` register. The value obtained is compared against the 
value programmed into `MBTRIGxVAL`.


#### MBTRIGxCTL register (`TB`*n*+`0x0002`)

Each memory trigger block can be independently selected to compare either 
the **MAB** or the **MDB** with a given value.  
Depending on the implemented EEM, the comparison can be `=`, `≠`, `≥`, or 
`≤`.  
The comparison can also be limited to certain bits with the use of a 
mask. The mask is either bit-wise or byte-wise, depending upon the 
device.

In addition to selecting the bus and the comparison, the **condition** 
under which the trigger is active can be selected. The conditions include 
**read access**, **write access**, **DMA access**, and **instruction 
fetch**.

|  15 |  14 |  13 |  12 |  11 |  10 |  9  |  8  |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|  x  |  x  |  x  |  x  |  x  |  x  |  x  |  x  |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
|  x  | TRG | TRG | CMP | CMP | TRG | TRG | MDB |

- **MDB (b0)**: This bit can be set to one of the following values:
  - **MAB (0x0000)**: Indicates the source to obtain a value ids the 
  address bus.
  - **MDB (0x0001)**: INdicates the source to obtain a value is the
  data bus.
- **TRG (b6-b5,b2-b1)**: These bits programs the event type used as 
source of the information:
  - **TRIG_0 (`0x0000`)**: The event that activates the trigger is the 
  **Instruction Fetch**, that is the instant where the CPU loads an 
  *op-code*. This is typically the use case for a breakpoint, in case,
  if the **MAB** address equal to the `MBTRIGxVAL`.  
  Depending on the MDB bit you can compare the `MBTRIGxVAL` against a 
  specific *op-code*.
  - **TRIG_1 (`0x0002`)**: TI documentation states *Instruction Fetch Hold*, 
  which is quite vague.
  - **TRIG_2 (`0x0004`)**: An access to the MAB or MDB which is not an 
  *Instruction Fetch*.
  - **TRIG_3 (`0x0006`)**: Don't care/Undefined.
  - **TRIG_4 (`0x0020`)**: An access to the MAB or MDB where *no 
  Instruction Fetch* is happening but in a *Read* operation.
  - **TRIG_5 (`0x0022`)**: An access to the MAB or MDB where *no 
  Instruction Fetch* is done but for a *Write* cycle.
  - **TRIG_6 (`0x0024`)**: This event indicates that a *Read* has 
  happened.
  - **TRIG_7 (`0x0026`)**: This event indicates that a *Write* has 
  happened.
  - **TRIG_8 (`0x0040`)**: This event is triggered when *no Instruction 
  Fetch* and also *no DMA access* happens.
  - **TRIG_9 (`0x0042`)**: This event is triggered if the bus access 
  occurs through DMA Access (Read or Write).
  - **TRIG_A (`0x0044`)**: This event occurs on all bus accesses, except 
  if caused by DMA.
  - **TRIG_B (`0x0046`)**: This event occurs on a write bus access, 
  except thouse caused by DMA.
  - **TRIG_C (`0x0060`)**: This event occurs for read bus access, except 
  on an Instruction Fetch and also not a DMA.
  - **TRIG_D (`0x0062`)**: This event occurs on a read bus access, 
  excluding those caused by a DMA.
  - **TRIG_E (`0x0064`)**: This event occurs on a read bus access caused 
  by a DMA.
  - **TRIG_F (`0x0066`)**: This event occurs on a write bus access caused 
  by a DMA

The following table shows the relation of the access mode to the signals 
Fetch, R/W and DMA.

| Label  | Operation                                   | Fetch | R/W | DMA |
|--------|---------------------------------------------|-------|-----|-----|
| TRIG_0 | Instuction fetch                            |   1   | (R) | (0) |
| TRIG_1 | Instruction fetch hold                      |   1   | (R) | (0) |
| TRIG_2 | No instruction fetch                        |   0   |  X  |  X  |
|        |                                             |   1   |  X  |  1  |
| TRIG_3 | Don't care                                  |   X   |  X  |  X  |
| TRIG_4 | No intruction fetch & read                  |   0   |  R  |  X  |
|        |                                             |   1   |  R  |  1  |
| TRIG_5 | No instruction fetch & write                |   X   |  W  |  X  |
| TRIG_6 | Read                                        |   X   |  R  |  X  |
| TRIG_7 | Write                                       |   X   |  W  |  X  |
| TRIG_8 | No intruction fetch & no DMA access         |   0   |  X  |  0  |
| TRIG_9 | DMA access (read or write)                  |   X   |  X  |  1  |
| TRIG_A | No DMA access                               |   X   |  X  |  0  |
| TRIG_B | Write & no DMA access                       |   X   |  W  |  0  |
| TRIG_C | No instruction fetch & read & no DMA access |   0   |  R  |  0  |
| TRIG_D | Read & no DMA access                        |   X   |  R  |  0  |
| TRIG_E | Read & DMA access                           |   X   |  R  |  1  |
| TRIG_F | Write & DMA access                          |   X   |  W  |  1  |

- **CMP (b4-b3)**: This field controls the type of comparison to be made: 
  - **CMP_EQUAL (`0x0000`)**: The trigger value must be equal to  
  `MBTRIG0VAL`.
  - **CMP_GREATER (`0x0008`)**: The trigger value must be greater than 
  `MBTRIG0VAL`.
  - **CMP_LESS (`0x0010`)**: The trigger value must be less than 
  `MBTRIG0VAL`.
  - **CMP_NOT_EQUAL (`0x0018`)**: The trigger value must be different 
  than `MBTRIG0VAL`.


#### MBTRIGxMSK register (`TB`*n*+`0x0004`)

Each memory trigger block can be independently selected to compare either 
the MAB or the MDB with a given value. Depending on the implemented EEM, 
the comparison can be =, ≠, ≥, or ≤. 

The comparison can also be **limited to certain bits with the use of a 
mask**. The mask is either **bit-wise** or **byte-wise**, depending upon 
the device. 

In addition to selecting the bus and the comparison, the condition under 
which the trigger is active can be selected. The conditions include read 
access, write access, DMA access, and instruction fetch.

|  23 |  22 |  21 |  20 |  19 |  18 |  17 |  16 |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|  x  |  x  |  x  |  x  | MSK | MSK | MSK | MSK |
| **15** | **14** | **13** | **12** | **11** | **10** | **9** | **8** |
| MSK | MSK | MSK | MSK | MSK | MSK | MSK | MSK |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
| MSK | MSK | MSK | MSK | MSK | MSK | MSK | MSK |

This is the mask register, which is up to 20 bits, dependending on the 
CPU model.

The `EEM_defs.h` predefines some common values:
- **NO_MASK (`0x00000`)**
- **MASK_ALL (`0xFFFFF`)**
- **MASK_XADDR (`0xF0000`)**
- **MASK_HBYTE (`0x0FF00`)**
- **MASK_LBYTE (`0x000FF`)**


#### MBTRIGxCMB register (`TB`*n*+`0x0004`)

Triggers can be combined to form more complex triggers. Once a condition 
is met, you can signal it to one of the matrix outputs.

Depending on the device you have up to 10 outputs:

|  15 |  14 |  13 |  12 |  11 |  10 |  9  |  8  |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|  x  |  x  |  x  |  x  |  x  |  x  | EN9 | EN8 |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
| EN7 | EN6 | EN5 | EN4 | EN3 | EN2 | EN1 | EN0 |

This register uses the same definitions as the **BREAKREACT** register, 
described below.


### BREAKREACT (`0x80`)

This is the break reaction register, that individually controls the 
**CPU Stop** feature of the EEM. Each bit activates one line that is 
OR-ed to stop the CPU.

Note that the implementation of each bit depends on the size of the 
**Combination triggers AND-Matrix**. Each bit corresponds to the each 
ordered output of the Matrix.

|  15 |  14 |  13 |  12 |  11 |  10 |  9  |  8  |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|  x  |  x  |  x  |  x  |  x  |  x  | EN9 | EN8 |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
| EN7 | EN6 | EN5 | EN4 | EN3 | EN2 | EN1 | EN0 |

- **EN0**: This is the output 0 of the AND-Matrix. This output is present 
on all devices.
- **EN1**: This is the output 1 of the AND-Matrix. This output is present 
on all devices.
- **EN2**: This is the output 2 of the AND-Matrix. This output is present 
on all devices, except the **XS** parts.
- **EN3**: This is the output 3 of the AND-Matrix. This output is present 
on all devices, except the **XS** parts.
- **EN4**: This is the output 4 of the AND-Matrix. This output is present 
on the **M** and **L** devices.
- **EN5**: This is the output 5 of the AND-Matrix. This output is present 
on the **M** and **L** devices.
- **EN6**: This is the output 6 of the AND-Matrix. This output is only 
present on the **L** devices.
- **EN7**: This is the output 7 of the AND-Matrix. This output is only 
present on the **L** devices.
- **EN8**: This is the output 8 of the AND-Matrix. This output is only 
present on the **L** devices.
- **EN9**: This is the output 9 of the AND-Matrix. This output is only 
present on the **L** devices.


### STOR_REACT (`0x98`)

This is the store reaction register, that individually controls the 
data storage feature of the EEM. Each bit enables the respective line 
that is OR-ed to trigger a state storage.

The state storage function uses a built-in buffer to store MAB, MDB, and 
CPU control signal information (that is, read, write, or instruction 
fetch) in a nonintrusive manner. The built-in buffer can hold up to eight 
entries. The flexible configuration allows the user to record the 
information of interest very efficiently.


### Register for the Event/Cycle counter


#### EVENT_REACT (`0x94`)

This is the event reaction register, where each bit enables a connection 
to the **AND-matrix Combination registers**, used to trigger a cycle 
counter.

The **cycle counter** provides one or two 40-bit counters to measure the 
cycles used by the CPU to execute certain tasks. On some devices, the 
cycle counter operation can be controlled using triggers. This allows, 
for example, conditional profiling, such as profiling a specific 
section of code. 


#### EVENT_CTRL (`0x96`)

This register is the general control for the cycle counter.

|  15 |  14 |  13 |  12 |  11 |  10 |  9  |  8  |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|  x  |  x  |  x  |  x  |  x  |  x  |  x  |  x  |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
|  x  |  x  |  x  |  x  |  x  |  x  |  x  | EVT |

- **EVENT_TRIG (`0x0001`)** TODO: Enables the feature?


#### CCNT0CTL (`0xB0`) / CCNT1CTL (`0xB8`)

|  15 |  14 |  13 |  12 |  11 |  10 |  9  |  8  |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|  x  |  x  | CLR | CLR | STP | STP | STT | STT |
| **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
|  x  | RST |  x  |  x  |  x  | MOD | MOD | MOD |

- **MOD** can be one of the following values:
  - **CCNTMODE0 (`0x0000`)**: Counter stopped
  - **CCNTMODE1 (`0x0001`)**: Increment on reaction
  - **CCNTMODE4 (`0x0004`)**: Increment on instruction fetch cycles
  - **CCNTMODE5 (`0x0005`)**: Increment on all bus cycles (including DMA 
  cycles)
  - **CCNTMODE6 (`0x0006`)**: Increment on all CPU bus cycles (excluding 
  DMA cycles)
  - **CCNTMODE7 (`0x0007`)**: Increment on all DMA bus cycles
- **RST (`0x0040`)** Reset counter
- **STT** can be one of the following values:
  - **CCNTSTT0 (`0x0000`)**: Start when CPU released from JTAG/EEM
  - **CCNTSTT1 (`0x0100`)**: Start on reaction `CCNT1REACT` (only 
  `CCNT1`) 
  - **CCNTSTT2 (`0x0200`)**: Start when other (second) counter is started 
  (only if available)
  - **CCNTSTT3 (`0x0300`)**: Start immediately
- **STP** can be one of the following values:
  - **CCNTSTP0 (`0x0000`)**: Stop when CPU is stopped by EEM or under 
  JTAG control
  - **CCNTSTP1 (`0x0400`)**: Stop on reaction `CCNT1REACT` (only `CCNT1`) 
  - **CCNTSTP2 (`0x0800`)**: Stop when other (second) counter is started 
  (only if available)
  - **CCNTSTP3 (`0x0C00`)**: No stop event
- **CLR** can be one of the following values:
  - **CCNTCLR0 (`0x0000`)**: No clear event
  - **CCNTCLR1 (`0x1000`)**: Clear on reaction `CCNT1REACT` (only 
  `CCNT1`) 
  - **CCNTCLR2 (`0x2000`)**: Clear when other (second) counter is started 
  (only if available)
  - **CCNTCLR3 (`0x3000`)**: Reserved


#### CCNT0L (`0xB2`) / CCNT0H (`0xB4`) / CCNT1L (`0xBA`) / CCNT1H (`0xBC`)

These registers stores the 24-bit counters 0 and 1.


#### CCNT1REACT (`0xBE`)

This register contains the reaction value only implemented in `CCNT1`, 
for the specific configurations: `CCNTSTT1`, `CCNTSTP1` and `CCNTCLR1`. 
