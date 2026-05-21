
P:\github.com\glossy-msp430\Firmware.shared\res\EraseXv2.elf:     file format elf32-msp430


Disassembly of section .lower.text:

00005c00 <main>:
** Stack pointer needs to be initialized and have 4 bytes space
*/
void EraseXv2(EraseCtrlXv2 * ctrl)
{
	// Stop WDT
	WDTCTL = WDTPW | WDTHOLD;
    5c00:	b2 40 80 5a 	mov	#23168,	&0x015c	;#0x5a80
    5c04:	5c 01 

00005c06 <.L2>:

	// Wait for Flash idle
	while (FCTL3 & BUSY)
    5c06:	92 b3 44 01 	bit	#1,	&0x0144	;r3 As==01
    5c0a:	fd 23       	jnz	$-4      	;abs 0x5c06

00005c0c <.Loc.27.1>:
	{ }

	// Backup Flash controller settings, correcting password
	uint16_t old_fctl1 = FCTL1 ^ (FRPW ^ FWPW);
    5c0c:	1f 42 40 01 	mov	&0x0140,r15	;0x0140
    5c10:	3f e0 00 33 	xor	#13056,	r15	;#0x3300

00005c14 <.Loc.28.1>:
	uint16_t old_fctl3 = FCTL3 ^ (FRPW ^ FWPW);
    5c14:	1e 42 44 01 	mov	&0x0144,r14	;0x0144
    5c18:	3e e0 00 33 	xor	#13056,	r14	;#0x3300

00005c1c <.LVL2>:

	FCTL3 = ctrl->fctl3_;
    5c1c:	1d 4c 06 00 	mov	6(r12),	r13	;

00005c20 <.Loc.30.1>:
    5c20:	82 4d 44 01 	mov	r13,	&0x0144	;

00005c24 <.Loc.32.1>:
	// LOCKA bit is a toggle bit. Provide the toggle to match
	if((uint8_t)FCTL3 != (uint8_t)ctrl->fctl3_)
    5c24:	1b 42 44 01 	mov	&0x0144,r11	;0x0144

00005c28 <.Loc.32.1>:
    5c28:	4b 9d       	cmp.b	r13,	r11	;
    5c2a:	04 24       	jz	$+10     	;abs 0x5c34

00005c2c <.Loc.33.1>:
		FCTL3 = ctrl->fctl3_ | LOCKA;
    5c2c:	3d d0 40 00 	bis	#64,	r13	;#0x0040
    5c30:	82 4d 44 01 	mov	r13,	&0x0144	;

00005c34 <.L3>:
	// Enable flash write mode
	FCTL1 = ctrl->fctl1_;
    5c34:	40 18 92 4c 	movx.w	4(r12),	&0x00140;
    5c38:	04 00 40 01 

00005c3c <.Loc.37.1>:
	// Dummy write to start erase
	*ctrl->addr_ = 0xFFFF;
    5c3c:	0c 0c       	mova	@r12,	r12	;

00005c3e <.LVL3>:
    5c3e:	bc 43 00 00 	mov	#-1,	0(r12)	;r3 As==11

00005c42 <.L4>:
	// Wait until Flash finishes operation
	while (FCTL3 & BUSY)
    5c42:	92 b3 44 01 	bit	#1,	&0x0144	;r3 As==01
    5c46:	fd 23       	jnz	$-4      	;abs 0x5c42

00005c48 <.Loc.42.1>:
	{ }
	// Restore flash controller
	FCTL1 = old_fctl1;
    5c48:	82 4f 40 01 	mov	r15,	&0x0140	;

00005c4c <.Loc.43.1>:
	FCTL3 = old_fctl3;
    5c4c:	82 4e 44 01 	mov	r14,	&0x0144	;

00005c50 <.Loc.45.1>:
	// Toggle LOCKA bit to 1 (locked) if necessary
	if ((FCTL3 & LOCKA) == 0)
    5c50:	b2 b0 40 00 	bit	#64,	&0x0144	;#0x0040
    5c54:	44 01 
    5c56:	04 20       	jnz	$+10     	;abs 0x5c60

00005c58 <.Loc.46.1>:
		FCTL3 = old_fctl3 | LOCKA;
    5c58:	3e d0 40 00 	bis	#64,	r14	;#0x0040

00005c5c <.LVL4>:
    5c5c:	82 4e 44 01 	mov	r14,	&0x0144	;

00005c60 <.L5>:

	// Notifies controller that operation has completed
	SYSJMBO1 = 0xCAFE;
    5c60:	b2 40 fe ca 	mov	#-13570,&0x018e	;#0xcafe
    5c64:	8e 01 

00005c66 <.Loc.50.1>:
	SYSJMBO0 = 0xBABE;
    5c66:	b2 40 be ba 	mov	#-17730,&0x018c	;#0xbabe
    5c6a:	8c 01 

00005c6c <.FOREVER>:
	{ }
#else
	__asm volatile(
		".FOREVER:\r\n"
		"	jmp	.FOREVER\r\n"
		);
    5c6c:	ff 3f       	jmp	$+0      	;abs 0x5c6c
