EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "MSPBMP Development Protoboard"
Date "2021-12-04"
Rev "2"
Comp "Mathias Gruber"
Comment1 "With optional TRACE SWO using FT232RL"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L YAAJ_BluePill_Part_Like:YAAJ_BluePill_Part_Like U2
U 1 1 61865053
P 5700 3750
F 0 "U2" H 5700 4915 50  0000 C CNN
F 1 "STM32F103 BluePill" H 5700 4824 50  0000 C CNN
F 2 "lib:YAAJ_BluePill_1" H 6400 2750 50  0001 C CNN
F 3 "" H 6400 2750 50  0001 C CNN
	1    5700 3750
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR015
U 1 1 6186F99F
P 9200 3050
F 0 "#PWR015" H 9200 2900 50  0001 C CNN
F 1 "+3V3" H 9215 3223 50  0000 C CNN
F 2 "" H 9200 3050 50  0001 C CNN
F 3 "" H 9200 3050 50  0001 C CNN
	1    9200 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	9100 3450 9200 3450
Wire Wire Line
	9200 3450 9200 3100
NoConn ~ 9100 3550
NoConn ~ 9100 3650
NoConn ~ 9100 3850
$Comp
L power:GND #PWR014
U 1 1 61871790
P 8350 4300
F 0 "#PWR014" H 8350 4050 50  0001 C CNN
F 1 "GND" H 8355 4127 50  0000 C CNN
F 2 "" H 8350 4300 50  0001 C CNN
F 3 "" H 8350 4300 50  0001 C CNN
	1    8350 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	8600 3850 8350 3850
Wire Wire Line
	8350 3850 8350 4050
$Comp
L power:+3V3 #PWR03
U 1 1 61872A93
P 4400 4650
F 0 "#PWR03" H 4400 4500 50  0001 C CNN
F 1 "+3V3" H 4415 4823 50  0000 C CNN
F 2 "" H 4400 4650 50  0001 C CNN
F 3 "" H 4400 4650 50  0001 C CNN
	1    4400 4650
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 4750 4400 4750
Wire Wire Line
	4400 4750 4400 4650
$Comp
L Device:R R5
U 1 1 61874137
P 8000 3650
F 0 "R5" V 7900 3550 50  0000 C CNN
F 1 "330" V 7900 3750 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7930 3650 50  0001 C CNN
F 3 "~" H 8000 3650 50  0001 C CNN
	1    8000 3650
	0    1    1    0   
$EndComp
Wire Wire Line
	8600 3650 8150 3650
Text Label 8400 3650 0    50   ~ 0
JTMS
Wire Wire Line
	7850 3650 7600 3650
Text Label 4650 4250 0    50   ~ 0
BTMS
$Comp
L Device:R R6
U 1 1 61876DB6
P 8000 3850
F 0 "R6" V 7900 3750 50  0000 C CNN
F 1 "330" V 7900 3950 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7930 3850 50  0001 C CNN
F 3 "~" H 8000 3850 50  0001 C CNN
	1    8000 3850
	0    1    1    0   
$EndComp
Wire Wire Line
	8600 3750 8250 3750
Wire Wire Line
	8250 3750 8250 3850
Wire Wire Line
	8250 3850 8150 3850
$Comp
L Device:R R4
U 1 1 61879062
P 8000 3450
F 0 "R4" V 7900 3350 50  0000 C CNN
F 1 "330" V 7900 3550 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7930 3450 50  0001 C CNN
F 3 "~" H 8000 3450 50  0001 C CNN
	1    8000 3450
	0    1    1    0   
$EndComp
Wire Wire Line
	8600 3550 8250 3550
Wire Wire Line
	8250 3550 8250 3450
Wire Wire Line
	8250 3450 8150 3450
Wire Wire Line
	8600 3450 8350 3450
Text Label 8400 3450 0    50   ~ 0
JTDO
Text Label 8400 3550 0    50   ~ 0
JTDI
Text Label 8400 3750 0    50   ~ 0
JTCK
Text Label 4650 2950 0    50   ~ 0
BTCK
Text Label 4650 3050 0    50   ~ 0
JTDO
Wire Wire Line
	7600 3450 7800 3450
Text Label 4650 3150 0    50   ~ 0
BTDI
$Comp
L Device:R R7
U 1 1 6187BD4B
P 8000 4050
F 0 "R7" V 7900 3950 50  0000 C CNN
F 1 "330" V 7900 4150 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7930 4050 50  0001 C CNN
F 3 "~" H 8000 4050 50  0001 C CNN
	1    8000 4050
	0    1    1    0   
$EndComp
Wire Wire Line
	8600 3950 8250 3950
Wire Wire Line
	8250 3950 8250 4050
Wire Wire Line
	8250 4050 8150 4050
Text Label 8400 3950 0    50   ~ 0
JRST
Text Label 4650 2850 0    50   ~ 0
BRST
Wire Wire Line
	7850 4050 7600 4050
Text Label 7600 4050 0    50   ~ 0
BRST
$Comp
L Connector_Generic:Conn_01x04 J4
U 1 1 6187FB53
P 3750 2500
F 0 "J4" H 3830 2542 50  0000 L CNN
F 1 "GDB_COMM" H 3830 2451 50  0000 L CNN
F 2 "Connector_Molex:Molex_KK-254_AE-6410-04A_1x04_P2.54mm_Vertical" H 3750 2500 50  0001 C CNN
F 3 "~" H 3750 2500 50  0001 C CNN
	1    3750 2500
	-1   0    0    -1  
$EndComp
$Comp
L power:GND #PWR02
U 1 1 61882731
P 4300 2800
F 0 "#PWR02" H 4300 2550 50  0001 C CNN
F 1 "GND" H 4305 2627 50  0000 C CNN
F 2 "" H 4300 2800 50  0001 C CNN
F 3 "" H 4300 2800 50  0001 C CNN
	1    4300 2800
	-1   0    0    -1  
$EndComp
$Comp
L power:GND #PWR05
U 1 1 618834F3
P 4800 4850
F 0 "#PWR05" H 4800 4600 50  0001 C CNN
F 1 "GND" H 4805 4677 50  0000 C CNN
F 2 "" H 4800 4850 50  0001 C CNN
F 3 "" H 4800 4850 50  0001 C CNN
	1    4800 4850
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 4650 4800 4650
Wire Wire Line
	4800 4650 4800 4850
$Comp
L power:+3V3 #PWR08
U 1 1 618847E3
P 6600 2550
F 0 "#PWR08" H 6600 2400 50  0001 C CNN
F 1 "+3V3" H 6615 2723 50  0000 C CNN
F 2 "" H 6600 2550 50  0001 C CNN
F 3 "" H 6600 2550 50  0001 C CNN
	1    6600 2550
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 3850 4500 3850
Text Label 4500 3850 0    50   ~ 0
TRACESWO
Wire Wire Line
	6500 3050 6600 3050
$Comp
L power:GND #PWR010
U 1 1 61891E0C
P 6950 3150
F 0 "#PWR010" H 6950 2900 50  0001 C CNN
F 1 "GND" H 6955 2977 50  0000 C CNN
F 2 "" H 6950 3150 50  0001 C CNN
F 3 "" H 6950 3150 50  0001 C CNN
	1    6950 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	6500 2850 6950 2850
Wire Wire Line
	6950 2850 6950 2950
Wire Wire Line
	6500 2950 6950 2950
Connection ~ 6950 2950
Wire Wire Line
	6950 2950 6950 3150
$Comp
L Device:R R8
U 1 1 618966E8
P 8000 4250
F 0 "R8" V 7900 4150 50  0000 C CNN
F 1 "330" V 7900 4350 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7930 4250 50  0001 C CNN
F 3 "~" H 8000 4250 50  0001 C CNN
	1    8000 4250
	0    1    1    0   
$EndComp
Wire Wire Line
	9100 3750 9350 3750
Wire Wire Line
	9350 3750 9350 4250
Wire Wire Line
	9350 4250 8150 4250
Text Label 4650 3250 0    50   ~ 0
PA8
Text Label 9100 3750 0    50   ~ 0
JTEST
Text Label 4900 3450 2    50   ~ 0
GDB_RX
Text Label 4900 3350 2    50   ~ 0
GDB_TX
Text Label 3950 2600 0    50   ~ 0
GDB_RX
Text Label 3950 2400 0    50   ~ 0
GDB_TX
$Comp
L FT232RL_Breakout:FT232RL_Breakout U1
U 1 1 618702F4
P 1850 2800
F 0 "U1" H 1850 3565 50  0000 C CNN
F 1 "FT232RL_Breakout" H 1850 3474 50  0000 C CNN
F 2 "lib:FT232RL_Breakout" H 1900 2150 50  0001 C CNN
F 3 "" H 2550 1800 50  0001 C CNN
	1    1850 2800
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR01
U 1 1 618721F8
P 1100 3200
F 0 "#PWR01" H 1100 2950 50  0001 C CNN
F 1 "GND" H 1105 3027 50  0000 C CNN
F 2 "" H 1100 3200 50  0001 C CNN
F 3 "" H 1100 3200 50  0001 C CNN
	1    1100 3200
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 2900 1100 2900
Wire Wire Line
	1100 2900 1100 3200
Wire Wire Line
	1200 2700 700  2700
Text Label 700  2700 0    50   ~ 0
TRACESWO
NoConn ~ 2500 3100
NoConn ~ 2500 3000
NoConn ~ 2500 2900
NoConn ~ 2500 2800
NoConn ~ 2500 2700
NoConn ~ 2500 2600
NoConn ~ 2500 2500
NoConn ~ 2500 2400
NoConn ~ 2500 2300
NoConn ~ 1200 2300
NoConn ~ 1200 2400
NoConn ~ 1200 2500
NoConn ~ 1200 2600
NoConn ~ 1200 2800
NoConn ~ 1200 3000
NoConn ~ 1200 3100
$Comp
L power:PWR_FLAG #FLG01
U 1 1 6188E4E8
P 6200 7350
F 0 "#FLG01" H 6200 7425 50  0001 C CNN
F 1 "PWR_FLAG" H 6200 7523 50  0000 C CNN
F 2 "" H 6200 7350 50  0001 C CNN
F 3 "~" H 6200 7350 50  0001 C CNN
	1    6200 7350
	1    0    0    -1  
$EndComp
NoConn ~ 4900 4550
$Comp
L power:PWR_FLAG #FLG02
U 1 1 618A1803
P 6700 7350
F 0 "#FLG02" H 6700 7425 50  0001 C CNN
F 1 "PWR_FLAG" H 6700 7523 50  0000 C CNN
F 2 "" H 6700 7350 50  0001 C CNN
F 3 "~" H 6700 7350 50  0001 C CNN
	1    6700 7350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR09
U 1 1 618A273A
P 6700 7450
F 0 "#PWR09" H 6700 7200 50  0001 C CNN
F 1 "GND" H 6705 7277 50  0000 C CNN
F 2 "" H 6700 7450 50  0001 C CNN
F 3 "" H 6700 7450 50  0001 C CNN
	1    6700 7450
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR07
U 1 1 618A3259
P 5850 7350
F 0 "#PWR07" H 5850 7200 50  0001 C CNN
F 1 "+3V3" H 5865 7523 50  0000 C CNN
F 2 "" H 5850 7350 50  0001 C CNN
F 3 "" H 5850 7350 50  0001 C CNN
	1    5850 7350
	1    0    0    -1  
$EndComp
Wire Wire Line
	6600 2550 6600 3050
Wire Wire Line
	6700 7450 6700 7350
Wire Wire Line
	6200 7350 6200 7450
Wire Wire Line
	6200 7450 5850 7450
Wire Wire Line
	5850 7450 5850 7350
$Comp
L Connector_Generic:Conn_01x07 J8
U 1 1 618B4662
P 8950 2450
F 0 "J8" H 9030 2492 50  0000 L CNN
F 1 "LogicAna" H 9030 2401 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x07_P2.54mm_Vertical" H 8950 2450 50  0001 C CNN
F 3 "~" H 8950 2450 50  0001 C CNN
	1    8950 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	8750 2150 8450 2150
Wire Wire Line
	8750 2250 8450 2250
Wire Wire Line
	8750 2350 8450 2350
Wire Wire Line
	8750 2450 8450 2450
Wire Wire Line
	8750 2550 8450 2550
Wire Wire Line
	8750 2650 8450 2650
Text Label 8450 2450 0    50   ~ 0
JTDO
Text Label 8450 2350 0    50   ~ 0
JTDI
Text Label 8450 2150 0    50   ~ 0
JTMS
Text Label 8450 2250 0    50   ~ 0
JTCK
Text Label 8450 2650 0    50   ~ 0
JRST
Text Label 8450 2550 0    50   ~ 0
JTEST
$Comp
L Connector_Generic:Conn_01x02 J5
U 1 1 618CD373
P 4850 5750
F 0 "J5" H 4930 5792 50  0000 L CNN
F 1 "VCC" H 4930 5701 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4850 5750 50  0001 C CNN
F 3 "~" H 4850 5750 50  0001 C CNN
	1    4850 5750
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J6
U 1 1 618CED27
P 5800 5750
F 0 "J6" H 5880 5792 50  0000 L CNN
F 1 "GND" H 5880 5701 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 5800 5750 50  0001 C CNN
F 3 "~" H 5800 5750 50  0001 C CNN
	1    5800 5750
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR04
U 1 1 618CF4D3
P 4550 5550
F 0 "#PWR04" H 4550 5400 50  0001 C CNN
F 1 "+3V3" H 4565 5723 50  0000 C CNN
F 2 "" H 4550 5550 50  0001 C CNN
F 3 "" H 4550 5550 50  0001 C CNN
	1    4550 5550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR06
U 1 1 618CFB9F
P 5500 5950
F 0 "#PWR06" H 5500 5700 50  0001 C CNN
F 1 "GND" H 5505 5777 50  0000 C CNN
F 2 "" H 5500 5950 50  0001 C CNN
F 3 "" H 5500 5950 50  0001 C CNN
	1    5500 5950
	1    0    0    -1  
$EndComp
Wire Wire Line
	4550 5850 4650 5850
Wire Wire Line
	4650 5750 4550 5750
Connection ~ 4550 5750
Wire Wire Line
	4550 5750 4550 5850
Wire Wire Line
	5500 5950 5500 5850
Wire Wire Line
	5500 5650 5600 5650
Wire Wire Line
	5600 5750 5500 5750
Connection ~ 5500 5750
Wire Wire Line
	5500 5750 5500 5650
Wire Wire Line
	5600 5850 5500 5850
Connection ~ 5500 5850
Wire Wire Line
	5500 5850 5500 5750
NoConn ~ 3950 2700
Wire Wire Line
	3950 2500 4300 2500
Wire Wire Line
	4300 2500 4300 2800
$Comp
L Device:R R1
U 1 1 6186FACF
P 7150 2800
F 0 "R1" V 7050 2700 50  0000 C CNN
F 1 "1K" V 7050 2900 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7080 2800 50  0001 C CNN
F 3 "~" H 7150 2800 50  0001 C CNN
	1    7150 2800
	-1   0    0    1   
$EndComp
$Comp
L Device:LED D1
U 1 1 61872699
P 7150 3150
F 0 "D1" V 7189 3032 50  0000 R CNN
F 1 "LED" V 7098 3032 50  0000 R CNN
F 2 "LED_THT:LED_D3.0mm" H 7150 3150 50  0001 C CNN
F 3 "~" H 7150 3150 50  0001 C CNN
	1    7150 3150
	0    -1   -1   0   
$EndComp
Wire Wire Line
	7150 3000 7150 2950
Wire Wire Line
	7150 3300 7150 3550
Wire Wire Line
	7150 3550 6500 3550
$Comp
L power:+3V3 #PWR011
U 1 1 618791D1
P 7150 2500
F 0 "#PWR011" H 7150 2350 50  0001 C CNN
F 1 "+3V3" H 7165 2673 50  0000 C CNN
F 2 "" H 7150 2500 50  0001 C CNN
F 3 "" H 7150 2500 50  0001 C CNN
	1    7150 2500
	1    0    0    -1  
$EndComp
Wire Wire Line
	7150 2650 7150 2500
$Comp
L Device:R R2
U 1 1 61882F9B
P 7950 4850
F 0 "R2" V 7850 4750 50  0000 C CNN
F 1 "4K7" V 7850 4950 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7880 4850 50  0001 C CNN
F 3 "~" H 7950 4850 50  0001 C CNN
	1    7950 4850
	-1   0    0    1   
$EndComp
$Comp
L Device:R R3
U 1 1 618853F5
P 7950 5250
F 0 "R3" V 7850 5150 50  0000 C CNN
F 1 "4K7" V 7850 5350 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7880 5250 50  0001 C CNN
F 3 "~" H 7950 5250 50  0001 C CNN
	1    7950 5250
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR013
U 1 1 618857D9
P 7950 5450
F 0 "#PWR013" H 7950 5200 50  0001 C CNN
F 1 "GND" H 7955 5277 50  0000 C CNN
F 2 "" H 7950 5450 50  0001 C CNN
F 3 "" H 7950 5450 50  0001 C CNN
	1    7950 5450
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR012
U 1 1 61885C62
P 7950 4650
F 0 "#PWR012" H 7950 4500 50  0001 C CNN
F 1 "+3V3" H 7965 4823 50  0000 C CNN
F 2 "" H 7950 4650 50  0001 C CNN
F 3 "" H 7950 4650 50  0001 C CNN
	1    7950 4650
	1    0    0    -1  
$EndComp
Wire Wire Line
	6500 4350 6800 4350
Wire Wire Line
	6800 4350 6800 5050
Wire Wire Line
	7950 4650 7950 4700
Wire Wire Line
	7950 5400 7950 5450
Text Label 7900 5050 2    50   ~ 0
VREF
Text Label 6500 3550 0    50   ~ 0
LEDG
Wire Wire Line
	4900 3350 4550 3350
Wire Wire Line
	4550 3350 4550 2400
Wire Wire Line
	3950 2400 4550 2400
Wire Wire Line
	4900 3450 4450 3450
Wire Wire Line
	4450 3450 4450 2600
Wire Wire Line
	3950 2600 4450 2600
Wire Wire Line
	7950 5000 7950 5050
Wire Wire Line
	6800 5050 7950 5050
Connection ~ 7950 5050
Wire Wire Line
	7950 5050 7950 5100
Wire Wire Line
	7600 4250 7850 4250
Text Notes 850  1250 0    50   ~ 0
USART1: GDB Port\nTRACESWO: External device or Optional FT232 Breakout\nSPI2: JTAG interface (also supports bit-bang)\nTIM4: JTMS signal generation\nGPIO: LEDG, JTAG (partial)
Text Label 7600 4250 0    50   ~ 0
BTEST
Text Label 7600 3850 0    50   ~ 0
BTCK
Text Label 7600 3450 0    50   ~ 0
BTDI
Text Label 7600 3650 0    50   ~ 0
BTMS
Text Label 8400 3850 0    50   ~ 0
GND
$Comp
L Connector_Generic:Conn_01x13 J3
U 1 1 619E257C
P 2100 5850
F 0 "J3" H 2180 5842 50  0000 L CNN
F 1 "Conn_01x11" H 2180 5751 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x13_P2.54mm_Vertical" H 2100 5850 50  0001 C CNN
F 3 "~" H 2100 5850 50  0001 C CNN
	1    2100 5850
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 5650 1650 5650
Text Label 1650 6250 0    50   ~ 0
PB1
Text Label 6750 3350 2    50   ~ 0
BTXD
Text Label 6750 3250 2    50   ~ 0
BRXD
Text Label 6750 3450 2    50   ~ 0
PB1
$Comp
L Connector_Generic:Conn_01x05 J2
U 1 1 61A3D364
P 2100 4850
F 0 "J2" H 2180 4892 50  0000 L CNN
F 1 "Conn_01x05" H 2180 4801 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" H 2100 4850 50  0001 C CNN
F 3 "~" H 2100 4850 50  0001 C CNN
	1    2100 4850
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 4650 1650 4650
Wire Wire Line
	1900 4750 1650 4750
Wire Wire Line
	1900 4850 1650 4850
Wire Wire Line
	1900 4950 1650 4950
Wire Wire Line
	1900 5050 1650 5050
Wire Wire Line
	4900 3250 4650 3250
Wire Wire Line
	4900 3150 4650 3150
Wire Wire Line
	4900 3050 4650 3050
Wire Wire Line
	4900 2950 4650 2950
Wire Wire Line
	4900 2850 4650 2850
Text Label 6750 3750 2    50   ~ 0
PA6
Text Label 6750 3850 2    50   ~ 0
PA5
Text Label 6750 4050 2    50   ~ 0
PA3
Text Label 6750 3950 2    50   ~ 0
PA4
Text Label 6750 4150 2    50   ~ 0
PA2
$Comp
L Connector_Generic:Conn_01x07 J1
U 1 1 61A7D00F
P 2100 4100
F 0 "J1" H 2180 4092 50  0000 L CNN
F 1 "Conn_01x07" H 2180 4001 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x07_P2.54mm_Vertical" H 2100 4100 50  0001 C CNN
F 3 "~" H 2100 4100 50  0001 C CNN
	1    2100 4100
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 3800 1450 3800
Wire Wire Line
	1900 4000 1650 4000
Wire Wire Line
	1900 4300 1650 4300
Wire Wire Line
	1900 4400 1650 4400
Wire Wire Line
	4900 4050 4650 4050
Wire Wire Line
	4900 4150 4650 4150
Wire Wire Line
	4900 4250 4650 4250
Wire Wire Line
	4900 4350 4650 4350
Wire Wire Line
	4900 4450 4650 4450
Text Label 4650 4050 0    50   ~ 0
PB5
Text Label 6750 4250 2    50   ~ 0
PA1
Text Label 6750 3650 2    50   ~ 0
PA7
Text Label 4650 4350 0    50   ~ 0
JENA
Text Label 1450 3800 0    50   ~ 0
TRACESWO
Text Label 1650 4000 0    50   ~ 0
PB5
Text Label 1650 5050 0    50   ~ 0
PA8
Text Label 1650 4300 0    50   ~ 0
JENA
Wire Wire Line
	4550 5550 4550 5750
NoConn ~ 4900 3550
NoConn ~ 4900 3650
Wire Wire Line
	1900 5750 1650 5750
Wire Wire Line
	1900 5850 1650 5850
Wire Wire Line
	1900 5950 1650 5950
Wire Wire Line
	1900 6050 1650 6050
Wire Wire Line
	1900 6150 1650 6150
Wire Wire Line
	1900 6250 1650 6250
Wire Wire Line
	1900 6350 1650 6350
Text Label 1650 6150 0    50   ~ 0
LEDG
Text Label 1650 4950 0    50   ~ 0
BTDI
Text Label 1650 4850 0    50   ~ 0
JTDO
Text Label 1650 4750 0    50   ~ 0
BTCK
Text Label 1650 4400 0    50   ~ 0
BTEST
Text Label 1650 4200 0    50   ~ 0
BTMS
Text Label 1650 4650 0    50   ~ 0
BRST
Wire Wire Line
	6500 4650 6750 4650
Text Label 6750 4650 2    50   ~ 0
PC13
Wire Wire Line
	7850 3850 7600 3850
Wire Wire Line
	6500 3850 6750 3850
Text Label 4650 4150 0    50   ~ 0
BTCK
Wire Wire Line
	6500 4250 6750 4250
Wire Wire Line
	6500 4150 6750 4150
Wire Wire Line
	6500 4050 6750 4050
Wire Wire Line
	6500 3950 6750 3950
Wire Wire Line
	6500 3650 6750 3650
Wire Wire Line
	6500 3750 6750 3750
NoConn ~ 6500 4450
NoConn ~ 6500 4550
Wire Wire Line
	6500 3450 6750 3450
Text Label 9300 3950 2    50   ~ 0
JTXD
$Comp
L Device:R R9
U 1 1 61B80BD6
P 9750 3850
F 0 "R9" V 9650 3750 50  0000 C CNN
F 1 "330" V 9650 3950 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 9680 3850 50  0001 C CNN
F 3 "~" H 9750 3850 50  0001 C CNN
	1    9750 3850
	0    1    1    0   
$EndComp
Wire Wire Line
	8600 4050 8350 4050
Connection ~ 8350 4050
Wire Wire Line
	8350 4050 8350 4300
$Comp
L Connector_Generic:Conn_02x07_Odd_Even J7
U 1 1 6186D80B
P 8800 3750
F 0 "J7" H 8850 4267 50  0000 C CNN
F 1 "JTAG" H 8850 4176 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x07_P2.54mm_Vertical" H 8800 3750 50  0001 C CNN
F 3 "~" H 8800 3750 50  0001 C CNN
	1    8800 3750
	1    0    0    -1  
$EndComp
$Comp
L Device:R R10
U 1 1 61B88E56
P 9750 4050
F 0 "R10" V 9650 3950 50  0000 C CNN
F 1 "330" V 9650 4150 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 9680 4050 50  0001 C CNN
F 3 "~" H 9750 4050 50  0001 C CNN
	1    9750 4050
	0    1    1    0   
$EndComp
Wire Wire Line
	9100 4050 9600 4050
Wire Wire Line
	9100 3950 9500 3950
Wire Wire Line
	9500 3950 9500 3850
Wire Wire Line
	9500 3850 9600 3850
Wire Wire Line
	9900 3850 10250 3850
Wire Wire Line
	9900 4050 10250 4050
Text Label 10250 3850 2    50   ~ 0
BRXD
Text Label 9300 4050 2    50   ~ 0
JRXD
Text Label 10250 4050 2    50   ~ 0
BTXD
Wire Wire Line
	6500 3350 6750 3350
Wire Wire Line
	6500 3250 6750 3250
NoConn ~ 6500 3150
NoConn ~ 4900 3750
NoConn ~ 4900 3950
Text Label 1650 5450 0    50   ~ 0
PA1
Text Label 1650 5550 0    50   ~ 0
PA2
Text Label 1650 5650 0    50   ~ 0
PA3
Text Label 1650 5750 0    50   ~ 0
PA4
Text Label 1650 5850 0    50   ~ 0
PA5
Text Label 1650 6050 0    50   ~ 0
PA7
Text Label 1650 5950 0    50   ~ 0
PA6
Wire Wire Line
	1900 5450 1650 5450
Wire Wire Line
	1900 5550 1650 5550
NoConn ~ 6500 4750
Wire Wire Line
	1900 5250 1650 5250
Wire Wire Line
	1900 5350 1650 5350
Wire Wire Line
	1900 6450 1650 6450
Text Label 1650 6450 0    50   ~ 0
BRXD
Text Label 1650 5250 0    50   ~ 0
PC13
Text Label 1650 5350 0    50   ~ 0
VREF
Text Label 1650 6350 0    50   ~ 0
BTXD
NoConn ~ 1900 3900
Text Label 1650 4100 0    50   ~ 0
BTCK
Wire Wire Line
	1900 4100 1650 4100
Wire Wire Line
	1900 4200 1650 4200
$Comp
L Device:R R11
U 1 1 619EC4B5
P 8000 3250
F 0 "R11" V 7900 3150 50  0000 C CNN
F 1 "330" V 7900 3350 50  0000 C CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 7930 3250 50  0001 C CNN
F 3 "~" H 8000 3250 50  0001 C CNN
	1    8000 3250
	0    1    1    0   
$EndComp
Wire Wire Line
	7850 3250 7800 3250
Wire Wire Line
	7800 3250 7800 3450
Connection ~ 7800 3450
Wire Wire Line
	7800 3450 7850 3450
Wire Wire Line
	8350 3450 8350 3250
Wire Wire Line
	8350 3250 8150 3250
$Comp
L Device:C C1
U 1 1 61A0708E
P 9450 3300
F 0 "C1" H 9565 3346 50  0000 L CNN
F 1 "100nF" H 9565 3255 50  0000 L CNN
F 2 "Capacitor_THT:C_Disc_D4.3mm_W1.9mm_P5.00mm" H 9488 3150 50  0001 C CNN
F 3 "~" H 9450 3300 50  0001 C CNN
	1    9450 3300
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR016
U 1 1 61A08324
P 9450 3500
F 0 "#PWR016" H 9450 3250 50  0001 C CNN
F 1 "GND" H 9455 3327 50  0000 C CNN
F 2 "" H 9450 3500 50  0001 C CNN
F 3 "" H 9450 3500 50  0001 C CNN
	1    9450 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	9450 3500 9450 3450
Wire Wire Line
	9450 3150 9450 3100
Wire Wire Line
	9450 3100 9200 3100
Connection ~ 9200 3100
Wire Wire Line
	9200 3100 9200 3050
$Comp
L power:GND #PWR?
U 1 1 61A3CEF5
P 8700 2800
F 0 "#PWR?" H 8700 2550 50  0001 C CNN
F 1 "GND" H 8705 2627 50  0000 C CNN
F 2 "" H 8700 2800 50  0001 C CNN
F 3 "" H 8700 2800 50  0001 C CNN
	1    8700 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	8700 2800 8700 2750
Wire Wire Line
	8700 2750 8750 2750
Text Label 4650 4450 0    50   ~ 0
BTEST
$EndSCHEMATC
