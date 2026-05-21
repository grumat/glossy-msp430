
P:\github.com\glossy-msp430\Firmware.shared\res\WriteFlashXv2.elf:     file format elf32-msp430


Disassembly of section .lower.text:

00005c00 <main>:
** Stack pointer needs to be initialized and have 4 bytes space
*/
void WriteFlashXv2(uint16_t* addr, uint16_t cnt)
{
	// Stop WDT
	WDTCTL = WDTPW | WDTHOLD;
    5c00:	b2 40 80 5a 	mov	#23168,	&0x015c	;#0x5a80
    5c04:	5c 01 

00005c06 <.Loc.22.1>:
	// Indicates driver start
	SYSJMBO1 = 0xABAD;
    5c06:	b2 40 ad ab 	mov	#-21587,&0x018e	;#0xabad
    5c0a:	8e 01 

00005c0c <.Loc.23.1>:
	SYSJMBO0 = 0xBABE;
    5c0c:	b2 40 be ba 	mov	#-17730,&0x018c	;#0xbabe
    5c10:	8c 01 

00005c12 <.L2>:

	// Flash busy?
	while (FCTL3 & BUSY)
    5c12:	92 b3 44 01 	bit	#1,	&0x0144	;r3 As==01
    5c16:	fd 23       	jnz	$-4      	;abs 0x5c12

00005c18 <.Loc.30.1>:
		;

	// Toggle LOCKA if not already 0 (see docs)
	FCTL3 = FWPW | (FCTL3 & LOCKA);
    5c18:	1e 42 44 01 	mov	&0x0144,r14	;0x0144
    5c1c:	7e f0 40 00 	and.b	#64,	r14	;#0x0040

00005c20 <.Loc.30.1>:
    5c20:	3e d0 00 a5 	bis	#-23296,r14	;#0xa500
    5c24:	82 4e 44 01 	mov	r14,	&0x0144	;

00005c28 <.Loc.33.1>:

	// Flash write word access
	FCTL1 = FWPW + WRT;
    5c28:	b2 40 40 a5 	mov	#-23232,&0x0140	;#0xa540
    5c2c:	40 01 

00005c2e <.Loc.35.1>:
	// Disable auto-clear feature, 16-bit mode
	SYSJMBC = JMBCLR0OFF + JMBMODE;
    5c2e:	b2 40 50 00 	mov	#80,	&0x0186	;#0x0050
    5c32:	86 01 
    5c34:	3d 53       	add	#-1,	r13	;r3 As==11

00005c36 <.LVL1>:
    5c36:	0e 4d       	mov	r13,	r14	;
    5c38:	ae 00 01 00 	adda	#1,	r14	;
    5c3c:	4e 02       	rlam.a	#1,	r14	;
    5c3e:	ee 0c       	adda	r12,	r14	;

00005c40 <.L3>:
	// Repeat for all expected words
	do
	{
		// Wait for data available in the mailbox?
		while ((SYSJMBC & JMBIN0FG) == 0)
    5c40:	92 b3 86 01 	bit	#1,	&0x0186	;r3 As==01
    5c44:	fd 27       	jz	$-4      	;abs 0x5c40

00005c46 <.Loc.43.1>:
			;
		// Move data to memory
		*addr++ = SYSJMBI0;
    5c46:	ac 00 02 00 	adda	#2,	r12	;

00005c4a <.LVL3>:
    5c4a:	4f 18 9c 42 	movx.w	&0x00188,-2(r12)	; 0xffffe
    5c4e:	88 01 fe ff 

00005c52 <.Loc.44.1>:
		SYSJMBC &= ~JMBIN0FG;
    5c52:	92 c3 86 01 	bic	#1,	&0x0186	;r3 As==01

00005c56 <.L4>:
		// Wait for flash operation
		while (FCTL3 & BUSY)
    5c56:	92 b3 44 01 	bit	#1,	&0x0144	;r3 As==01
    5c5a:	fd 23       	jnz	$-4      	;abs 0x5c56

00005c5c <.Loc.49.1>:
			;
	}
	while (--cnt);
    5c5c:	de 0c       	cmpa	r12,	r14	;
    5c5e:	f0 23       	jnz	$-30     	;abs 0x5c40

00005c60 <.Loc.52.1>:

	// Lock flash again
	FCTL1 = FWPW;
    5c60:	b2 40 00 a5 	mov	#-23296,&0x0140	;#0xa500
    5c64:	40 01 

00005c66 <.Loc.54.1>:
	// Toggle if LOCKA do disable
	FCTL3 = FWPW + LOCK + LOCKA;
    5c66:	b2 40 50 a5 	mov	#-23216,&0x0144	;#0xa550
    5c6a:	44 01 

00005c6c <.Loc.57.1>:

	// Signal end of function
	SYSJMBO1 = 0xCAFE;
    5c6c:	b2 40 fe ca 	mov	#-13570,&0x018e	;#0xcafe
    5c70:	8e 01 

00005c72 <.Loc.58.1>:
	SYSJMBO0 = 0xBABE;
    5c72:	b2 40 be ba 	mov	#-17730,&0x018c	;#0xbabe
    5c76:	8c 01 

00005c78 <.FOREVER>:
	}
#else
	__asm volatile(
	".FOREVER:\r\n"
		"	jmp	.FOREVER\r\n"
		);
    5c78:	ff 3f       	jmp	$+0      	;abs 0x5c78
