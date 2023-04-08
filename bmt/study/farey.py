
import math

MHZ = 1000000
XTAL = 8000000

class PllLimits:
	def __init__(self):
		self.fin_low = 4000000
		self.fin_hi = 16000000
		self.vco_low = 64000000
		self.vco_hi = 344000000


def farey(x, N0, N1):
	"Original algorithm, adapted to a given multiplier range"
	if N0 > 1:
		N0 -= 1
	a, b = 0, 1
	c, d = 1, N0
	while (b < N1 and d < N1):
		mediant = float(a+c)/(b+d)
		if x == mediant:
			if b + d <= N1:
				return a+c, b+d
			elif d > b:
				return c, d
			else:
				return a, b
		elif x > mediant:
			a, b = a+c, b+d
		else:
			c, d = a+c, b+d

	if (b > N1):
		return c, d
	else:
		return a, b

def farey2(x, N0, N1):
	"With simple error control"
	if N0 > 1:
		N0 -= 1
	a, b = 0, 1
	c, d = 1, N0
	err = x / 1000
	while (b < N1 and d < N1):
		mediant = float(a+c)/(b+d)
		if math.fabs(x - mediant) <= err:
			if b + d <= N1:
				return a+c, b+d
			elif d > b:
				return c, d
			else:
				return a, b
		elif x > mediant:
			a, b = a+c, b+d
		else:
			c, d = a+c, b+d

	if (b > N1):
		return c, d
	else:
		return a, b

def farey3(x, N0, N1):
	"""
	With selective error control.
	The idea behind is simple: algorithm tries a new approach which is theoretically 
	better, but since the range of the quotient is limited by a given value it will 
	stop before converging. A previous value with less error is then the best result.
	"""
	if N0 > 1:
		N0 -= 1
	m1, q1 = 0, 1
	m0, q0 = 0, 1
	m2, q2 = 1, N0
	err_o = 1.0
	while (q1 < N1 and q2 < N1):
		mediant = float(m1+m2)/(q1+q2)
		err = math.fabs(x - mediant)
		if err == 0:
			if q1 + q2 <= N1:
				return m1+m2, q1+q2
			elif q2 > q1:
				return m2, q2
			else:
				return m1, q1
		else:
			# Store candidate values while converging and inside bounds
			if (err < err_o) and (q1 + q2 <= N1):
				err_o = err
				m0, q0 = m1+m2, q1+q2
			if x > mediant:
				m1, q1 = m1+m2, q1+q2
			else:
				m2, q2 = m1+m2, q1+q2
	if (q1 > N1):
		m, q = m2, q2
	else:
		m, q = m1, q1
	# Update error, since previous value was calculated for something out of the desired range
	mediant = float(m)/q
	err = math.fabs(x - mediant)
	if err_o < err:
		# old approach was better than new, because new approach requires
		# a quotient outside of proposed bounds
		return m0, q0
	return m, q

def clksel(vco, clk):
	"""
	Formula for the PLL:
		VCO(out) = ( (CLKIN / M) * N ) / R
	Where:
		VCO(out)	: Typically the PLLCLK that drives CPU. It also drives 
					  PLL48M1CLK and PLLSAI2CLK.
		CLKIN		: This is selectable between MSI, HSI16 or HSE
		M			: Input divisor for the PLL. It is important the the clock 
					  runs in the range on 4 to 16 MHz for correct operation.
					  In the datasheet this is labeled as '/M'; in this source 
					  code '_M'.
		N			: VCO multiplier that produces a high frequency. For the 
					  correct operation, datasheet states an operating range 
					  between 64 and 344 Mhz. In the datasheet and in this 
					  source code are labeled as 'xN'.
		R			: Output divisor for the PLLCLK. Hardware also offers the 
					  /P and /Q divisors with the same functionality. These
					  have to be set as not to exceed 80 MHz. In the datasheet 
					  this is labeled '/R', which is equivalent to '_R' in the 
					  source code.
	
	This routine will try to compute '/M' and 'xN' terms to achieve the best 
	approximation to a given target frequency.
	The algorithm tries all possible '/R' values. At the end each computed 
	possibility is printed out with an addition range check to disqualify 
	unusable configurations.
	"""
	limits = PllLimits()
	N0 = 8		# Minimum PLL multiplier
	N1 = 86 	# Maximum PLL multiplier
	for _R in (2, 4, 6, 8):
		# The 'freq' does not exists in the PLL hardware, but Math allows us to 
		# freely swap '/R' and '/M' terms without affecting the equations results, 
		# so that computed values are correct. So ignore circuit on the datasheet
		# and trust in math equivalence property.
		freq = clk / _R

		# IMPORTANT IMPLEMENTATION DETAIL:
		# https://en.wikipedia.org/wiki/Farey_sequence
		# The farey algorithm requires a value within the range [0.0 ... 1.0] to 
		# converge to an equivalent fraction. A PLL will always increase a frequency. 
		# For a series of technical constraints the 'xN / _M' ratio shall be >= 1.0
		# So the trick for the farey algorithm approximation is to use the '_M / xN'
		# ratio (i.e. the inverse) to compute 'xN' and '/M'.
		# Similarly, the resulting fraction is reversed to produce the desired 
		# result.
		x = freq / vco	# inverse as explained above

		# Compute fraction and reverse terms
		_M, xN = farey2(x, N0, N1)

		if _M == 0:
			# Result is too low and a division by 0 is not allowed
			print ("No approximation was possible with /R=" + str(_R))
		else:
			# Produce the status string
			status = ""
			if (xN < N0):
				status += "\txN < {0}".format(N0)
			elif (xN > N1):
				status += "\txN > {0}".format(N1)
			clki = clk / _M
			if clki < limits.fin_low:
				status += "\tVCO(in) < {0:.0f} MHz".format(limits.fin_low/MHZ)
			elif clki > limits.fin_hi:
				status += "\tVCO(in) > {0:.0f} MHz".format(limits.fin_hi/MHZ)
			vco_out = clki * xN
			if vco_out < limits.vco_low:
				status += "\tVCO < {0:.0f} MHz".format(limits.vco_low/MHZ)
			elif vco_out > limits.vco_hi:
				status += "\tVCO > {0:.0f} MHz".format(limits.vco_hi/MHZ)
			if (_M > 8):
				status += "\t/M > 8"
			# Reformat status string before producing an output
			if status:
				status = '\t' + status[1:].replace('\t', '; ')
			# Display result and status
			print("\t{5:5.3f} MHz / (M:={0}) * (N:={1}) / (R:={2})\t= {3:5.3f} MHz{4}".format(_M, xN, _R, math.trunc(vco_out / _R)/MHZ, status, clk/MHZ))


if 1:
	freq = XTAL
	while freq < 81000000:
		print(freq)
		clksel(freq, XTAL)
		freq += 1000000
else:
	clksel(8000000, XTAL)

