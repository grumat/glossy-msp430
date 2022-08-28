# BMPMSP2-stick

> This is a **physical model** prototype of the Glossy MSP430, which has 
> the objective to check if the PCB project can fit this commercially 
> available plastic case, including the fact that the 14-pin JTAG 
> connector can replace the original 10-pin form by just moving it outside 
> the case border.  
> The circuit sent to fabrication actually works, but has a deprecated 
> pin layout which will not be maintained. Many other aspects will also 
> change in the future. Until a V2 version of this layout will be 
> developed, the information on this page, including schematics are just 
> a rough reference.  
> By the way, the results of this **physical model** was a success.

The second option uses the same principle, but is a more compact option, and uses the form factor of the ST-Link *baite* version.

[<img src="https://wiki.cuvoodoo.info/lib/exe/fetch.php?cache=&media=jtag:baite_dongle_front.jpg">](https://wiki.cuvoodoo.info/lib/exe/fetch.php?cache=&media=jtag:baite_dongle_front.jpg)

And a rough 3D image of the board is:

![alt text](images/MSPBMP2-stick.png "BMP-MSP430-2-stick")

I've just bough on Amazon or AliExpress both ST-Link devices and replaced the contents by the PCB's. The PCB's were produced by JLCPCB, which already mounted the SMD components, helping much on the final result.

These are the pictures of the physical prototype:

![baite-closed-fs8.png](images/baite-closed-fs8.png)

> Note that the 14-pin connector, which was moved outside the plastic 
> case, fits perfectly to the case design.

![baite-open-fs8.png](images/baite-open-fs8.png)

The evaluation of this prototype had the following results or 
observations: 
- Although the design of the case meant to use the 10-pin connector at 
the case end limit, and the concept was to move it straight to the 
outside border of the case. Alignment are very good as seen on both pictures.
- The PCB design has two recesses so that the board locks inside the case 
are perfect. When the board is inserted into the case no mechanical 
movements were observed.
- In this board, the 10-pin miniature connector is not at the center and 
collides with the top cover, so it is not possible to close the case if 
the component is mounted.
