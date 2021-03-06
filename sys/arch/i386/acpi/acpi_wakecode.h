/* THIS FILE IS AUTOMATICALLY GENERATED. DO NOT EDIT. */
/* from: NetBSD: acpi_wakecode.S,v 1.8 2006/06/20 22:36:58 jmcneill Exp  */
/* $NetBSD: acpi_wakecode.h,v 1.7 2006/06/20 22:37:14 jmcneill Exp $ */

#define	wakeup_16	0x00000000
#define	novbiosreset	0x00000026
#define	wakeup_sw32	0x0000005f
#define	wakeup_32	0x00000070
#define	beepon	0x00000107
#define	beepoff	0x00000116
#define	tmp_gdt	0x00000120
#define	tmp_gdtable	0x00000128
#define	physical_gdt	0x00000140
#define	previous_cr2	0x00000146
#define	previous_cr3	0x0000014a
#define	previous_cr4	0x0000014e
#define	previous_cr0	0x00000152
#define	previous_tr	0x00000156
#define	previous_gdt	0x00000158
#define	previous_ldt	0x0000015e
#define	previous_idt	0x00000160
#define	previous_ds	0x00000166
#define	previous_es	0x00000168
#define	previous_fs	0x0000016a
#define	previous_gs	0x0000016c
#define	previous_ss	0x0000016e
#define	where_to_recover	0x00000170
#define	vbios_reset	0x00000174

static const unsigned char wakecode[] = {
	0x90, 0xfa, 0xfc, 0x8c, 0xc8, 0x8e, 0xd8, 0x8e,
	0xd0, 0xbc, 0x00, 0x10, 0x66, 0x6a, 0x00, 0x66,
	0x9d, 0xe8, 0xf3, 0x00, 0x80, 0x3e, 0x74, 0x01,
	0x01, 0x75, 0x0b, 0x9a, 0x03, 0x00, 0x00, 0xc0,
	0x8c, 0xc8, 0x8e, 0xd8, 0x8e, 0xd0, 0xe8, 0xed,
	0x00, 0x66, 0x31, 0xf6, 0x8c, 0xce, 0x66, 0xc1,
	0xe6, 0x04, 0x66, 0x89, 0xf0, 0x66, 0x05, 0x70,
	0x00, 0x00, 0x00, 0x66, 0xa3, 0x61, 0x00, 0xeb,
	0x00, 0xeb, 0x00, 0x66, 0x89, 0xf0, 0x66, 0x05,
	0x28, 0x01, 0x00, 0x00, 0x66, 0xa3, 0x22, 0x01,
	0x0f, 0x01, 0x16, 0x20, 0x01, 0x0f, 0x20, 0xc0,
	0x66, 0x83, 0xc8, 0x01, 0x0f, 0x22, 0xc0, 0x66,
	0xea, 0x78, 0x56, 0x34, 0x12, 0x08, 0x00, 0x89,
	0xf6, 0x8d, 0xbc, 0x27, 0x00, 0x00, 0x00, 0x00,
	0x90, 0x66, 0xb8, 0x10, 0x00, 0x8e, 0xd8, 0x8e,
	0xc0, 0x8e, 0xe8, 0x8e, 0xd0, 0x8e, 0xe0, 0x8b,
	0x9e, 0x42, 0x01, 0x00, 0x00, 0x66, 0x0f, 0xb7,
	0x8e, 0x56, 0x01, 0x00, 0x00, 0x8d, 0x04, 0x0b,
	0x80, 0x60, 0x05, 0xf9, 0x8b, 0x86, 0x4a, 0x01,
	0x00, 0x00, 0x0f, 0x22, 0xd8, 0x8b, 0x86, 0x52,
	0x01, 0x00, 0x00, 0x0f, 0x22, 0xc0, 0xeb, 0x00,
	0xeb, 0x00, 0x90, 0x0f, 0x01, 0x96, 0x58, 0x01,
	0x00, 0x00, 0x0f, 0x01, 0x9e, 0x60, 0x01, 0x00,
	0x00, 0x0f, 0x00, 0x96, 0x5e, 0x01, 0x00, 0x00,
	0x8b, 0x86, 0x46, 0x01, 0x00, 0x00, 0x0f, 0x22,
	0xd0, 0x8b, 0x86, 0x4e, 0x01, 0x00, 0x00, 0x0f,
	0x22, 0xe0, 0x66, 0x8b, 0x86, 0x68, 0x01, 0x00,
	0x00, 0x8e, 0xc0, 0x66, 0x8b, 0x86, 0x6a, 0x01,
	0x00, 0x00, 0x8e, 0xe0, 0x66, 0x8b, 0x86, 0x6c,
	0x01, 0x00, 0x00, 0x8e, 0xe8, 0x66, 0x8b, 0x86,
	0x6e, 0x01, 0x00, 0x00, 0x8e, 0xd0, 0x8b, 0x9e,
	0x70, 0x01, 0x00, 0x00, 0x66, 0x8b, 0x86, 0x66,
	0x01, 0x00, 0x00, 0x8e, 0xd8, 0xff, 0xe3, 0xb0,
	0xc0, 0xe6, 0x42, 0xb0, 0x04, 0xe6, 0x42, 0xe4,
	0x61, 0x0c, 0x03, 0xe6, 0x61, 0xc3, 0xe4, 0x61,
	0x24, 0xfc, 0xe6, 0x61, 0xc3, 0x8d, 0x76, 0x00,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x9f, 0xcf, 0x00,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
