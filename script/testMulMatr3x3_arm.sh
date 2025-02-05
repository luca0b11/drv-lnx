#!/bin/bash

#   A       x       B       =       C
#1	3	0           1               10
#4	2	1           3               11
#0	2	1           1               7

#set matr A
busybox devmem 0xb000000 w 0x01
busybox devmem 0xb000004 w 0x03
busybox devmem 0xb000008 w 0x00

busybox devmem 0xb00000C w 0x04
busybox devmem 0xb000010 w 0x02
busybox devmem 0xb000014 w 0x01

busybox devmem 0xb000018 w 0x00
busybox devmem 0xb00001C w 0x02
busybox devmem 0xb000020 w 0x01

#set mart B
busybox devmem 0xb000200 w 0x01
busybox devmem 0xb000204 w 0x03
busybox devmem 0xb000208 w 0x01

#set size
busybox devmem 0xb000410 w 0x03

#set bit 0 of control reg to 1, to start computation
busybox devmem 0xb000400 w 0x01

#read status reg
busybox devmem 0xb000420 w

#read matr C
busybox devmem 0xb000300 w
busybox devmem 0xb000304 w
busybox devmem 0xb000308 w
