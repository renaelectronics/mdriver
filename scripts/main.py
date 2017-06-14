#!/usr/bin/python
"""
# set/get methods for bit array
#
#                   1         2
#   pos : 012345678901234567890123
# index : 0.......1.......2.......
"""
import sys

""" constant and global """
BIT_GET = 0
BIT_SET = 1
BIT_CLR = 2

""" tx/rx packet """
""" [ HEADER ][ PAYLOAD ][ CRC ] """
HEADER_SIZE = 4
PAYLOAD_SIZE = 128
CRC_SIZE = 1
MAX_PACKET_SIZE = (HEADER_SIZE + PAYLOAD_SIZE + CRC_SIZE)

""" AN1310 define """
STX = 0xf
ETX = 0x4
DLE = 0x5

""" packet size """
ascii = ['0','1']
tx_packet = bytearray(MAX_PACKET_SIZE)
rx_packet = bytearray(MAX_PACKET_SIZE)
payload = bytearray(PAYLOAD_SIZE)
piodata = 0;
pioclk = 0;

""" functions """
def write_bit(data):
	# clock low
	sys.stdout.write('%c' % ascii[0])
	# data value
	sys.stdout.write('%c' % ascii[data])
	# clock high
	sys.stdout.write('%c' % ascii[1])
	# data value
	sys.stdout.write('%c' % ascii[data])
	# clodk low
	sys.stdout.write('%c' % ascii[0])
	# data value
	sys.stdout.write('%c' % ascii[data])

def write_packet(packet):
	for byte in packet:
		for i in range(0, 8):
			if (byte & (1<<i)):
				write_bit(0)
			else:
				write_bit(0)

def read_bootloader_information():
	packet = bytearray([STX,0x00,0x00,0x00,ETX])
	write_packet(packet)	
	return packet	

def bit_op(bytearray, pos, op):
	"""given a bytearray, get/set/clr the bit position"""
	# boundary check
	if (pos/8  >= len(bytearray)):
		return -1

	index = pos/8
	mask = (0x80 >> (pos & 0x7));
        if op == BIT_GET:
		if (bytearray[index] & mask):
	       		return 1
		else:
			return 0

	if op == BIT_SET:
		bytearray[index] = bytearray[index] | mask
		return 0

	if op == BIT_CLR:
		bytearray[index] = bytearray[index] & ~mask
		return 0
	return 0
	
def set_bit(bytearray, pos):
        return bit_op(bytearray, pos, BIT_SET)

def clr_bit(bytarray, pos):
        return bit_op(bytearray, pos, BIT_CLR)

def get_bit(bytarray, pos):
        return bit_op(bytarray, pos, BIT_GET)

def dump_data(bytearray):
	for n in range(0, len(bytearray)):
		sys.stdout.write('%02x ' % bytearray[n])
		if (((n+1)%16) == 0):
			sys.stdout.write("\n")	
	sys.stdout.write("\n")
	
""" initialize variables """
for n in range(0, MAX_PACKET_SIZE-1):
	rx_packet[n] = 0
	tx_packet[n] = 0

""" read form stdin and packet it into a tx_packet format """
#for character in sys.stdin:
#	sys.stdout.write(character)

read_bootloader_information()
