import os
import sys
import ezodf
import re

DATABASE = \
[
	["^COMP(\d)_OUT$", "o"],
	["^EVENTOUT$", "o"],
	["^FDCAN(\d)_RX$", "i"],
	["^FDCAN(\d)_TX$", "o"],
	["^I2C(\d)_SCL$", "o"],
	["^I2C(\d)_SDA$", "o"],
	["^I2C(\d)_SMBA$", "i"],
	["^I2S(\d)_CK$", "o"],
	["^I2S(\d)_MCK$", "o"],
	["^I2S(\d)_SD$", "o"],
	["^I2S(\d)_WS$", "o"],
	["^I2SCKIN$", "i"],
	["^IR_OUT$", "o"],
	["^JTCK$", "i"],
	["^JTDI$", "i"],
	["^JTMS$", "i"],
	["^LPTIM(\d)_OUT$", "o"],
	["^LPTIM(\d)_IN(\d)$", "i"],
	["^LPTIM(\d)_ETR$", "i"],
	["^LPUART(\d)_CTS$", "i"],
	["^LPUART(\d)_RX$", "i"],
	["^LPUART(\d)_TX$", "o"],
	["^LPUART(\d)_RTS_DE$", "o"],
	["^MCO$", "o"],
	["^RTC_REFIN$", "i"],
	["^SAI(\d)_CK(\d)$", "o"],
	["^SAI(\d)_D(\d)$", "o"],
	["^SAI(\d)_FS_([AB])$", "o"],
	["^SAI(\d)_MCLK_([AB])$", "o"],
	["^SAI(\d)_SCK_([AB])$", "o"],
	["^SAI(\d)_SD_([AB])$", "o"],
	["^SPI(\d)_NSS$", "io"],
	["^SPI(\d)_SCK$", "io"],
	["^SPI(\d)_MISO$", "io"],
	["^SPI(\d)_MOSI$", "io"],
	["^SWDIO$", "o"],
	["^SWCLK$", "i"],
	["^TIM(\d\d?)_BKIN(\d)?$", "i"],
	["^TIM(\d\d?)_CH(\d)$", "io"],
	["^TIM(\d\d?)_CH(\d)N$", "o"],
	["^TIM(\d\d?)_ETR$", "i"],
	["^UCPD(\d)_FRSTX$", "o"],
	["^USART(\d)_CK$", "o"],
	["^USART(\d)_CTS$", "i"],
	["^UART(\d)_CTS$", "i"],
	["^USART(\d)_RTS_DE$", "o"],
	["^USART(\d)_RX$", "i"],
	["^USART(\d)_TX$", "o"],
	["^USB_CRS_SYNC$", "i"],
]

def validate_header(table):
	hdr = re.compile(r"AF(\d\d?)")
	cnt = -2
	# validate first row
	for cell in table.row(0):
		cnt += 1
		if (cell.value is None):
			if (cnt != -1):
				raise Exception ("Top/Left cell should be empty")
		else:
			res = hdr.match(cell.value)
			if not res:
				raise Exception ("expected header line should contain AF0..AF15 ")
			if int(res.group(1)) != cnt:
				raise Exception ("AF<nn> cell is not ordered")
			
def validate_row(cols):
	port = re.compile(r"P([A-H])(\d\d?)")
	res = port.match(cols[0].value)

def validate_label(s):
	for rule in DATABASE:
		res = re.match(rule[0], s)
		if res:
			return rule
	return None

def load():
	map = {}
	spreadsheet = ezodf.opendoc("pinmap.g4.ods")
	table = spreadsheet.sheets['Sheet1']
	validate_header(table)
	rowcount = table.nrows()
	colcount = table.ncols()
	for i in range(0, rowcount):
		cols = table.row(i)
		if cols[0].value is None:
			continue
		validate_row(cols)
		for j in range(1, colcount):
			val = cols[j].value
			if val is None:
				continue
			toks = val.split('/')
			for t in toks:
				if not validate_label(t):
					msg = "Label {0} cannot be recognized or database needs update".format(t)
					raise Exception(msg)
				if t not in map:
					map[t] = [[cols[0].value, j-1]]
				else:
					col = map[t]
					col.append([cols[0].value, j-1])
					map[t] = col
	return map

map = load()
keys = list(map)
keys.sort()
for k in keys:
	print (k, map[k])
