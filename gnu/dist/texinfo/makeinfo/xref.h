/*	$NetBSD: xref.h,v 1.1.1.1 2004/07/12 23:26:47 wiz Exp $	*/

/* xref.h -- declarations for the cross references.
   Id: xref.h,v 1.2 2004/02/13 22:01:39 dirt Exp

   Copyright (C) 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef XREF_H
#define XREF_H

enum reftype
{
  menu_reference, followed_reference
};

extern char *get_xref_token (int expand);

#endif /* not XREF_H */
