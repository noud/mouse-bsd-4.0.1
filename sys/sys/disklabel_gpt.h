/*	$NetBSD: disklabel_gpt.h,v 1.3 2006/08/13 15:31:21 christos Exp $	*/

/*
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/gpt.h,v 1.7 2004/08/02 18:46:52 marcel Exp $
 */

#ifndef _SYS_DISKLABEL_GPT_H_
#define	_SYS_DISKLABEL_GPT_H_

/*
 * Definitions for the EFI GUID Partition Table disk partitioning scheme.
 *
 * NOTE: As EFI is an Intel specification, all fields are stored in
 * little-endian byte-order.
 */

/*
 * GUID Partition Table Header
 */
struct gpt_hdr {
	int8_t		hdr_sig[8];	/* identifies GUID Partition Table */
	uint32_t	hdr_revision;	/* GPT specification revsion */
	uint32_t	hdr_size;	/* size of GPT Header */
	uint32_t	hdr_crc_self;	/* CRC32 of GPT Header */
	uint32_t	hdr__rsvd0;	/* must be zero */
	uint64_t	hdr_lba_self;	/* LBA that contains this Header */
	uint64_t	hdr_lba_alt;	/* LBA of backup GPT Header */
	uint64_t	hdr_lba_start;	/* first LBA usable for partitions */
	uint64_t	hdr_lba_end;	/* last LBA usable for partitions */
	uint8_t		hdr_guid[16];	/* GUID to identify the disk */
	uint64_t	hdr_lba_table;	/* first LBA of GPE array */
	uint32_t	hdr_entries;	/* number of entries in GPE array */
	uint32_t	hdr_entsz;	/* size of each GPE */
	uint32_t	hdr_crc_table;	/* CRC32 of GPE array */
	/*
	 * The remainder of the block that contains the GPT Header
	 * is reserved by EFI for future GPT Header expansion, and
	 * must be zero.
	 */
};

#define	GPT_HDR_SIG		"EFI PART"
#define	GPT_HDR_REVISION	0x00010000	/* 1.0 */

#define	GPT_HDR_BLKNO		1

/*
 * GUID Partition Entry
 */
struct gpt_ent {
	uint8_t		ent_type[16];	/* partition type GUID */
	uint8_t		ent_guid[16];	/* unique partition GUID */
	uint64_t	ent_lba_start;	/* start of partition */
	uint64_t	ent_lba_end;	/* end of partition */
	uint64_t	ent_attr;	/* partition attributes */
	uint16_t	ent_name[36];	/* partition name in UNICODE-16 */
};

#define	GPT_ENT_ATTR_PLATFORM	(1ULL << 0)	/* required for platform
						   to function */

/*
 * Partition types defined by the EFI specification:
 *
 *	GPT_ENT_TYPE_UNUSED		Unused Entry
 *	GPT_ENT_TYPE_EFI		EFI System Partition
 *	GPT_ENT_TYPE_MBR		Partition containing legacy MBR
 */
#define	GPT_ENT_TYPE_UNUSED		\
	{0x00000000,0x0000,0x0000,0x00,0x00,{0x00,0x00,0x00,0x00,0x00,0x00}}
#define	GPT_ENT_TYPE_EFI		\
	{0xc12a7328,0xf81f,0x11d2,0xba,0x4b,{0x00,0xa0,0xc9,0x3e,0xc9,0x3b}}
#define	GPT_ENT_TYPE_MBR		\
	{0x024dee41,0x33e7,0x11d3,0x9d,0x69,{0x00,0x08,0xc7,0x81,0xf3,0x9f}}

/*
 * Partition types defined by other operating systems.
 */
#define	GPT_ENT_TYPE_FREEBSD		\
	{0x516e7cb4,0x6ecf,0x11d6,0x8f,0xf8,{0x00,0x02,0x2d,0x09,0x71,0x2b}}
#define	GPT_ENT_TYPE_FREEBSD_SWAP	\
	{0x516e7cb5,0x6ecf,0x11d6,0x8f,0xf8,{0x00,0x02,0x2d,0x09,0x71,0x2b}}
#define	GPT_ENT_TYPE_FREEBSD_UFS	\
	{0x516e7cb6,0x6ecf,0x11d6,0x8f,0xf8,{0x00,0x02,0x2d,0x09,0x71,0x2b}}
#define	GPT_ENT_TYPE_FREEBSD_VINUM	\
	{0x516e7cb8,0x6ecf,0x11d6,0x8f,0xf8,{0x00,0x02,0x2d,0x09,0x71,0x2b}}
/*
 * The following are unused but documented here to avoid reuse.
 *
 *      GPT_ENT_TYPE_FREEBSD_UFS2	\
 *	{0x516e7cb7,0x6ecf,0x11d6,0x8f,0xf8,{0x00,0x02,0x2d,0x09,0x71,0x2b}}
 */

#define	GPT_ENT_TYPE_MS_RESERVED	\
	{0xe3c9e316,0x0b5c,0x4db8,0x81,0x7d,{0xf9,0x2d,0xf0,0x02,0x15,0xae}}
#define	GPT_ENT_TYPE_MS_BASIC_DATA	\
	{0xebd0a0a2,0xb9e5,0x4433,0x87,0xc0,{0x68,0xb6,0xb7,0x26,0x99,0xc7}}
#define	GPT_ENT_TYPE_MS_LDM_METADATA	\
	{0x5808c8aa,0x7e8f,0x42e0,0x85,0xd2,{0xe1,0xe9,0x04,0x34,0xcf,0xb3}}
#define	GPT_ENT_TYPE_MS_LDM_DATA	\
	{0xaf9b60a0,0x1431,0x4f62,0xbc,0x68,{0x33,0x11,0x71,0x4a,0x69,0xad}}

#define	GPT_ENT_TYPE_LINUX_DATA		GPT_ENT_TYPE_MS_BASIC_DATA
#define	GPT_ENT_TYPE_LINUX_RAID		\
	{0xa19d880f,0x05fc,0x4d3b,0xa0,0x06,{0x74,0x3f,0x0f,0x84,0x91,0x1e}}
#define	GPT_ENT_TYPE_LINUX_SWAP		\
	{0x0657fd6d,0xa4ab,0x43c4,0x84,0xe5,{0x09,0x33,0xc8,0x4b,0x4f,0x4f}}
#define	GPT_ENT_TYPE_LINUX_LVM		\
	{0xe6d6d379,0xf507,0x44c2,0xa2,0x3c,{0x23,0x8f,0x2a,0x3d,0xf9,0x28}}

#define	GPT_ENT_TYPE_APPLE_HFS		\
	{0x48465300,0x0000,0x11aa,0xaa,0x11,{0x00,0x30,0x65,0x43,0xec,0xac}}

#endif /* _SYS_DISKLABEL_GPT_H_ */
