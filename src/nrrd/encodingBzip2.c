/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "nrrd.h"
#include "privateNrrd.h"

#if TEEM_BZIP2
#include <bzlib.h>
#endif

int
_nrrdEncodingBzip2_available(void) {

#if TEEM_BZIP2
  return AIR_TRUE;
#else
  return AIR_FALSE;
#endif
}

int
_nrrdEncodingBzip2_read(Nrrd *nrrd, NrrdIoState *nio) {
  char me[]="_nrrdEncodingBzip2_read", err[AIR_STRLEN_MED];
#if TEEM_BZIP2
  size_t num, bsize, size, total_read;
  int block_size, read, i, bzerror=BZ_OK;
  char *data;
  BZFILE* bzfin;
  
  if (nio->skipData) {
    return 0;
  }
  num = nrrdElementNumber(nrrd);
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  if (num != bsize/nrrdElementSize(nrrd)) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  /* Allocate memory for the incoming data. */
  if (_nrrdCalloc(nrrd)) {
    sprintf(err, "%s: couldn't allocate sufficient memory for all data", me);
    biffAdd(NRRD, err); return 1;
  }

  /* Create the BZFILE* for reading in the gzipped data. */
  bzfin = BZ2_bzReadOpen(&bzerror, nio->dataFile, 0, 0, NULL, 0);
  if (bzerror != BZ_OK) {
    /* there was a problem */
    sprintf(err, "%s: error opening BZFILE: %s", me, 
	    BZ2_bzerror(bzfin, &bzerror));
    biffAdd(NRRD, err);
    BZ2_bzReadClose(&bzerror, bzfin);
    return 1;
  }

  /* Here is where we do the byte skipping. */
  for(i = 0; i < nio->byteSkip; i++) {
    unsigned char b;
    /* Check to see if a single byte was able to be read. */
    read = BZ2_bzRead(&bzerror, bzfin, &b, 1);
    if (read != 1 || bzerror != BZ_OK) {
      sprintf(err, "%s: hit an error skipping byte %d of %d: %s",
	      me, i, nio->byteSkip, BZ2_bzerror(bzfin, &bzerror));
      biffAdd(NRRD, err);
      return 1;
    }
  }
  
  /* bzip2 can handle data sizes up to INT_MAX, so we can't just 
     pass in the size, because it might be too large for an int.
     Therefore it must be read in chunks if the size is larger 
     than INT_MAX. */
  if (size <= INT_MAX) {
    block_size = (int)size;
  } else {
    block_size = INT_MAX;
  }

  /* This counter will help us to make sure that we read as much data
     as we think we should. */
  total_read = 0;
  /* Pointer to the blocks as we read them. */
  data = nrrd->data;
  
  /* Ok, now we can begin reading. */
  bzerror = BZ_OK;
  while ((read = BZ2_bzRead(&bzerror, bzfin, data, block_size))
	  && (BZ_OK == bzerror || BZ_STREAM_END == bzerror) ) {
    /* Increment the data pointer to the next available spot. */
    data += read;
    total_read += read;
    /* We only want to read as much data as we need, so we need to check
       to make sure that we don't request data that might be there but that
       we don't want.  This will reduce block_size when we get to the last
       block (which may be smaller than block_size).
    */
    if (size - total_read < block_size)
      block_size = (int)(size - total_read);
  }
  
  if (!( BZ_OK == bzerror || BZ_STREAM_END == bzerror )) {
    sprintf(err, "%s: error reading from BZFILE: %s",
	    me, BZ2_bzerror(bzfin, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }

  /* Close the BZFILE. */
  BZ2_bzReadClose(&bzerror, bzfin);
  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error closing BZFILE: %s", me,
	    BZ2_bzerror(bzfin, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }
  
  /* Check to see if we got out as much as we thought we should. */
  if (total_read != size) {
    sprintf(err, "%s: expected " _AIR_SIZE_T_FMT " bytes and received "
	    _AIR_SIZE_T_FMT " bytes",
	    me, size, total_read);
    biffAdd(NRRD, err);
    return 1;
  }
  
  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with bzip2 enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}

int
_nrrdEncodingBzip2_write(const Nrrd *nrrd, NrrdIoState *nio) {
  char me[]="_nrrdEncodingBzip2_write", err[AIR_STRLEN_MED];
#if TEEM_BZIP2
  size_t num, bsize, size, total_written;
  int block_size, bs, bzerror=BZ_OK;
  char *data;
  BZFILE* bzfout;

  if (nio->skipData) {
    return 0;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  num = nrrdElementNumber(nrrd);
  if (!num) {
    sprintf(err, "%s: calculated number of elements to be zero!", me);
    biffAdd(NRRD, err); return 1;
  }
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  if (num != bsize/nrrdElementSize(nrrd)) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  /* Set compression block size. */
  if (1 <= nio->bzip2BlockSize && nio->bzip2BlockSize <= 9) {
    bs = nio->bzip2BlockSize;
  } else {
    bs = 9;
  }
  /* Open bzfile for writing. Verbosity and work factor are set
     to default values. */
  bzfout = BZ2_bzWriteOpen(&bzerror, nio->dataFile, bs, 0, 0);
  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error opening BZFILE: %s", me, 
	    BZ2_bzerror(bzfout, &bzerror));
    biffAdd(NRRD, err);
    BZ2_bzWriteClose(&bzerror, bzfout, 0, NULL, NULL);
    return 1;
  }

  /* bzip2 can handle data sizes up to INT_MAX, so we can't just 
     pass in the size, because it might be too large for an int.
     Therefore it must be read in chunks if the size is larger 
     than INT_MAX. */
  if (size <= INT_MAX) {
    block_size = (int)size;
  } else {
    block_size = INT_MAX;
  }

  /* This counter will help us to make sure that we write as much data
     as we think we should. */
  total_written = 0;
  /* Pointer to the blocks as we write them. */
  data = nrrd->data;
  
  /* Ok, now we can begin writing. */
  bzerror = BZ_OK;
  while (size - total_written > block_size) {
    BZ2_bzWrite(&bzerror, bzfout, data, block_size);
    if (BZ_OK != bzerror) break;
    /* Increment the data pointer to the next available spot. */
    data += block_size; 
    total_written += block_size;
  }
  /* write the last (possibly smaller) block when its humungous data;
     write the whole data when its small */
  if (BZ_OK == bzerror) {
    block_size = (int)(size - total_written);
    BZ2_bzWrite(&bzerror, bzfout, data, block_size);
    total_written += block_size;
  }

  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error writing to BZFILE: %s",
	    me, BZ2_bzerror(bzfout, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }

  /* Close the BZFILE. */
  BZ2_bzWriteClose(&bzerror, bzfout, 0, NULL, NULL);
  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error closing BZFILE: %s", me,
	    BZ2_bzerror(bzfout, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }
  
  /* Check to see if we got out as much as we thought we should. */
  if (total_written != size) {
    sprintf(err, "%s: expected to write " _AIR_SIZE_T_FMT " bytes, but only "
	    "wrote " _AIR_SIZE_T_FMT,
	    me, size, total_written);
    biffAdd(NRRD, err);
    return 1;
  }
  
  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with bzip2 enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}

const NrrdEncoding
_nrrdEncodingBzip2 = {
  "bzip2",      /* name */
  "raw.bz2",   /* suffix */
  AIR_TRUE,    /* endianMatters */
  AIR_TRUE,   /* isCompression */
  _nrrdEncodingBzip2_available,
  _nrrdEncodingBzip2_read,
  _nrrdEncodingBzip2_write
};

const NrrdEncoding *const
nrrdEncodingBzip2 = &_nrrdEncodingBzip2;
