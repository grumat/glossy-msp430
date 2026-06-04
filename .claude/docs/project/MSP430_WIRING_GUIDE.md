# Glossy MSP430 — Probe ↔ Target Wiring Guide (DRAFT)

> **Status: draft.** Probe-side and proto-board tables are filled from
> `target.*/platform.h` and the `Hardware/*/README.md` files. Items I inferred
> rather than read verbatim are flagged **⚠ CONFIRM**. The two things only you
> can supply — your physical cable/adapter inventory and any board you own that
> isn't a repo proto-board — are marked **🛠 FILL IN**.
>
> When finalised, promote this to `Hardware/WIRING.md` (a real deliverable) and
> drop the DRAFT banner.

## How to use this guide

1. Find your **probe** (§2) — that fixes the connector type and the SBW wiring
   convention (TI vs ARM-remap).
2. Find your **target board / MCU** (§4) — that tells you whether **JTAG**,
   **SBW**, or **both** are available and how to jumper the mode.
3. Pick the **adapter / cable** (§3) that bridges the two.
4. Mind the **power & voltage** rules (§6) — especially the STLinkV2 3.3 V limit.
5. §5 has the TI 14-pin reference; §7 has ready-made connection recipes.

## 1. Signal glossary

| JTAG name | SBW name | Role | TI 14-pin pin |
|-----------|----------|------|:-------------:|
| TCK  | **SBWTCK** | clock | 7 |
| TMS  | — (folded into SBWTDIO) | mode select | 5 |
| TDI / **TCLK** | — | data in / target clock in RTI | 3 |
| TDO  | — | data out | 1 |
| TEST | **SBWTCK carrier** | enables JTAG/SBW; SBWTCK rides here | 8 |
| RST/NMI | **SBWTDIO** | reset; SBW bidirectional data rides here | 11 |

In **2-wire SBW** only two lines reach the chip: **SBWTCK** (chip TEST pin) and
**SBWTDIO** (chip RST/NMI pin). Every JTAG bit is encoded as 3 cycles on those
two wires. This is why the SBWTDIO line is loaded by the target's reset RC — see
§6.

## 2. Probes (host side)

### 2a. BluePill-G431 "Jiga-Board" (and plain BluePill)

Dual-socket carrier: accepts either a **BluePill (STM32F103)** or a
**G431-in-BluePill-form** board. Target-facing signal map (from
`target.bluepill.g431kb/platform.h` / `Hardware/BluePill-G431/README.md`):

| Signal | MCU pin | Peripheral | Notes |
|--------|:-------:|------------|-------|
| TCK / **SBWCLK** | PA5 | SPI1_SCK | |
| TDO | PA6 | SPI1_MISO | from target |
| TDI / TCLK | PA7 | SPI1_MOSI | to target |
| TMS | PA9 | TIM1_CH2 | timer-driven during frames |
| RST | PA1 | GPIO | bit-bang |
| TEST | PA0 | GPIO | bit-bang |
| **SBWDIO** | PA6/PA7 pair → merged | — | host-side SBWDI/SBWDO merge to one target line |
| SBW direction (SBW_RD) | PA10 | GPIO (TIM1_CH3 capable) | turnaround control |
| Target UART | PB6 (TX) / PB7 (RX) | USART1 | VCP builds only |
| GDB serial | PA2 (TX) / PA3 (RX) | USART2 | non-VCP |
| Target Vcc gen | PA4 (DAC) **or** PB9 (PWM) | DAC1_OUT1 / TIM4_CH4 | one at a time |

- **SBW wiring convention: TI-standard.** SBWCLK rides TCK, SBWDIO rides the
  TDO/TDI pair through the level converter onto the TI RST/TEST lines, so **the
  same TI 14-pin cable carries both JTAG and SBW** — firmware picks the mode.
- **Level translator present** → can drive MSP430 targets **below 3.3 V**
  (unlike STLinkV2).
- Bus buffers are enable-gated (`DIS_RST`/`DIS_TCK`/`DIS_JTAG`/`DIS_COM`); the
  firmware sequences them per mode (see the Bus-buffer-control table in the
  board README).

### 2b. STLinkV2 clone (redesign / clone repurposing)

STM32F103CBT6 (or Geehy APM32F103CB clone). Target-facing map (from
`target.stlinv2/platform.h` / `Hardware/STLinkV2/README.md`):

| Signal | MCU pin | Peripheral | Notes |
|--------|:-------:|------------|-------|
| JTCK / **SBWCLK** | PA5 | SPI1_SCK / GPIO | **shorted to PB13** on PCB |
| JTDO | PA6 | SPI1_MISO | only signal behind a level converter (input xlate) |
| JTDI / JTCLK | PA7 | SPI1_MOSI | |
| JTMS / **SBWDIO** | PB14 | TIM1_CH2N / GPIO | bidirectional SBW data |
| /RST | PB0 | GPIO | |
| TEST / TRST | PB1 | GPIO | adapter needs a strong TEST pull-down |
| SWD_IN (read-back) | PB12 | GPIO | passive echo of PB14 — the SBW read-back path |
| UART | PA2 (TX) / PA3 (RX) | USART2 | borrowed SWIM connector |

- **SBW wiring convention: ARM-SWD remap (NOT TI).** SBWDIO = **TMS** (PB14,
  JTAG-20 pin 7), SBWCLK = **TCK** (PA5). ⚠ **The JTAG-14 connector cannot be
  used for SBW on this probe** — you must use the adapter's dedicated SBW
  connector (§3a).
- **No hardware direction mux** → SBW turnaround is software-paced via the PB12
  passive read-back echo.
- **3.3 V-only outputs.** The clone's translators only translate *inputs*;
  debug outputs are fixed at 3.3 V. **Do not connect an MSP430 powered below
  ~3 V** — over-voltage on the debug pins. (§6)
- Connector options: ARM **20-pin** (standard), or jumper wires for SWD-style
  access; ⚠ 10-pin clones are **SBW-only**. **CONFIRM** which connector your
  unit exposes.

<img src="../../../Hardware/STLinkV2/images/STLinkV2-fs8.png" alt="STLinkV2 board" width="300"> <img src="../../../Hardware/STLinkV2/images/STLinkV2-JTAG-fs8.png" alt="STLinkV2 ARM 10-pin JTAG header" width="300">

## 3. Adapters (repo `Hardware/*-Adapter/`)

### 3a. STLink-Adapter — ST 20-pin → MSP430 (passive)

Bridges a generic STLink 20-pin to MSP430. **For use with the STLinkV2 probe.**

- **JTAG-20 port**: fits a generic STLink (⚠ *not* a J-Link — power pinout
  differs).
- **JTAG-14 port**: TI MSP430-compatible 14-pin. ⚠ **Do NOT use for SBW.**
- **Dedicated SBW connector** (required for SBW): uses the ARM-remap layout —
  **DIO = TMS** (JTAG-20 pin 7), **CLK = TCK**. (TI uses DIO=TDO; Olimex uses
  DIO=RST — this adapter is neither.)
- **Power-select switch**: `Vref` (self-powered target) vs `Vcc` (probe powers
  target).
- Weak **TEST pull-down** included (optional; on-chip pull-down usually
  suffices).
- Passive — turnaround is entirely the probe/firmware's job.

<img src="../../../Hardware/STLink-Adapter/images/STLink-Adapter-fs8.png" alt="STLink-Adapter board" width="280"> <img src="../../../Hardware/STLink-Adapter/images/STLink-Adapter-JTAG14-fs8.png" alt="TI JTAG-14 port" width="220"> <img src="../../../Hardware/STLink-Adapter/images/STLink-Adapter-SBW-fs8.png" alt="Dedicated SBW connector (ARM-remap: DIO=TMS, CLK=TCK)" width="220">

*Left → right: the adapter, its TI JTAG-14 port (JTAG only), and the dedicated SBW connector (ARM-remap — use this for SBW, never the 14-pin).*

### 3b. SBW-Adapter — SBW breakout daughter-board

Robust 2-wire SBW breakout with two switches:
- **Power switch**: TVCC as target **power** vs **voltage reference**.
- **Emulator-style select**: TI (Data-I/O on JTAG **pin 1 / TDO**) vs Olimex
  (Data-I/O on **pin 12 / RST**). ⚠ Confirm which side matches your probe's SBW
  convention before use (Glossy/STLinkV2 = ARM-remap, so verify against §2b).

<img src="../../../Hardware/SBW-Adapter/images/SBW-Adapter-fs8.png" alt="SBW-Adapter breakout" width="280">

### 3c. SWD-Adapter — ARM SWD only *(out of scope)*

For ARM SWD targets (DIO/CLK/SWO/RST), favoring BluePill cabling. **Not an
MSP430 wiring path** — listed only so it isn't mistaken for one.

## 4. Target capability matrix (repo proto-boards)

All proto-boards expose a **standard TI 14-pin JTAG connector** and use the
**TI SBW pin-out** (SBWTCK = chip TEST, SBWTDIO = chip RST/NMI). Mode is chosen
by an on-board jumper block (mutually exclusive — never enable both).

| Users guide | MCU family / package | JTAG | SBW | Mode jumper | Notes |
|-------------|----------------------|:----:|:---:|-------------|-------|
| SLAU049 / SLAU144 | F1xx / F2xx / Gxxx, LQFP64 (+ G2955/22xx 38-pin) | ✅ | ⚠ **38-pin parts only** | none for LQFP64 | LQFP64 parts are JTAG-only; SBW is a 38-pin-package feature |
| SLAU208 | F5418 family (F54xx), LQFP80 | ✅ | ✅ | **J10** (3 jmp = JTAG, 2 = SBW) | main bring-up target |
| SLAU272 / SLAU367 | FR57xx / FR58xx, TSSOP-38 | ✅ | ✅ | **J3** (4 jmp = JTAG, 2 = SBW-TI or Olimex) | FRAM |
| SLAU335 | i20xx, TSSOP-28 | ✅ | ✅ | **J3** (4 jmp = JTAG, 2 = SBW-TI) | 24-bit ADC metering parts |
| SLAU445 | FR2476 family (FR24xx/FR26xx), LQFP48 | ✅ | ✅ | **J5** (4 jmp = JTAG, 2 = SBW-TI or Olimex) | FRAM; FR5994 is a sibling family — see issues #19/#20 |

**Per-board mode jumper detail (TI pin-out):**
- **JTAG** routes: TDO→TDO, TCK→TCK, RST→RESET, (+ TEST→TEST on FRAM/i20xx).
- **SBW (TI)** routes: **TDO→RESET, TCK→TEST** (i.e. chip RST/NMI = SBWTDIO,
  chip TEST = SBWTCK).
- **SBW (Olimex)** where offered: RST→RESET (+ TEST→TEST) — *not* what
  Glossy/STLinkV2 expects unless you match the convention; default to TI.

🛠 **FILL IN:** any non-repo target you wire (LaunchPads, custom boards) — MCU,
package, JTAG/SBW availability, connector.

### 4.1 Board photos & mode-jumper images

Each row: the board, then the **JTAG** jumper set and the **SBW (TI)** jumper
set (where the board README provides them). The silk-screen on each board is the
authority.

**SLAU049 / SLAU144 (F1xx/F2xx/Gxxx)** — JTAG-only except 38-pin parts; README
has no jumper images.

<img src="../../../Hardware/Target_Proto_Boards/SLAU049_SLAU144/images/MSP_Proto.png" alt="SLAU049/144 generic proto board" width="280">

**SLAU208 (F5418, LQFP80)** — JTAG / SBW via **J10**.

<img src="../../../Hardware/Target_Proto_Boards/SLAU208_F5418/images/F5418.png" alt="F5418 board" width="220"> <img src="../../../Hardware/Target_Proto_Boards/SLAU208_F5418/images/JTAG-fs8.png" alt="J10 JTAG jumpers" width="150"> <img src="../../../Hardware/Target_Proto_Boards/SLAU208_F5418/images/SBW-TI-fs8.png" alt="J10 SBW-TI jumpers" width="150">

**SLAU272 / SLAU367 (FR57xx/FR58xx, TSSOP-38)** — JTAG / SBW via **J3**.

<img src="../../../Hardware/Target_Proto_Boards/SLAU272_SLAU367/images/SLAU272_FR5739-fs8.png" alt="FR5739/FR58xx board" width="220"> <img src="../../../Hardware/Target_Proto_Boards/SLAU272_SLAU367/images/JTAG-fs8.png" alt="J3 JTAG jumpers" width="150"> <img src="../../../Hardware/Target_Proto_Boards/SLAU272_SLAU367/images/SBW-TI-fs8.png" alt="J3 SBW-TI jumpers" width="150">

**SLAU335 (i20xx, TSSOP-28)** — JTAG / SBW via **J3**.

<img src="../../../Hardware/Target_Proto_Boards/SLAU335/images/SLAU335-fs8.png" alt="i20xx board" width="220"> <img src="../../../Hardware/Target_Proto_Boards/SLAU335/images/JTAG-fs8.png" alt="J3 JTAG jumpers" width="150"> <img src="../../../Hardware/Target_Proto_Boards/SLAU335/images/SBW-TI-fs8.png" alt="J3 SBW-TI jumpers" width="150">

**SLAU445 (FR2476, LQFP48)** — JTAG / SBW via **J5**.

<img src="../../../Hardware/Target_Proto_Boards/SLAU445_FR2476/images/FR2476.png" alt="FR2476 board" width="220"> <img src="../../../Hardware/Target_Proto_Boards/SLAU445_FR2476/images/JTAG.png" alt="J5 JTAG jumpers" width="150"> <img src="../../../Hardware/Target_Proto_Boards/SLAU445_FR2476/images/SBW-TI.png" alt="J5 SBW-TI jumpers" width="150">

## 5. TI 14-pin JTAG connector reference ⚠ CONFIRM against board silk

```
        ┌────────────┐
   TDO  1 ●        ● 2   VCC_TOOL  (probe → target supply)
   TDI  3 ●        ● 4   VCC_TARGET (Vref sense)
   TMS  5 ●        ● 6   (nc)
   TCK  7 ●        ● 8   TEST / VPP        ← SBWTCK in SBW mode
   GND  9 ●        ● 10  (nc)
 RST/NMI 11 ●       ● 12  (nc)             ← SBWTDIO rides RST/NMI (pin 11)
       13 ●        ● 14  (nc)
        └────────────┘
```

- **JTAG (4-wire):** TDO(1), TDI(3), TMS(5), TCK(7) + RST(11), TEST(8), GND(9).
- **SBW (2-wire):** **SBWTCK = TEST(8)**, **SBWTDIO = RST/NMI(11)**, GND(9).
- Supply: pin 2 = tool→target power; pin 4 = target Vref to the probe. Select
  with the board's `VSEL` jumper: **Vref** = self-powered target, **Vtool** =
  probe powers the board.

## 6. Power & voltage cautions

- **STLinkV2 is 3.3 V-only on outputs.** Its translators convert inputs only;
  outputs stay at 3.3 V regardless of Vref. **Never** attach an MSP430 running
  below ~3 V to the STLinkV2 path — debug pins are over-driven. The
  BluePill-G431 board *does* have a real level translator and can go lower.
- **SBW wire speed is target-RC-bound.** SBWTDIO sits on the chip RST/NMI pin,
  whose reset RC caps the practical SBW rate (~1.2 MHz on the proto targets,
  even with a series resistor). Long/marginal cabling → step the grade down
  (firmware grades: 200/400/800/1000/1200 kHz).
- **One supply domain.** On the STLinkV2, the JTAG `VCC` input pin ties to
  connector pins 1/2 — keep the probe and target on the same supply rail.
- **VSEL jumper before connecting:** set `Vref` (self-powered) or `Vtool`
  (probe-powered) on the target board *before* plugging the cable.

## 7. Connection recipes

| Probe | Target mode | Cable / adapter | Convention | Voltage |
|-------|-------------|-----------------|-----------|---------|
| BluePill-G431 Jiga | **JTAG** | TI 14-pin ribbon, direct | TI | any (translator) |
| BluePill-G431 Jiga | **SBW** | TI 14-pin ribbon, direct (same cable) | TI | any (translator) |
| STLinkV2 | **JTAG** | STLink-Adapter: ST 20-pin → TI 14-pin | TI | 3.3 V only |
| STLinkV2 | **SBW** | STLink-Adapter **dedicated SBW connector** (⚠ not the 14-pin) | ARM-remap (DIO=TMS, CLK=TCK) | 3.3 V only |
| plain BluePill (in Jiga) | JTAG/SBW | same as BluePill-G431 row | TI | any (translator) |

🛠 **FILL IN:** map the above to the **actual cables/adapters you own** (ribbon
lengths, pin-1 orientation, any home-made jumper looms). Photos or silk labels
welcome — I can transcribe them into a per-cable table.

## 8. Open items to resolve before promoting to `Hardware/WIRING.md`

- [ ] Confirm the TI 14-pin pinout (§5) against your board silk (esp. pins 2/4
      supply and any TXD/RXD on 12/14).
- [ ] Confirm STLinkV2 connector variant (20-pin vs 10-pin SBW-only).
- [ ] Confirm SLAU049/144 SBW availability is strictly the 38-pin package.
- [ ] 🛠 Your physical adapter/cable inventory (§7) and any non-repo targets
      (§4).
- [x] Inline the wiring images from each `Hardware/*/images/` folder (done —
      §2b/§3/§4.1). **BluePill-G431 has no images yet**; add a board photo when
      available.
- [ ] **On promotion to `Hardware/WIRING.md`:** rewrite the image paths from
      `../../../Hardware/...` to `...` (the guide will then sit inside
      `Hardware/`, one level above the board folders).
