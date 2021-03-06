/*	$NetBSD: it8368reg.h,v 1.4 2001/09/15 12:47:05 uch Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	ITE IT8368E PCMCIA/GPIO Buffer Chip
 *	http://www.ite.com/tw/mobile/it8368v07.pdf
 */
#define IT8368_GPIODATAOUT_REG		0x00
#define IT8368_MFIODATAOUT_REG		0x02
#define IT8368_GPIODIR_REG		0x04
#define IT8368_MFIODIR_REG		0x06
#define IT8368_MFIOSEL_REG		0x0a
#define IT8368_GPIODATAIN_REG		0x0c
#define IT8368_MFIODATAIN_REG		0x0e
#define IT8368_GPIOPOSINTEN_REG		0x10
#define IT8368_MFIOPOSINTEN_REG		0x12
#define IT8368_GPIONEGINTEN_REG		0x14
#define IT8368_MFIONEGINTEN_REG		0x16
#define IT8368_GPIOPOSINTSTAT_REG	0x18
#define IT8368_MFIOPOSINTSTAT_REG	0x1a
#define IT8368_GPIONEGINTSTAT_REG	0x1c
#define IT8368_MFIONEGINTSTAT_REG	0x1e
#define IT8368_CTRL_REG			0x20

#define IT8368_GPIO_MAX			12
#define IT8368_MFIO_MAX			10

#define IT8368_GPIODATAOUT_MASK		0x1fff
#define IT8368_MFIODATAOUT_MASK		0x07ff
#define IT8368_GPIODIR_MASK		0x1fff
#define IT8368_MFIODIR_MASK		0x07ff
					  
#define IT8368_MFIOSEL_VGAEN		0x0800
#define IT8368_MFIOSEL_MASK		0x07ff
#define IT8368_GPIODATAIN_MASK		0x1fff
#define IT8368_MFIODATAIN_MASK		0x07ff
#define IT8368_GPIOPOSINTEN_MASK	0x1fff
#define IT8368_MFIOPOSINTEN_MASK	0x07ff
#define IT8368_GPIONEGINTEN_MASK	0x1fff
#define IT8368_MFIONEGINTEN_MASK	0x07ff
#define IT8368_GPIOPOSINTSTAT_MASK	0x1fff
#define IT8368_MFIOPOSINTSTAT_MASK	0x07ff
#define IT8368_GPIONEGINTSTAT_MASK	0x1fff
#define IT8368_MFIONEGINTSTAT_MASK	0x07ff
					  
					  
#define IT8368_CTRL_FIXATTRIO		0x8000
#define IT8368_FIXATTR_OFFSET		0x02000000
#define IT8368_FIXIO_OFFSET		0x0
#define IT8368_FIXIOATTR_SIZE		0x02000000

#define	IT8368_CTRL_ADDRSEL		0x0010
#define	IT8368_CTRL_BYTESWAP		0x0008
#define	IT8368_CTRL_CARDEN		0x0004
#define	IT8368_CTRL_GLOBALEN		0x0002
#define	IT8368_CTRL_INTTRIEN		0x0001
					  
#define	IT8368_PIN_CRDSW		0x1000
#define	IT8368_PIN_CRDDET2		0x0800
#define	IT8368_PIN_CRDDET1		0x0400
#define	IT8368_PIN_CRDSENSE2		0x0200
#define	IT8368_PIN_CRDSENSE1		0x0100
#define	IT8368_PIN_CRDVCCON1		0x0080
#define	IT8368_PIN_CRDVCCON0		0x0040
#define	IT8368_PIN_CRDVPPON1		0x0020
#define	IT8368_PIN_CRDVPPON0		0x0010
#define	IT8368_PIN_BCRDWP		0x0008
#define	IT8368_PIN_BCRDRDY		0x0004
#define	IT8368_PIN_BCRBVD2		0x0002
#define	IT8368_PIN_BCRDRST		0x0001

#define IT8368_PIN_CRDVCCMASK		0x00c0
#define IT8368_PIN_CRDVPPMASK		0x0030
#define IT8368_PIN_CRDVCC_0V		0x0000
#define IT8368_PIN_CRDVCC_3V		IT8368_PIN_CRDVCCON0
#define IT8368_PIN_CRDVCC_5V		IT8368_PIN_CRDVCCON1
#define IT8368_PIN_CRDVPP_0V		0x0000
#define IT8368_PIN_CRDVPP_CRDVCC	IT8368_PIN_CRDVPPON0
#define IT8368_PIN_CRDVCC_12V		IT8368_PIN_CRDVPPON1
#define IT8368_PIN_CRDVCC_HIZ		(IT8368_PIN_CRDVPPON0 |		\
					IT8368_PIN_CRDVPPON1)
