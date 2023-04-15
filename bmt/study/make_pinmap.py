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

FILE = "stm32l432_af.csv"
OUTF = "pinremap.l4.h"

DATABASE = \
[
	["^ADC(\d)_IN(\d\d?)$", "an"],
	["^CAN(\d)_RX$", "af"],
	["^CAN(\d)_TX$", "af"],
	["^COMP(\d)_OUT$", "af"],
	["^COMP(\d)_IN([MP])$", "an"],
	["^DAC(\d)_OUT(\d)$", "an"],
	["^EVENTOUT$", "af"],
	["^FDCAN(\d)_RX$", "af"],
	["^FDCAN(\d)_TX$", "af"],
	["^I2C(\d)_SCL$", "af"],
	["^I2C(\d)_SDA$", "af"],
	["^I2C(\d)_SMBA$", "af"],
	["^I2S(\d)_CK$", "af"],
	["^I2S(\d)_MCK$", "af"],
	["^I2S(\d)_SD$", "af"],
	["^I2S(\d)_WS$", "af"],
	["^I2SCKIN$", "af"],
	["^IR_OUT$", "af"],
	["^JTCK$", "af"],
	["^JTDI$", "af"],
	["^JTDO$", "af"],
	["^JTMS$", "af"],
	["^LPTIM(\d)_OUT$", "af"],
	["^LPTIM(\d)_IN(\d)$", "af"],
	["^LPTIM(\d)_ETR$", "af"],
	["^LPUART(\d)_CTS$", "af"],
	["^LPUART(\d)_RX$", "af"],
	["^LPUART(\d)_TX$", "af"],
	["^LPUART(\d)_DE$", "af"],
	["^LPUART(\d)_RTS$", "af"],
	["^LPUART(\d)_RTS_DE$", "af"],
	["^MCO$", "af"],
	["^NJTRST$", "af"],
	["^RTC_REFIN$", "af"],
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
	["^SWDIO$", "af"],
	["^SWCLK$", "af"],
	["^SWPMI(\d)_IO$", "af"],
	["^SWPMI(\d)_RX$", "af"],
	["^SWPMI(\d)_SUSPEND$", "af"],
	["^SWPMI(\d)_TX$", "af"],
	["^QUADSPI_BK1_IO(\d)$", "af"],
	["^QUADSPI_BK1_NCS$", "af"],
	["^QUADSPI_CLK$", "af"],
	["^TIM(\d\d?)_BKIN(\d)?$", "af"],
	["^TIM(\d\d?)_BKIN(\d)?_COMP(\d)$", "af"],
	["^TIM(\d\d?)_CH(\d)$", "af"],
	["^TIM(\d\d?)_CH(\d)N$", "af"],
	["^TIM(\d\d?)_ETR$", "af"],
	["^TRACESWO$", "af"],
	["^TSC_G(\d)_IO(\d)$", "af"],
	["^UCPD(\d)_FRSTX$", "af"],
	["^USB_D([MP])$", "af"],
	["^USB_NOE$", "af"],
	["^USART(\d)_CK$", "af"],
	["^USART(\d)_CTS$", "af"],
	["^UART(\d)_CTS$", "af"],
	["^USART(\d)_DE$", "af"],
	["^USART(\d)_RTS$", "af"],
	["^USART(\d)_RTS_DE$", "af"],
	["^USART(\d)_RX$", "af"],
	["^USART(\d)_TX$", "af"],
	["^USB_CRS_SYNC$", "af"],
]

ORGANIZER = \
[
	[ "^JTC[KIOS]$", 100, "SYS" ],
	[ "^JTD[IO]$", 100, "SYS" ],
	[ "^JTMS$", 100, "SYS" ],
	[ "^NJTRST$", 100, "SYS" ],
	[ "^SWCLK$", 100, "SYS" ],
	[ "^SWDIO$", 100, "SYS" ],
	[ "^TRACESWO$", 100, "SYS" ],
	[ "^MCO$", 100, "SYS" ],
	[ "^ADC(\d)$", 10000 ],
	[ "^DAC(\d)$", 11000 ],
	[ "^TIM(\d\d?)$", 12000 ],
	[ "^LPTIM(\d\d?)$", 13000 ],
	[ "^IR$", 14000 ],
	[ "^I2C(\d)$", 15000 ],
	[ "^SPI(\d)$", 16000 ],
	[ "^US?ART(\d\d?)$", 17000 ],
	[ "^LPUART(\d\d?)$", 18000 ],
	[ "^CAN(\d)$", 19000 ],
	[ "^TSC$", 20000 ],
	[ "^USB$", 21000 ],
	[ "^QUADSPI$", 22000 ],
	[ "^COMP(\d)$", 23000 ],
	[ "^SWPMI(\d)$", 24000 ],
	[ "^SAI(\d)$", 25000 ],
	[ "^EVENTOUT$", 50000 ],
]


class AfPinName:
	def __init__(self, dev, port, pin, af, pos, group_title):
		self.dev = dev
		self.port = port
		self.pin = pin
		self.af = af
		self.pos = pos
		self.group_title = group_title
	def __lt__(self, o):
		if self.pos < o.pos:
			return True
		elif self.pos == o.pos:
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
		return self.af > 15
	def GetAfName(self):
		return "Af{}_{}{}".format(self.dev, self.port, self.pin)
	def GetAfDecl(self):
		s1 = "typedef AnyAFR<Port::{0}, {1}, AF::k{2}>".format(self.port, self.pin, self.af)
		s2 = "Af{2}_{0}{1};".format(self.port, self.pin, self.dev)
		l = len(s1)
		cols = 40
		if l < cols:
			while (l < cols):
				s1 += '\t'
				l += 4
				l -= l % 4
		else:
			s1 == '\t'
		return s1 + s2


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
			return rule
	return None

def extract_dev(s):
	if not s.startswith('TSC_'):
		res = re.match(r"^([A-Z_]{2,7})(\d\d?)_.*$", s)
		if res:
			return (res.group(1)+res.group(2), int(res.group(2)))
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
						group_title = (len(ord) > 2) and ord[2] or ids
						afp = AfPinName(dev, port, pin, af, pos, group_title)
						all_pins.append(afp)
	return all_pins


def create_pinremap(all_pins):
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
			if last_title != afp.group_title:
				last_title = afp.group_title
				print (last_title)
				fh.write("\n")
				fh.write("// {}\n".format(last_title))
			fh.write(afp.GetAfDecl() + '\n')
		fh.write("\n")
		fh.write("\n")
		fh.write("}\t// namespace Gpio\n")
		fh.write("}\t// namespace Bmt\n")
		fh.write("\n")

all_pins = load()
all_pins.sort()

create_pinremap(all_pins)

