/*
 * EEM_defs.h
 *
 * <FILEBRIEF>
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

//! MSP430 EEM (Enhanced Emulation Module) register map and bit definitions.
//! Values are TI's EEM programming model; grouped as typed enums so they remain
//! freely combinable (`addr + kMxWrite`, `bit | bit`) at the JtagPlayer call
//! sites with zero overhead.

//! EEM register addresses (low byte of the EEM address space).
enum EemReg : uint16_t
{
	kBreakReact = 0x80,	//!< CPU-stop reaction register
	kGenCtrl    = 0x82,	//!< General Debug Control Register
	kEemVer     = 0x86,	//!< EEM version
	kGenClkCtrl = 0x88,	//!< General Clock Control Register
	kModClkCtrl0 = 0x8A,	//!< Module Clock Control Register 0
	kTrigFlag   = 0x8E,	//!< Trigger flag register
	kEventReact = 0x94,	//!< Event reaction register
	kEventCtrl  = 0x96,	//!< Event control register
	kStorReact  = 0x98,	//!< State-storage reaction register
	kStorCtl    = 0x9E,	//!< State Storage Control Register
};

//! Power-on default of the kModClkCtrl0 register. Cached in
//! CpuContext::eem_clk_ctrl_ so an EEM read of this register can be served
//! without disturbing the bus.
constexpr uint16_t kModClkCtrl0Default = 0x0417;

//! General Clock Control Register (kGenClkCtrl) bit fields.
enum EemGenClkCtrl : uint16_t
{
	kMclkSel0  = 0x0000,
	kSmclkSel0 = 0x0000,
	kAclkSel0  = 0x0000,
	kMclkSel3  = 0x6000,
	kSmclkSel3 = 0x0C00,
	kAclkSel3  = 0x0180,
	kStopMclk  = 0x0008,
	kStopSmclk = 0x0004,
	kStopAclk  = 0x0002,
};

//! General Debug Control Register (kGenCtrl) bit fields.
enum EemGenCtrl : uint16_t
{
	kEemEn        = 0x0001,
	kClearStop    = 0x0002,
	kEmuClkEn     = 0x0004,
	kEmuFeatEn    = 0x0008,
	kDebTrigLatch = 0x0010,
	kEemRst       = 0x0040,
	kEStopped     = 0x0080,
};

//! Trigger Block base addresses.
enum EemTrigBlock : uint16_t
{
	kTb0 = 0x0000,
	kTb1 = 0x0008,
	kTb2 = 0x0010,
	kTb3 = 0x0018,
	kTb4 = 0x0020,
	kTb5 = 0x0028,
	kTb6 = 0x0030,
	kTb7 = 0x0038,
	kTb8 = 0x0040,
	kTb9 = 0x0048,
};

//! Trigger Block register offsets (added to a kTbx base).
enum EemTrigReg : uint16_t
{
	kMbTrigxVal = 0x0000,
	kMbTrigxCtl = 0x0002,
	kMbTrigxMsk = 0x0004,
	kMbTrigxCmb = 0x0006,
};

//! MAB/MDB Trigger Control Register fields.
enum EemTrigCtrl : uint16_t
{
	kMab   = 0x0000,
	kMdb   = 0x0001,
	kTrig0 = 0x0000,	//!< Instruction Fetch
	kTrig1 = 0x0002,	//!< Instruction Fetch Hold
	kTrig2 = 0x0004,	//!< No Instruction Fetch
	kTrig3 = 0x0006,	//!< Don't care
	kTrig4 = 0x0020,	//!< No Instruction Fetch & Read
	kTrig5 = 0x0022,	//!< No Instruction Fetch & Write
	kTrig6 = 0x0024,	//!< Read
	kTrig7 = 0x0026,	//!< Write
	kTrig8 = 0x0040,	//!< No Instruction Fetch & No DMA Access
	kTrig9 = 0x0042,	//!< DMA Access (Read or Write)
	kTrigA = 0x0044,	//!< No DMA Access
	kTrigB = 0x0046,	//!< Write & No DMA Access
	kTrigC = 0x0060,	//!< No Instruction Fetch & Read & No DMA Access
	kTrigD = 0x0062,	//!< Read & No DMA Access
	kTrigE = 0x0064,	//!< Read & DMA Access
	kTrigF = 0x0066,	//!< Write & DMA Access
	kCmpEqual    = 0x0000,
	kCmpGreater  = 0x0008,
	kCmpLess     = 0x0010,
	kCmpNotEqual = 0x0018,
};

//! MAB/MDB Trigger Mask Register values (20-bit, hence uint32_t).
enum EemTrigMask : uint32_t
{
	kNoMask    = 0x00000,
	kMaskAll   = 0xFFFFF,
	kMaskXAddr = 0xF0000,
	kMaskHByte = 0x0FF00,
	kMaskLByte = 0x000FF,
};

//! MAB/MDB Combination Register & Reaction Register enable bits.
enum EemReact : uint16_t
{
	kEn0 = 0x0001,
	kEn1 = 0x0002,
	kEn2 = 0x0004,
	kEn3 = 0x0008,
	kEn4 = 0x0010,
	kEn5 = 0x0020,
	kEn6 = 0x0040,
	kEn7 = 0x0080,
	kEn8 = 0x0100,
	kEn9 = 0x0200,
};

//! State Storage Control Register (kStorCtl) bit fields.
enum EemStorCtrl : uint16_t
{
	kVarWatch0      = 0x0000,	//!< Two
	kVarWatch1      = 0x2000,	//!< Four
	kVarWatch2      = 0x4000,	//!< Six
	kVarWatch3      = 0x6000,	//!< Eight
	kStorFull       = 0x0200,
	kStorWrit       = 0x0100,
	kStorTest       = 0x0080,
	kStorRst        = 0x0040,
	kStorStopOnTrig = 0x0020,
	kStorStartOnTrig = 0x0010,
	kStorOneShot    = 0x0008,
	kStorMode0      = 0x0000,	//!< Store on enabled triggers
	kStorMode1      = 0x0002,	//!< Store on Instruction Fetch
	kStorMode2      = 0x0004,	//!< Variable Watch
	kStorMode3      = 0x0006,	//!< Store all bus cycles
	kStorEn         = 0x0001,	//!< enable state storage
};

//! Event Control Register (kEventCtrl) bit fields.
enum EemEventCtrl : uint16_t
{
	kEventTrig = 0x0001,
};

//! Cycle Counter register addresses.
enum EemCycleCntReg : uint16_t
{
	kCcnt0Ctl   = 0xB0,
	kCcnt0L     = 0xB2,
	kCcnt0H     = 0xB4,
	kCcnt1Ctl   = 0xB8,
	kCcnt1L     = 0xBA,
	kCcnt1H     = 0xBC,
	kCcnt1React = 0xBE,
};

//! Cycle Counter Control Register bit fields.
enum EemCycleCntCtrl : uint16_t
{
	kCcntMode0 = 0x0000,	//!< Counter stopped
	kCcntMode1 = 0x0001,	//!< Increment on reaction
	kCcntMode4 = 0x0004,	//!< Increment on instruction fetch cycles
	kCcntMode5 = 0x0005,	//!< Increment on all bus cycles (including DMA cycles)
	kCcntMode6 = 0x0006,	//!< Increment on all CPU bus cycles (excluding DMA cycles)
	kCcntMode7 = 0x0007,	//!< Increment on all DMA bus cycles
	kCcntRst   = 0x0040,
	kCcntStt0  = 0x0000,	//!< Start when CPU released from JTAG/EEM
	kCcntStt1  = 0x0100,	//!< Start on reaction kCcnt1React (only CCNT1)
	kCcntStt2  = 0x0200,	//!< Start when other (second) counter is started (if available)
	kCcntStt3  = 0x0300,	//!< Start immediately
	kCcntStp0  = 0x0000,	//!< Stop when CPU is stopped by EEM or under JTAG control
	kCcntStp1  = 0x0400,	//!< Stop on reaction kCcnt1React (only CCNT1)
	kCcntStp2  = 0x0800,	//!< Stop when other (second) counter is started (if available)
	kCcntStp3  = 0x0C00,	//!< No stop event
	kCcntClr0  = 0x0000,	//!< No clear event
	kCcntClr1  = 0x1000,	//!< Clear on reaction kCcnt1React (only CCNT1)
	kCcntClr2  = 0x2000,	//!< Clear when other (second) counter is started (if available)
	kCcntClr3  = 0x3000,	//!< Reserved
};

//! General clock control selector (used by the EEM clock setup).
enum EemGccMode : uint16_t
{
	kGccNone      = 0x0000,	//!< No clock control
	kGccStandard  = 0x0001,	//!< Standard clock control
	kGccExtended  = 0x0002,	//!< Extended clock control
	kGccStandardI = 0x0003,	//!< Standard clock control, special handling Note 1793
};
