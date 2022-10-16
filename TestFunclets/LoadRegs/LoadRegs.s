#include <msp430.h>

.text
	.balign 2
	.global	main
	.type	main, @function

main:
	MOV.W #0x5A80,&WDTCTL		; 6-bytes wide
	MOV.W 0(R12), R1
	MOV.W 2(R12), R4
	MOV.W 4(R12), R5
	MOV.W 6(R12), R6
	MOV.W 8(R12), R7
	MOV.W 10(R12), R8
	MOV.W 12(R12), R9
	MOV.W 14(R12), R10
	MOV.W 16(R12), R11
	MOV.W 20(R12), R13
	MOV.W 22(R12), R14
	MOV.W 24(R12), R15
	MOV.W 18(R12), R12

.FOREVER:
	JMP .FOREVER
