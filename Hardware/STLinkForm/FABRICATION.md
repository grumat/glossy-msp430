# Fabrication Hints (V2)

## JLCPCB Order

For the fabrication of samples, use the [output\STLinkForm-grumat.zip](output\STLinkForm-grumat.zip) 
file. This can be sent directly to JLCPCB, as design was once approved 
and produced in 08-Dec-2022.

These are the order details:

| Property                | Value                                 |
|:------------------------|:--------------------------------------|
| Base Material           | FR-4                                  |
| Layers                  | 2                                     |
| Dimension               | 73.7 mm * 38.1 mm                     |
| PCB Thickness           | 1.6                                   |
| Impedance Control       | No                                    |
| PCB Qty                 | 5 (min)                               |
| PCB Color               | Green (fastest delivery time)         |
| Silkscreen              | White                                 |
| Via Covering            | Tented                                |
| Surface Finish          | HASL(with lead)                       |
| Deburring/Edge rounding | No                                    |
| Outer Copper Weight     | 1                                     |
| Gold Fingers            | No                                    |
| Flying Probe Test       | Fully Test                            |
| Castellated Holes       | No                                    |
| Remove Order Number     | No                                    |
| 4-Wire Kelvin Test      | No                                    |
| Material Type           | FR4-Standard TG 135-140               |
| Paper between PCBs      | No                                    |
| Appearance Quality      | IPC Class 2 Standard                  |
| Confirm Production file | No                                    |
| Silkscreen Technology   | Ink-jet/Screen Printing Silkscreen    |
| Cost                    | €3.80 for 5 pcs                       |

> Note that the file **does not contain** information for pick and place 
> mounting by [JLCPCB](https://jlcpcb.com/). These are just hand soldered. 


## Special Parts and Exceptions

The part list can be found in the [STLinkForm.csv](output/STLinkForm.csv) 
file. 

### Y1 8MHz Crystal

A common concern with crystals is that they required tuned **CL** 
capacitors, that are required for stable operation.

In this design the crystal load capacitors are **C1** and **C4**, which 
have **20pF** each. You are allowed to use any 8Mhz 3525 crystal as long 
as the correct load capacitors are used for the part you have.

> If you don´t follow this rule it you may run into issues with the 
> stability of your USB connection and other forms of drop-outs.

<big>**ERRATUM:**</big> 


### Würth 15141RV31 LED

Very important to know is, that this PCB version contains 
an error and the *Würth 15141RV31* part number is **not compatible** 
with the PCB.

A compatible replacement part is the **Kingbright AAA3528SURKCGKS** but 
it also has a caveat: orientation is rotated in 180&ordm;. If you don't 
follow this guidelines, LED colors will be inverted.

A future release of this PCB will fix this issue.


### U1 (SRV05-4)

The **U1** (SRV05-4) has double connection to each USB signal line. This 
avoids reliable connection because of the added capacitance. Please, 
gently cut pins **4** and **6** before soldering the component. 
Alternatively, you can cut pins **1** and **3**, if you believe it is 
better for handling solder task.

A future release of this PCB will fix this issue.


## STM32F103CBT6

Since the chip crisis, this MCU is still not easy to find, although 
slowly recovering stocks. Currently an alternative being validated is the 
[APM32F103CBT6](https://global.geehy.com/apm32?id=14) that can be found 
on *generic electronic stores*.


## Soldering Tips

These are the most important hints:

- A real game changer for mounting SMD components was the acquisition of 
a magnifying lense, directly coupled to my glasses:  
[Carson 2x Clip and Flip Power Magnifying Lenses](https://amzn.eu/d/3bhR9gK)
- Tweezers and a couple of toothpicks will help handle the tiny parts.
- The recommended mounting order is very important for a good result.
- Start with components based on it's height. This helps soldering.
- Through hole elements are always mounted at the last step.
- Two soldering methods were used:
  - Solder iron, adding sufficient solder flux to avoid short circuits.
  - Hot Air gun, using a 1:1 mix of solder paste and flux.
- Recommended order:
  - Resistor networks:  **RN1**, **RN2**, **RN3** and **RN4**.  
  **Method:** Hot air gun.
  - ICs in the following order: **U8**, **U2**, **U4**, **U5**.  
  **Method:** Solder iron.
  - SMD 0603 resistors: all of them.  
  **Method:** Solder iron.
  - **L1** SMD 0603 filter.  
  **Method:** Solder iron.
  - SMD 0603 fuse (**F1**).  
  **Method:** Solder iron.
  - **Y1** crystal: *Please read the note at the start of this document regarding crystal and matching load capacitors (**C1** and **C4**).  
  **Method:** Hot air gun.
  - SMD 0603 capacitors: Thickness will vary on these devices and they 
  are far more trickier than SMD resistors. Solder order will depend on 
  the specific thickness of each value. Usually lower values have also 
  lower body sizes, but that is not always true when mixing brands.  
  **Method:** Solder iron.
  - SOT packaged ICs: **U1**, **U3**,  **U6**, **U7** and **U9**. *Make 
  sure to read the **Erratum** at the start of this document*.  
  **Method:** Solder iron.
  - Diode and LED: **D1**, **D2**. *Make sure to read the **Erratum** at
  the start of this document*.  
  **Method:** Solder iron.
  - **J1** Micro USB connector.  
  **Method:** Solder iron.
  - At this point, perform a shallow clean-up with isopropyl alcohol, a 
  toothbrush and paper towel.
  - Any SMD part not listed before should be soldered before continuing 
  the next step.
  - **J2** JTAG 14-pin Connector.  
  **Method:** Solder iron.
  - **J3** UART/RS232 4-pin connector.  
  **Method:** Solder iron.
  - **Do not solder J4**. Gently tilting pin header wires you will be 
  able to program a firmware. Besides, soldering a pin header will 
  **hinder to close the plastic case**.
  - SWD jumper **J5** is **optional:** and required only for firmware 
  development.  
  **Method:** Solder iron.
  


## Alternative Components

### Resettable Fuse (F1)

The recommended resettable fuse is for 100 mA. The OpAmp is designed for 
250 mA, but excessive heat could be produced on that circuit if drawing 
so much current. A design based on MSP430 is ultra low power and 100 mA 
is usually much more than typical.

This can be replaced by a low value resistor (2R2 to 4R7), as long as 
good practice is maintained and no short circuits are made for long 
periods.

Some valid mouser.com parts are:
- 0603L010YR
- PTS060315V010


### ESD Protection (U6 and U7)

These chips are dispensable as they have no effect on the device 
function, except for ESD discharging, which could happen when connecting 
or reconnecting cables.

In the case these components are not mounted, take care when handling 
boards, keeping your body discharged (ESD wrist cable or similar 
procedure).

Besides operating voltage, which is rather standard for these parts, the 
parasitic capacitance on each pin should be as low as possible.

Some valid mouser.com parts are:
- XBP15SRV05W-G (1.2 pF)
- SRV05-4L-TP (1.5 pF)
- SRV05-4HTG-D (2.4 pF)
- SRV05-4.TCT (5 pF)
- CDSOT23-SRV05-4 (5 pF)


### LDO &mdash; Voltage Regulator (U3)

On the part list this component is listed as HT7833, manufactured by 
Holtek, which is easy to be found on *generic electronic* sites, but not 
for USA based distributors.

In general, compatibility is easy on this part. Just follow this requirements:
- **SOT-89** package (3 pins, sometimes listed as **SOT-89-3**)
- At least 400 mA output current
- Dropout voltage below **1.5V**
- Pin 1 is **GND**
- Pin 2 and 4 (flange) **Input** (sometimes listed as **VIN**)
- Pin 3 is **Output** (sometimes listed as **VOUT**)

These are some compatible Mouser part numbers:
- 621-AP7215-33YG-13
- 621-AP7365-33YRG-13
- 621-AZ1117CR2-3.3TRG
- 865-XC6203P332PR-G


### JTAG 14-pin Connector

This connector uses regular dual row 2.54 mm pin-headers. On regular 
MSP430 boards a connector having a frame is used. This is not the case 
on this design, since the plastic case offers the mechanical alignment 
for a safe connection.

The best source for this type of component are chinese sites, as 
mouser.com prices are abysmal.


### General Part Number Table

Although most parts are flooded with alternatives, below follows a table 
with recommended values:

| Designators                   |     Value   | Mouser.com Part Number |
|:-----------------------------:|:-----------:|:-----------------------|
| C1, C4 *                      |        8 pF | 791-0603N8R0C500CT     |
|                               |       10 pF | 603-CC603JRNPO0BN100   |
|                               |       12 pF | 581-06031A120JAT4A     |
|                               |       18 pF | 791-0603N180J101CT     |
|                               |       20 pF | 791-0603N200J101CT     |
| C2, C19                       |       22 µF | 81-GRM188R61A226ME5J   |
| C9, C14, C18, C23, C26        |       10 µF | 81-GRM188Z71A106KA3D   |
| C3, C6, C8, C10, C11, C13, C15, C16, C17, C20, C21, C22, C25, C27, C28 | 100 nF | 187-CL10B104KA85PNC |
| C5, C7                        |       12 pF | 581-06031A120JAT4A     |
| C12                           |      470 nF | 791-0603B474K6R3CT     |
| C24                           |        1 nF | 791-MT18N102J500CT     |
| D1                            |             | 771-PMEG4010BEAT/R     |
| D2                            |             | 604-AAA3528SURKCGKS    |
| F1                            |      100 mA | 576-0603L010YR, 504-PTS060315V010 |
| J1                            |     USB     | 538-105017-0001        |
| J2                            |     JTAG    | 649-1012938191404BLF   |
| J3                            |     UART    | 538-22-23-2041         |
| J4                            |     ISP     | ---           |
| J5                            |     SWD     | 855-M50-3600542R       |
| L1                            |     1k8     | 81-BLM18BD182SN1D      |
| R1, R9, R10, R13              |  100 k&ohm; | 603-RC0603FR-13100KL   |
| R2, R7                        |    22 &ohm; | 603-RC0603JR-1322RL    |
| R3, R4, R14                   |  4.7 k&ohm; | 603-AC0603FR-7W4K7L    |
| R5, R8                        |    1 k&ohm; | 603-RT0603FRE071KL     |
| R6                            |  1.5 k&ohm; | 603-RC0603FR-101K5L    |
| R11, R12                      |   330 &ohm; | 603-RC0603JR-07330RL   |
| RN1, RN2                      | 4x100 &ohm; | 667-EXB-28V101JX       |
| RN3, RN4                      | 4x47 k&ohm; | 667-EXB-28V473JX       |
| U1, U7, U9 | SRV05-4 | 865-XBP15SRV05W-G, <br/> 833-SRV05-4L-TP, <br/> 576-SRV05-4HTG-D, <br/> 947-SRV05-4.TCT, <br/> 652-CDSOT23-SRV05-4 |
| U2                            |STM32F103CBT6| 511-STM32F103CBT6      |
| U3 * | 3.3V; 400++ mA | 621-AP7215-33YG-13 <br/> 621-AP7365-33YRG-13 <br/> 621-AZ1117CR2-3.3TRG <br/> 865-XC6203P332PR-G |
| U4, U5 | 74xxx125 | 771-74LVC125APWQ100J, <br/> 771-74LVC125APW-T, <br/> 771-ALVC125PW118, <br/> 595-N74LVC125AIPWREP |
| U6                            |  AD8531ARTZ | 584-AD8531ARTZ-R, <br/> 584-AD8531ARTZ-R7 |
| U8 | 74xxx2T45DC | 595-SN74LVC2T45DCUE4, <br/> 595-SN74LVC2T45DURG4, <br/> 595-SN74LVC2T45DCUR, <br/> 771-AVC2T45DC125, <br/> 771-AVCH2T45DC125, <br/> 771-74AXP2T45DCH, <br/> 595-SN74AXCH2T45DCUR, <br/> 595-SN74LXC2T45DCUR, <br/> 595-SN74AVC2T45DCUR |
| Y1 *                          |  8 MHz, CL8 | 520-80-8-33-JGN-TR3    |
|                               | 8 MHz, CL10 | 520-ECS-80-1033CHNTR   |
|                               | 8 MHz, CL12 | 520-80-12-33-JGN-TR    |
|                               | 8 MHz, CL18 | 520-80-18-33-JGN-TR    |
|                               | 8 MHz, CL20 | 520-80-20-33-JEN-TR3   |

* See exceptions on top of this document.

> Please check for the last stand on the [output/STLinkForm.csv](output/STLinkForm.csv) 
> file.


## Final Inspection

### PCB Rinsing

Before using the finalized prototype board, perform a thorough clean-up 
of the PCB removing all paste residue with isopropyl alcohol, toothbrush 
and paper towel.

### Electrical Validation

A minimal checkup is required before using this kit:
- Ensure that the JTAG connector has no cable connected.
- Power up the Glossy MSP430 using an USB cable and connected to your PC 
or an USB charger.
- Use a voltmeter and control the supply voltage SWD4p terminals of the 
PCB.

