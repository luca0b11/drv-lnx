#!/bin/bash

#   A           x       B       =       C
#1	2	4   0           3               13
#0  5   7   1           1               23
#1  0   5   3           2               25
#1  1   0   4           4               20

#set matr A
busybox devmem 0xb000000 w 0x01
busybox devmem 0xb000004 w 0x02
busybox devmem 0xb000008 w 0x04
busybox devmem 0xb00000C w 0x00

busybox devmem 0xb000010 w 0x00
busybox devmem 0xb000014 w 0x05
busybox devmem 0xb000018 w 0x07
busybox devmem 0xb00001C w 0x01

busybox devmem 0xb000020 w 0x01
busybox devmem 0xb000024 w 0x00
busybox devmem 0xb000028 w 0x05
busybox devmem 0xb00002C w 0x03

busybox devmem 0xb000030 w 0x01
busybox devmem 0xb000034 w 0x01
busybox devmem 0xb000038 w 0x00
busybox devmem 0xb00003C w 0x04

#set mart B
busybox devmem 0xb000200 w 0x03
busybox devmem 0xb000204 w 0x01
busybox devmem 0xb000208 w 0x02
busybox devmem 0xb00020C w 0x04

#set size
busybox devmem 0xb000410 w 0x04

#set bit 0 of control reg to 1, to start computation
busybox devmem 0xb000400 w 0x01

#read status reg
busybox devmem 0xb000420 w

#read matr C
busybox devmem 0xb000300 w
busybox devmem 0xb000304 w
busybox devmem 0xb000308 w
busybox devmem 0xb00030C w
