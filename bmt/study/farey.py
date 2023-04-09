
import math

BRUTE_FORCE = 1
STMF103 = 1
MHZ = 1000000
XTAL = 8000000

if STMF103:
	SCAN = (1,)
else:
	SCAN = (2, 4, 6, 8)

class PllLimits:
	def __init__(self):
		if STMF103:
			self.fin_low = 1000000
			self.fin_hi = 25000000
			self.vco_low = 16000000
			self.vco_hi = 72000000
			self.n_low = 2
			self.n_hi = 16
			self.m_low = 1
			self.m_hi = 2
		else:
			self.fin_low = 4000000
			self.fin_hi = 16000000
			self.vco_low = 64000000
			self.vco_hi = 344000000
			self.n_low = 8
			self.n_hi = 86
			self.m_low = 1
			self.m_hi = 8


def farey(x, limits):
	"Original algorithm, adapted to a given multiplier range"
	m1, q1 = 0, 1
	err1 = 2 * math.fabs(x - 0.0) / x
	if limits.n_low > 1:
		m2, q2 = 1, limits.n_low - 1
		mediant = float(m2)/q2
		err2 = 2.0
	else:
		m2, q2 = 1, 1
		err2 = 2 * math.fabs(x - 1.0) / (x + 1.0)
		if err2 == 0.0:
			return m2, q2, 0.0
	while ((q1 < limits.n_hi) and (q2 < limits.n_hi) and (m1 < limits.m_hi) and (m2 < limits.m_hi)):
		mediant = float(m1+m2)/(q1+q2)
		err = 2 * math.fabs(x - mediant) / (x + mediant)
		if err == 0.0:
			if q1 + q2 <= limits.n_hi:
				return m1+m2, q1+q2, err
			elif q2 > q1:
				return m2, q2, err2
			else:
				return m1, q1, err2
		elif x > mediant:
			m1, q1 = m1+m2, q1+q2
			err1 = err
		else:
			m2, q2 = m1+m2, q1+q2
			err2 = err

	if ((q1 > limits.n_hi) or (m1 > limits.m_hi)):
		return m2, q2, err2
	else:
		return m1, q1, err1
	
def brute_force(x, clk, limits:PllLimits):
	"""
	Farey algorithm fails badly in a constrained PLL, like on the STM32F103, which 
	has only two values for '/M'.
	So, before using Farley, we just scan the lowest border of the PLL with these 
	possibilities
	"""
	m_o = 1.0
	n_o = 0.0
	err_o = math.inf
	n = limits.n_low
	while (err_o != 0.0) and (n <= limits.n_hi):
		m = limits.m_low
		while (err_o != 0.0) and (m <= limits.m_hi):
			f_in = clk / m
			if (f_in >= limits.fin_low) and (f_in <= limits.fin_hi):
				f_out = f_in * n
				if (f_out >= limits.vco_low) and (f_out <= limits.vco_hi):
					value = n / m
					err = math.fabs(x - value)
					if err < err_o:
						err_o = err
						n_o = n
						m_o = m
			m += 1
		n += 1
	return n_o, m_o, err_o / x


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
	for _R in SCAN:
		# The 'freq' does not exists in the PLL hardware, but Math allows us to 
		# freely swap '/R' and '/M' terms without affecting the equations results, 
		# so that computed values are correct. So ignore circuit on the datasheet
		# and trust in math equivalence property.
		freq = clk / _R

		# IMPORTANT IMPLEMENTATION DETAIL:
		# https://en.wikipedia.org/wiki/Farey_sequence
		# The Farey algorithm requires a value within the range [0.0 ... 1.0] to 
		# converge to an equivalent fraction. A PLL will always increase a frequency. 
		# For a series of technical constraints the 'xN / _M' ratio shall be >= 1.0
		# So the trick for the Farey algorithm approximation is to use the '_M / xN'
		# ratio (i.e. the inverse) to compute 'xN' and '/M'.
		# Similarly, the resulting fraction is reversed to produce the desired 
		# result.
		x = freq / vco	# inverse as explained above

		# Compute fraction and reverse terms
		if BRUTE_FORCE:
			x = vco / freq
			xN, _M, err = brute_force(x, clk, limits)
		else:
			x = freq / vco	# inverse as explained above
			_M, xN, err = farey(x, limits)

		err = err * 100.0

		if (_M == 0) or (err == math.inf):
			# Result is too low and a division by 0 is not allowed
			print ("No approximation was possible with /R=" + str(_R))
		else:
			# Produce the status string
			status = ""
			if (xN < limits.n_low):
				status += "\txN < {0}".format(limits.n_low)
			elif (xN > limits.n_hi):
				status += "\txN > {0}".format(limits.n_hi)
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
			print("\t{5:5.3f} MHz / (M:={0}) * (N:={1}) / (R:={2})\t= {3:5.3f} MHz (err={6:10.7f}){4}".format(_M, xN, _R, math.trunc(vco_out / _R)/MHZ, status, clk/MHZ, err))


if 1:
	freq = XTAL
	while freq < 81000000:
		print(freq)
		clksel(freq, XTAL)
		freq += 1000000
else:
	clksel(8000000, XTAL)

