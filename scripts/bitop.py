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

""" packet size """
ascii = ['0','1']
tx_packet = bytearray(MAX_PACKET_SIZE)
rx_packet = bytearray(MAX_PACKET_SIZE)
payload = bytearray(PAYLOAD_SIZE)

""" functions """
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
