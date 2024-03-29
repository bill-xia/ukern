/*
 * Copyright 2012, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2002-2003, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef ATA_COMMANDS_H
#define ATA_COMMANDS_H

#define ATA_COMMAND_WRITE_DMA				0xca
#define ATA_COMMAND_WRITE_DMA_QUEUE			0xcc
#define ATA_COMMAND_WRITE_MULTIPLE			0xc5
#define ATA_COMMAND_WRITE_SECTORS			0x30

#define ATA_COMMAND_READ_DMA				0xc8
#define ATA_COMMAND_READ_DMA_QUEUED			0xc7
#define ATA_COMMAND_READ_MULTIPLE			0xc4
#define ATA_COMMAND_READ_SECTORS			0x20

#define ATA_COMMAND_WRITE_DMA_EXT			0x35
#define ATA_COMMAND_WRITE_DMA_QUEUED_EXT		0x36
#define ATA_COMMAND_WRITE_MULTIPLE_EXT			0x39
#define ATA_COMMAND_WRITE_SECTORS_EXT			0x34

#define ATA_COMMAND_READ_DMA_EXT			0x25
#define ATA_COMMAND_READ_DMA_QUEUED_EXT			0x26
#define ATA_COMMAND_READ_MULTIPLE_EXT			0x29
#define ATA_COMMAND_READ_SECTORS_EXT			0x24

#define ATA_COMMAND_PACKET				0xa0
#define ATA_COMMAND_DEVICE_RESET			0x08

#define ATA_COMMAND_SERVICE				0xa2
#define ATA_COMMAND_NOP					0

#define ATA_COMMAND_NOP_NOP				0
#define ATA_COMMAND_NOP_NOP_AUTOPOLL			1


#define ATA_COMMAND_GET_MEDIA_STATUS			0xda

#define ATA_COMMAND_FLUSH_CACHE				0xe7
#define ATA_COMMAND_FLUSH_CACHE_EXT			0xea

#define ATA_COMMAND_DATA_SET_MANAGEMENT			0x06

#define ATA_COMMAND_MEDIA_EJECT				0xed

#define ATA_COMMAND_IDENTIFY_PACKET_DEVICE		0xa1
#define ATA_COMMAND_IDENTIFY_DEVICE			0xec

#define ATA_COMMAND_SET_FEATURES			0xef
#define ATA_COMMAND_SET_FEATURES_ENABLE_RELELEASE_INT	0x5d
#define ATA_COMMAND_SET_FEATURES_ENABLE_SERVICE_INT	0x5e
#define ATA_COMMAND_SET_FEATURES_DISABLE_RELEASE_INT	0xdd
#define ATA_COMMAND_SET_FEATURES_DISABLE_SERVICE_INT	0xde

#endif // ATA_COMMANDS_H
