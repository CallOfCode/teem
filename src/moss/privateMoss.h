/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define MOSS_MAT_SET(mat, a, b, c, d, x, y) \
  (mat)[0]=(a); (mat)[1]=(b); \
  (mat)[2]=(c); (mat)[3]=(d); \
  (mat)[4]=(x); (mat)[5]=(y)

#define MOSS_MAT_COPY(m2, m1) \
  (m2)[0] = (m1)[0]; (m2)[1] = (m1)[1]; (m2)[2] = (m1)[2]; \
  (m2)[3] = (m1)[3]; (m2)[4] = (m1)[4]; (m2)[5] = (m1)[5]

#define MOSS_MAT_6TO9(m2, m1) \
  ELL_3V_SET((m2)+0, (m1)[0], (m1)[1], 0); \
  ELL_3V_SET((m2)+3, (m1)[2], (m1)[3], 0); \
  ELL_3V_SET((m2)+6, (m1)[4], (m1)[5], 1)

#define MOSS_MAT_9TO6(m2, m1) \
  MOSS_MAT_SET(m2, (m1)[0], (m1)[1], (m1)[3], (m1)[4], (m1)[6], (m1)[7])


/* methodsMoss.c */
extern int _mossCenter(int center);
