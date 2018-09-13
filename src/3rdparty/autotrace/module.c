/* module.c --- Autotrace plugin module management subsystem

  Copyright (C) 2003 Martin Weber
  Copyright (C) 2003 Masatake YAMATO

  The author can be contacted at <martweb@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */
#include "intl.h"

#include "private.h"

#include "input.h"

#ifdef HAVE_LIBPNG
#endif /* HAVE_LIBPNG */
#if HAVE_MAGICK
#else
int install_input_magick_readers(void)
{
  return 0;
}
#endif /* HAVE_MAGICK */

#if HAVE_LIBPSTOEDIT
#else
int install_output_pstoedit_writers(void)
{
  return 0;
}
#endif /* HAVE_LIBPSTOEDIT */

static int install_input_readers(void);
static int install_output_writers(void);

int at_module_init(void)
{
  int r, w;
  /* TODO: Loading every thing in dynamic.
     For a while, these are staticly added. */
  r = install_input_readers();
  w = install_output_writers();
  return (int)(r << 2 | w);
}

static int install_input_readers(void)
{
  return 0;
#ifdef HAVE_LIBPNG
#endif
}

static int install_output_writers(void)
{
  return 0;
}
