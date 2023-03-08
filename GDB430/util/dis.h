/* MSPDebug - debugging tool for the eZ430
 * Copyright (C) 2009, 2010 Daniel Beer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DIS_H_


/* Addressing modes.
 *
 * Addressing modes are not determined solely by the address mode bits
 * in an instruction. Rather, those bits specify one of four possible
 * modes (REGISTER, INDEXED, INDIRECT and INDIRECT_INC). Using some of
 * these modes in conjunction with special registers like PC or the
 * constant generator registers results in extra modes. For example, the
 * following code, written using INDIRECT_INC on PC:
 *
 *     MOV      @PC+, R5
 *     .word    0x5729
 *
 * can also be written as an instruction using IMMEDIATE addressing:
 *
 *     MOV      #0x5729, R5
 */
typedef enum {
	MSP430_AMODE_REGISTER           = 0x0,
	MSP430_AMODE_INDEXED            = 0x1,
	MSP430_AMODE_SYMBOLIC           = 0x81,
	MSP430_AMODE_ABSOLUTE           = 0x82,
	MSP430_AMODE_INDIRECT           = 0x2,
	MSP430_AMODE_INDIRECT_INC       = 0x3,
	MSP430_AMODE_IMMEDIATE          = 0x83
} msp430_amode_t;

/* MSP430 registers.
 *
 * These are divided into:
 *
 *     PC/R0:    program counter
 *     SP/R1:    stack pointer
 *     SR/R2:    status register/constant generator 1
 *     R3:       constant generator 2
 *     R4-R15:   general purpose registers
 */
typedef enum {
	MSP430_REG_PC           = 0,
	MSP430_REG_SP           = 1,
	MSP430_REG_SR           = 2,
	MSP430_REG_R3           = 3,
	MSP430_REG_R4           = 4,
	MSP430_REG_R5           = 5,
	MSP430_REG_R6           = 6,
	MSP430_REG_R7           = 7,
	MSP430_REG_R8           = 8,
	MSP430_REG_R9           = 9,
	MSP430_REG_R10          = 10,
	MSP430_REG_R11          = 11,
	MSP430_REG_R12          = 12,
	MSP430_REG_R13          = 13,
	MSP430_REG_R14          = 14,
	MSP430_REG_R15          = 15,
} msp430_reg_t;

/* Status register bits. */
#define MSP430_SR_V             0x0100
#define MSP430_SR_SCG1          0x0080
#define MSP430_SR_SCG0          0x0040
#define MSP430_SR_OSCOFF        0x0020
#define MSP430_SR_CPUOFF        0x0010
#define MSP430_SR_GIE           0x0008
#define MSP430_SR_N             0x0004
#define MSP430_SR_Z             0x0002
#define MSP430_SR_C             0x0001

int dis_reg_from_name(const char *name);
const char *dis_reg_name(msp430_reg_t reg);

#endif
