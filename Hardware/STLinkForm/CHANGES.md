# Changes for STLinkForm hardware

## Version 1

The following fixes are required, which can be patched during board 
assembly:


### U1 (SRV05-4)

The **U1** (SRV05-4) has double connection to each USB signal line. This 
avoids reliable connection because of the added capacitance. Please, 
gently cut pins **4** and **6** before soldering the component. 
Alternatively, you can cut pins **1** and **3**, if you believe it is 
better for handling solder task.


### CL1 and CL2 for the XTAL

The **C1** and **C4** should be paired with the characteristics of your 
XTAL device, according to it's datasheet. In my case I had a generic XTAL 
I've bought in AliExpress and system worked best without any capacitor.
I am considering the addition of a very low value like 4.7pF on each 
place, if board continues to show reliability problems.


## Extra NRST Pullup

The design has a **R10** pullup for the reset circuit. This component can 
be ignored, since STM32F103 has an internal pullup on this pin (see 
datasheet).


## LED component

The PCB footprint used for this device is not pin compatible to the 
**Wuerth 150141RV731** component, although it fits the size. So do not 
mount this component, because it won't work.
I suggest using two **0603** or **0805** LEDs instead, and mount them in 
alternate polarity:

```
--------------------------------
|                              |
|        |              |      |
|        |              |      |
|   ----------     ----------  |
|    \      /          /\      |
|     \    /          /  \     |
|      \  /          /    \    |
|       \/          /      \   |
|   ----------     ----------  |
|        |              |      |
|        |              |      |
|                              |
--------------------------------
              D2
```

They will not fit perfectly, but you can connect them by using some 
solder iron skills.

> The [Kingbright AAA3528SURKCGKS](https://octopart.com/aaa3528surkcgkc09-kingbright-85364949) 
> LED should be compatible with the current board layout. I've ordered 
> some samples and will post updates as soon as I can confirm that.
