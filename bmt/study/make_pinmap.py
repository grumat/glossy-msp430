# This script is used to generate the pinremap.*.h files for the bmt Library
# As input it takes CSV files with the mapping of alternate functions
# You can grab them in
# https://github.com/micropython/micropython/tree/master/ports/stm32/boards
#
# The script is managed quite interactively, where FILE, OUF, DATABASE and
# ORGANIZER have to be tuned for the expected output.

import os
import sys
import re
import csv

if 1:
	FILE = "stm32g474_af.csv"
	OUTF = "pinremap.g4.h"
	OUTF2 = "gpio-types.g4.h"
else:
	FILE = "stm32l496_af.csv"
	OUTF = "pinremap.l4.h"
	OUTF2 = "gpio-types.l4.h"


DATABASE = \
[
	["^ADC(\d+)_IN(\d\d?)$", "an"],
	["^CAN(\d)_RX$", "af"],
	["^CAN(\d)_RXFD$", "af"],
	["^CAN(\d)_TX$", "af"],
	["^COMP(\d)_OUT$", "af"],
	["^COMP(\d)_IN([MP])$", "an"],
	["^DAC(\d)_OUT(\d)$", "an"],
	["^DCMI_D(\d+)$", "af"],
	["^DCMI_[HV]SYNC$", "af"],
	["^DCMI_PIXCLK$", "af"],
	["^DFSDM(\d+)_CKIN(\d+)$", "af"],
	["^DFSDM(\d+)_CKOUT$", "af"],
	["^DFSDM(\d+)_DATIN(\d+)$", "af"],
	["^EVENTOUT$", "af"],
	["^FDCAN(\d)_RX$", "af"],
	["^FDCAN(\d)_TX$", "af"],
	["^FMC_A(\d+)$", "af"],
	["^FMC_CLK$", "af"],
	["^FMC_D(\d+)$", "af"],
	["^FMC_INT$", "af"],
	["^FMC_NE(\d)$", "af"],
	["^FMC_NL$", "af"],
	["^FMC_NBL(\d)$", "af"],
	["^FMC_NCE$", "af"],
	["^FMC_NOE$", "af"],
	["^FMC_NWAIT$", "af"],
	["^FMC_NWE$", "af"],
	["^HRTIM(\d)_CH[A-F](\d)$", "af"],
	["^HRTIM(\d)_FLT(\d)$", "af"],
	["^HRTIM(\d)_SCIN$", "af"],
	["^HRTIM(\d)_SCOUT$", "af"],
	["^HRTIM(\d)_EEV(\d\d?)$", "af"],
	["^I2C(\d)_SCL$", "af,od"],
	["^I2C(\d)_SDA$", "af,od"],
	["^I2C(\d)_SMBA$", "af,od"],
	["^I2S(\d)_CK$", "af,od"],
	["^I2S(\d)_MCK$", "af,od"],
	["^I2S(\d)_SD$", "af,od"],
	["^I2S(\d)_WS$", "af,od"],
	["^I2SCKIN$", "af"],
	["^IR_OUT$", "af"],
	["^JTCK$", "af"],
	["^JTDI$", "af"],
	["^JTDO$", "af"],
	["^JTMS$", "af"],
	["^LCD_COM(\d+)$", "af"],
	["^LCD_SEG(\d+)$", "af"],
	["^LCD_VLCD$", "af"],
	["^LPTIM(\d)_OUT$", "af"],
	["^LPTIM(\d)_IN(\d)$", "af"],
	["^LPTIM(\d)_ETR$", "af"],
	["^LPUART(\d)_CTS$", "af,od"],
	["^LPUART(\d)_RX$", "af,od"],
	["^LPUART(\d)_TX$", "af"],
	["^LPUART(\d)_DE$", "af"],
	["^LPUART(\d)_RTS$", "af"],
	["^LPUART(\d)_RTS_DE$", "af"],
	["^MCO$", "af"],
	["^N?JTRST$", "af"],
	["^OPAMP(\d)_VIN[PM]$", "an"],
	["^OPAMP(\d)_VOUT$", "an"],
	["^OTG_FS_CRS_SYNC$", "af"],
	["^OTG_FS_D[MP]$", "af"],
	["^OTG_FS_ID$", "af"],
	["^OTG_FS_NOE$", "af"],
	["^OTG_FS_SOF$", "af"],
	["^RTC_REFIN$", "af"],
	["^RTC_OUT(\d?)$", "af"],
	["^SAI(\d)_CK(\d)$", "af"],
	["^SAI(\d)_D(\d)$", "af"],
	["^SAI(\d)_EXTCLK$", "af"],
	["^SAI(\d)_FS_([AB])$", "af"],
	["^SAI(\d)_MCLK_([AB])$", "af"],
	["^SAI(\d)_SCK_([AB])$", "af"],
	["^SAI(\d)_SCLK_([AB])$", "af"],
	["^SAI(\d)_SD_([AB])$", "af"],
	["^SPI(\d)_NSS$", "af"],
	["^SPI(\d)_SCK$", "af"],
	["^SPI(\d)_MISO$", "af"],
	["^SPI(\d)_MOSI$", "af"],
	["^SDMMC(\d+)_D(\d+)$", "af"],
	["^SDMMC(\d+)_CK$", "af"],
	["^SDMMC(\d+)_CMD$", "af"],
	["^SWDIO$", "af"],
	["^SWCLK$", "af"],
	["^SWPMI(\d)_IO$", "af"],
	["^SWPMI(\d)_RX$", "af"],
	["^SWPMI(\d)_SUSPEND$", "af"],
	["^SWPMI(\d)_TX$", "af"],
	["^QUADSPI(\d?)_BK(\d)_IO(\d)$", "af"],
	["^QUADSPI(\d?)_BK(\d)_NCS$", "af"],
	["^QUADSPI(\d?)_CLK$", "af"],
	["^TIM(\d+)_BKIN(\d)?$", "af"],
	["^TIM(\d+)_BKIN(\d)?_COMP(\d)$", "af"],
	["^TIM(\d+)_CH(\d)$", "af"],
	["^TIM(\d+)_CH(\d)N$", "af"],
	["^TIM(\d+)_ETR$", "af"],
	["^TRACECL?K$", "af"],
	["^TRACED(\d)$", "af"],
	["^TRACESWO$", "af"],
	["^TSC_G(\d)_IO(\d)$", "af"],
	["^TSC_SYNC$", "af"],
	["^UCPD(\d)_FRSTX$", "af"],
	["^USB_D([MP])$", "af"],
	["^USB_NOE$", "af"],
	["^US?ART(\d)_CK$", "af"],
	["^US?ART(\d)_CTS$", "af,od"],
	["^US?ART(\d)_DE$", "af"],
	["^US?ART(\d)_RTS$", "af"],
	["^US?ART(\d)_RTS_DE$", "af"],
	["^US?ART(\d)_RX$", "af,od"],
	["^US?ART(\d)_TX$", "af"],
	["^USB_CRS_SYNC$", "af"],
]

def def_ifdef(reg_ex):
	return reg_ex.group(0) + "_BASE"

def cond_adc(reg_ex):
	inst = int(reg_ex.group(1))
	if inst >= 12:
		res = ""
		for ch in reg_ex.group(1):
			if res:
				res += '&'
			res += "ADC" + ch +  "_BASE"
		return res
	else:
		return reg_ex.group(0) + "_BASE"

def ir_ifdef(reg_ex):
	return "TIM16_BASE"

def i2s_spi_ifdef(reg_ex):
	return "SPI_I2S_SUPPORT"

def quadspi_ifdef(reg_ex):
	return "QUADSPI"

ORGANIZER = \
[
	[ "^JTD[IO]$", 		200, 		"SYS",	 None ],
	[ "^JTCK$", 		200, 		"SYS",	 None ],
	[ "^JTMS$", 		200, 		"SYS",	 None ],
	[ "^N?JTRST$", 		200, 		"SYS",	 None ],
	[ "^SWCLK$", 		300, 		"SYS",	 None ],
	[ "^SWDIO$", 		300, 		"SYS",	 None ],
	[ "^TRACECL?K$", 	200, 		"SYS",	 None ],
	[ "^TRACED(\d)$",	200, 		"SYS",	 None ],
	[ "^TRACESWO$", 	200, 		"SYS",	 None ],
	[ "^MCO$", 			100, 		"SYS",	 None ],
	[ "^ADC(\d+)$", 	1000000,	None,	 cond_adc ],
	[ "^CAN(\d)$", 		1100000, 	None,	 def_ifdef ],
	[ "^COMP(\d)$", 	1200000, 	None,	 def_ifdef ],
	[ "^DAC(\d)$", 		1300000, 	None,	 def_ifdef ],
	[ "^DFSDM(\d+)$",	1400000, 	None,	 def_ifdef ],
	[ "^DCMI$", 		1500000, 	None,	 def_ifdef ],
	[ "^FMC$", 			1600000, 	None,	 def_ifdef ],
	[ "^HRTIM(\d+)$", 	1700000, 	None,	 def_ifdef ],
	[ "^IR$", 			1800000, 	None,	 ir_ifdef ],
	[ "^I2C(\d)$", 		1900000, 	None,	 def_ifdef ],
	[ "^I2S(\d)$", 		2000000, 	None,	 def_ifdef ],
	[ "^I2SCKIN$", 		2100000, 	None,	 i2s_spi_ifdef ],
	[ "^LCD$", 			2200000, 	None,	 def_ifdef ],
	[ "^LPTIM(\d+)$", 	2300000, 	None,	 def_ifdef ],
	[ "^LPUART(\d+)$", 	2400000, 	None,	 def_ifdef ],
	[ "^OPAMP(\d+)$", 	2500000, 	None,	 def_ifdef ],
	[ "^OTG$", 			2600000, 	None,	 def_ifdef ],
	[ "^QUADSPI(\d?)$", 2700000, 	None,	 quadspi_ifdef ],
	[ "^RTC$", 			2800000, 	None,	 def_ifdef ],
	[ "^SAI(\d)$",	 	2900000, 	None,	 def_ifdef ],
	[ "^SDMMC(\d)$", 	3000000, 	None,	 def_ifdef ],
	[ "^SPI(\d)$", 		3100000, 	None,	 def_ifdef ],
	[ "^SWPMI(\d)$", 	3200000, 	None,	 def_ifdef ],
	[ "^TIM(\d+)$", 	3300000, 	None,	 def_ifdef ],
	[ "^TSC$", 			3400000, 	None,	 def_ifdef ],
	[ "^US?ART(\d+)$", 	3500000, 	None,	 def_ifdef ],
	[ "^USB$", 			3600000, 	None,	 def_ifdef ],
	[ "^UCPD(\d)$", 	3700000, 	None,	 def_ifdef ],
	[ "^EVENTOUT$", 	5000000, 	None,	 None ],
]


def align_to_col(s, cols):
	l = len(s)
	if l < cols:
		while (l < cols):
			s += '\t'
			l += 4
			l -= l % 4
	else:
		s += '\t'
	return s

class AfPinName:
	def __init__(self, dev, port, pin, af, pos, group_title, impl, ifdef):
		self.dev = dev
		toks = dev.split('_', 1)
		self.func = toks[-1]
		self.port = port
		self.pin = pin
		self.af = af
		self.pos = pos
		self.group_title = group_title
		self.impl = impl
		self.ifdef = ifdef
	def __lt__(self, o):
		if self.pos < o.pos:
			return True
		elif self.pos == o.pos:
			sport = self.port < 'PC' and 'PC' or self.port
			oport = o.port < 'PC' and 'PC' or o.port
			if sport < oport:
				return True
			elif sport == oport:
				if self.func < o.func:
					return True
				elif self.func == o.func:
					if self.port < o.port:
						return True
					elif self.port == o.port:
						if self.pin < o.pin:
							return True
						elif self.pin == o.pin:
							if self.af < o.af:
								return True
							elif self.af == o.af:
								return self.dev < o.dev
		return False
	def is_analog(self):
		return "an" in self.impl
	def get_af_name(self):
		return "Af{}_{}{}".format(self.dev, self.port, self.pin)
	def get_af_decl_line(self):
		s1 = "typedef AnyAFR<Port::{0}, {1}, AF::k{2}>".format(self.port, self.pin, self.af)
		s2 = "Af{2}_{0}{1};".format(self.port, self.pin, self.dev)
		s1 = align_to_col(s1, 40)
		return s1 + s2
	def get_type_lines(self):
		lines = []
		s1 = "/// A default configuration to map {0} on {1}{2} pin".format(self.dev.replace('_', ' ', 1), self.port, self.pin)
		lines.append(s1)
		if self.is_analog():
			s1 = "typedef AnyAnalog<Port::{0}, {1}>".format(self.port, self.pin)
			s1 = align_to_col(s1, 36)
		elif 'od' in self.impl:
			s1 = "typedef AnyAltOutOD<Port::{0}, {1}, {2}>".format(self.port, self.pin, self.get_af_name())
			s1 = align_to_col(s1, 60)
		else:
			s1 = "typedef AnyAltOut<Port::{0}, {1}, {2}>".format(self.port, self.pin, self.get_af_name())
			s1 = align_to_col(s1, 60)
		s2 = "{0}_{1}{2};".format(self.dev, self.port, self.pin)
		lines.append(s1+s2)
		return lines

def validate_header(row):
	"Table geometry is hard coded. This makes sure that rules will work."
	hdr = re.compile(r"AF(\d\d?)")
	cnt = -2
	# validate first row
	for cell in row[1:]:
		cnt += 1
		if not cell:
			if (cnt != -1) and (cnt < 16):
				raise Exception ("Cannot recognize header")
		else:
			res = hdr.match(cell)
			if not res:
				raise Exception ("expected header line should contain AF0..AF15 ")
			if int(res.group(1)) != cnt:
				raise Exception ("AF<nn> cell is not ordered")
			
def validate_row(cols):
	"Port name parsing is hard-coded. Just makes sure that no surprises will happen"
	res = re.match(r"Port([A-H])", cols[0])
	res = re.match(r"P([A-H])(\d\d?)", cols[1])

def validate_label(s):
	for rule in DATABASE:
		res = re.match(rule[0], s)
		if res:
			return rule
	return None

def organize_label(s):
	for rule in ORGANIZER:
		res = re.match(rule[0], s)
		if res:
			ifdef = None
			if rule[3]:
				ifdef = rule[3](res)
			return [rule[0], rule[1], rule[2], ifdef]
	return None

def extract_dev(s):
	if not s.startswith('TSC_'):
		res = re.match(r"^(I2[CS])(\d+)_.*$", s)
		if res:
			delta = int(res.group(2))
			return (res.group(1)+res.group(2), delta)
		res = re.match(r"^ADC(\d*)_[A-Z]+(\d*).*$", s)
		if res:
			delta = int(res.group(1))
			# handles weird numbering for ADC merged into groups
			if delta >= 300:
				delta = 9
			elif delta >= 200:
				delta = 8
			elif delta >= 100:
				delta = 7
			elif delta > 10:
				delta = 6
			#delta += int(res.group(2))
			return ('ADC'+res.group(1), delta)
		res = re.match(r"^([A-Z]+)(\d+)_.*$", s)
		if res:
			delta = int(res.group(2))
			return (res.group(1)+res.group(2), delta)
	return (s.split('_')[0], 0)

def load():
	all_pins = []
	with open(FILE, 'r') as csv_file:
		reader = csv.reader(csv_file)
		for row in reader:
			if row[0] == "Port":
				validate_header(row)
			elif (not row[0]):
				continue;
			else:
				validate_row(row)
				port = row[1][:2]
				pin = int(row[1][2:])
				for af, val in enumerate(row[2:]):
					if not val:
						continue
					toks = val.split('/')
					for dev in toks:
						# Label is recognized?
						ctrl = validate_label(dev)
						if not ctrl:
							msg = "Label {0} cannot be recognized or database needs update".format(dev)
							print (msg)
							raise Exception(msg)
						# Decompose device name from label
						ids, ord_low = extract_dev(dev)
						# Rule to sort device
						ord = organize_label(ids)
						if not ord:
							msg = "Device {0} cannot be organized or database needs update".format(ids)
							print (msg)
							raise Exception(msg)
						pos = ord[1] + ord_low
						group_title = ord[2] or ids
						ifdef = ord[3]
						afp = AfPinName(dev, port, pin, af, pos, group_title, ctrl[1].split(','), ifdef)
						all_pins.append(afp)
	return all_pins


class IfDefManager:
	def __init__(self):
		self.last = None
		self.curr = None
		self.do_line = True
	def next_ifdef(self, value):
		"Starts the next #ifdef cycle"
		self.last = self.curr
		self.curr = value
	def do_end_block(self, fh):
		"Write #endif on transition"
		if self.last != self.curr:
			if self.last:
				fh.write("#endif\t// {}\n".format(self.mk_ifdef(self.last)))
	def do_start_block(self, fh):
		"Write #if on transition"
		if self.last != self.curr:
			if self.curr:
				fh.write("#if {}\n".format(self.mk_ifdef(self.curr)))
	@staticmethod
	def mk_ifdef(str):
		res = ""
		if '&' in str:
			for t in str.split('&'):
				if res:
					res += " && "
				res += "defined(" + t + ")"
		else:
			for t in str.split('|'):
				if res:
					res += " || "
				res += "defined(" + t + ")"
		return res
	
class IfDefPortManager(IfDefManager):
	def __init__(self):
		super(IfDefPortManager, self).__init__()
	def next_ifdef(self, port):
		if (not port) or (port <= 'PC'):
			port = None
		else:
			port = "GPIO" + port[1] + "_BASE"
		super().next_ifdef(port)

def create_pinremap(all_pins):
	ifdefm = IfDefManager()
	portm = IfDefPortManager()
	last_title = ""
	with open(OUTF, 'wt') as fh:
		fh.write("#pragma once\n")
		fh.write("\n")
		fh.write("namespace Bmt\n")
		fh.write("{\n")
		fh.write("namespace Gpio\n")
		fh.write("{\n")
		fh.write("\n")
		for afp in all_pins:
			# Analog channels does not have AF mapping
			if afp.is_analog():
				continue
			do_line = True
			portm.next_ifdef(afp.port)
			portm.do_end_block(fh)
			ifdefm.next_ifdef(afp.ifdef)
			ifdefm.do_end_block(fh)
			if last_title != afp.group_title:
				last_title = afp.group_title
				print (last_title)
				if do_line:
					fh.write("\n")
				fh.write("// {}\n".format(last_title))
			ifdefm.do_start_block(fh)
			portm.do_start_block(fh)
			fh.write(afp.get_af_decl_line() + '\n')
		portm.next_ifdef(None)
		portm.do_end_block(fh)
		ifdefm.next_ifdef(None)
		ifdefm.do_end_block(fh)
		fh.write("\n")
		fh.write("\n")
		fh.write("}\t// namespace Gpio\n")
		fh.write("}\t// namespace Bmt\n")
		fh.write("\n")

def create_types(all_pins):
	ifdefm = IfDefManager()
	portm = IfDefPortManager()
	last_title = ""
	with open(OUTF2, 'wt') as fh:
		fh.write("#pragma once\n")
		fh.write("\n")
		fh.write("namespace Bmt\n")
		fh.write("{\n")
		fh.write("namespace Gpio\n")
		fh.write("{\n")
		fh.write("\n")
		for afp in all_pins:
			portm.next_ifdef(afp.port)
			portm.do_end_block(fh)
			ifdefm.next_ifdef(afp.ifdef)
			ifdefm.do_end_block(fh)
			if last_title != afp.group_title:
				last_title = afp.group_title
				print (last_title)
				fh.write("\n")
				fh.write("//////////////////////////////////////////////////////////////////////\n")
				fh.write("// {}\n".format(last_title))
				fh.write("//////////////////////////////////////////////////////////////////////\n")
			ifdefm.do_start_block(fh)
			portm.do_start_block(fh)
			lines = afp.get_type_lines()
			for line in lines:
				fh.write(line + '\n')
		portm.next_ifdef(None)
		portm.do_end_block(fh)
		ifdefm.next_ifdef(None)
		ifdefm.do_end_block(fh)
		fh.write("\n")
		fh.write("\n")
		fh.write("}\t// namespace Gpio\n")
		fh.write("}\t// namespace Bmt\n")
		fh.write("\n")

all_pins = load()
all_pins.sort()

create_pinremap(all_pins)
create_types(all_pins)

