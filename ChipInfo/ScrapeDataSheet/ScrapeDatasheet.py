#!py -3

# pip install pdfplumber
import io
import pdfplumber
import copy

OUTF = r"D:\baba.txt"
INF = r"D:\[CDCache]\[Electronics]\[Embedded]\[µC]\Texas Instruments\MSP430\DataSheet\MSP430x2xx Family\slas439f - Datasheet - MSP430F21x1.pdf"


def GetParameter(name, toks_p, max_cnt = None):
	toks = copy.copy(toks_p)
	# convert to tokens
	keys = name.lower().split()
	# Match beginning
	if max_cnt is None:
		max_cnt = len(keys)
	cnt = 0
	# Find first match (this is a table!)
	while (len(toks) > len(keys)) and (toks[0] != keys[0]):
		cnt += 1
		if cnt > max_cnt:
			return None
		toks.pop(0)
	if len(toks) <= len(keys):
		return None
	# match all other tokens
	while len(keys):
		if toks[0] != keys[0]:
			return None
		toks.pop(0)
		keys.pop(0)
	# if here, toks match the key, continue columns until value is found
	res = []
	while len(toks):
		if toks[0].isdigit():
			res.append(int(toks[0]))
		toks.pop(0)
	return len(res) and res or None

def SearchFlash(it):
	match = False
	for i in range(3):
		line = next(it).strip()
		if 'PARAMETER' in line \
			and 'TEST CONDITIONS' in line:
			match = True
			break
	if not match:
		return None
	freq = None
	word = None
	block0 = None
	block1 = None
	block2 = None
	mass = None
	segment = None
	for i in range(15):
		toks = next(it).lower().split()
		# skip blank lines
		if not toks:
			continue
		nomore = True
		# Flash timing generator frequency
		if freq is None:
			nomore = False
			val = GetParameter("fFTG", toks)
			if val is not None:
				freq = val
				continue
		# Word or byte program time
		if word is None:
			nomore = False
			val = GetParameter("tword", toks)
			if val is not None:
				word = val
				continue
		# Block program time for first byte or word
		if block0 is None:
			nomore = False
			val = GetParameter("tblock, 0", toks)
			if val is not None:
				block0 = val
				continue
		# Block program time for each additional byte or word 
		if block1 is None:
			nomore = False
			val = GetParameter("tblock, 1-63", toks)
			if val is not None:
				block1 = val
				continue
		# Block program end-sequence wait time 
		if block2 is None:
			nomore = False
			val = GetParameter("tblock, End", toks)
			if val is not None:
				block2 = val
				continue
		# Mass erase time
		if mass is None:
			nomore = False
			val = GetParameter("tmass erase", toks)
			if val is not None:
				mass = val
				continue
		# Segment erase time
		if segment is None:
			nomore = False
			val = GetParameter("tseg erase", toks)
			if val is not None:
				segment = val
				continue
		#TODO more here if desired
		if nomore:
			break
	return (freq, word, block0, block1, mass, segment)

def ProcessPdf(fname):
	pdf = pdfplumber.open(fname)
	for page in pdf.pages:
		text = page.extract_text(x_tolerance=2, y_tolerance=4, layout=True)
		it = iter(text.splitlines())
		#TODO: Iterate header lines to locate SLAS code and Chip model
		for line in it:
			line = line.strip()
			if line == "Flash Memory":
				res = SearchFlash(it)
				if res:
					return


ProcessPdf(INF)
