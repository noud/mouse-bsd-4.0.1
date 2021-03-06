/*	$NetBSD: boot_params.S,v 1.4 2005/12/11 12:17:48 christos Exp $	*/

/* Default boot parameters - must match struct x86_boot_params in bootblock.h */

	.long	0			/* flags */
	.long	5			/* timeout in seconds */
	.long	0			/* console device 0 => CONSDEV_PC */
	.long	9600			/* serial baud rate */
	.space	16			/* md5 boot password */
	.space	64			/* keyboard xlat map */
	.long	0			/* console ioaddr */
