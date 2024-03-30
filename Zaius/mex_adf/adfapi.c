/*  adfapi.c
 *
 *
 *  ver 1.00 -1999 DLS
 *  ver 1.01 07-Oct-2002 YM/MPI moves APIs from adfapi2.c wrote by DAL
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "adfapi.h"
/*
 * Header support for streamer adf files
 */

static float adf_version = (float)ADF_VERSION;

/* The first magic number is for the original - raw data */
static char magic_numbers[] = { 10, 16, 19, 93 };

/* This magic number says the file has be reorganized by obs/channel */
static char magic_two[] = { 7, 8, 19, 67 };

void adf_freeHeader(ADF_HEADER *h)
{
  if (!h) return;

  if (!h->nobs) {
    free(h);
    return;
  }

  if (h->channeloffs) free(h->channeloffs);
  if (h->obscounts) free(h->obscounts);
  if (h->offsets) free(h->offsets);
  free(h);
}

ADF_HEADER *adf_readHeader(FILE *fp)
{
  ADF_HEADER *header = (ADF_HEADER *) calloc(1, ADF_HEADER_SIZE);
  char *h = (char *) header;
  int i;
  size_t status;

  status = fread(h, ADF_HEADER_SIZE, 1, fp);

  if (status == 1) {
    /* Check for magic number: 10 16 19 93 */
    if (h[0] == 10 && h[1] == 16 && 
	h[2] == 19 && h[3] == 93) {
      if (header->version == adf_version) {
	header->nobs = 0;
	return header;
      }
      /* Other byte format */
      else 
	return NULL;
    }
    /* Check for magic number two: 7 8 19 67 */
    if (h[0] == 7 && h[1] == 8 && 
	h[2] == 19 && h[3] == 67) {
      if (header->version == adf_version) {
	/* Get directory information */
	header->channeloffs = (int *) calloc(header->nchannels, sizeof(int));
	status = fread(header->channeloffs, sizeof(int), 
		       header->nchannels, fp);
	if (!status) goto error;

	header->obscounts = (int *) calloc(header->nobs, sizeof(int));
	status = fread(header->obscounts, sizeof(int), header->nobs, fp);
	if (!status) goto error;

	header->offsets = (int *) calloc(header->nobs, sizeof(int));
	status = fread(header->offsets, sizeof(int), header->nobs, fp);
	if (!status) goto error;

	/* correct a bug in which offsets[0] was not initialized to 0 */
	if (header->offsets[0] != 0) {
	  for (i = 0; i < header->nobs; i++) 
	    header->offsets[i] -= header->offsets[0];
	}

	return header;
      }
      /* Other byte format */
      else 
	return NULL;
    }
  }
error:
  if (header && header->channeloffs) free(header->channeloffs);
  if (header && header->obscounts) free(header->obscounts);
  if (header && header->offsets) free(header->offsets);
  if (header) free(header);
  return NULL;
}

ADF_DIR *adf_newDirectory(int n)
{
  ADF_DIR *d = (ADF_DIR *) calloc(1, sizeof(ADF_DIR));
  if (!d) return NULL;
  if (!(d->startblocks = (int *) calloc(n, sizeof(n)))) goto error;
  if (!(d->startoffs = (int *) calloc(n, sizeof(n)))) goto error;
  if (!(d->stopblocks = (int *) calloc(n, sizeof(n)))) goto error;
  if (!(d->stopoffs = (int *) calloc(n, sizeof(n)))) goto error;
  return d;

error:
  if (d->startblocks) free(d->startblocks);
  if (d->startoffs) free(d->startoffs);
  if (d->stopblocks) free(d->stopblocks);
  if (d->stopoffs) free(d->stopoffs);
  free(d);
  return NULL;
}

void adf_freeDirectory(ADF_DIR *d)
{
  if (d) {
    if (d->startblocks) free(d->startblocks);
    if (d->startoffs) free(d->startoffs);
    if (d->stopblocks) free(d->stopblocks);
    if (d->stopoffs) free(d->stopoffs);
    free(d);
  }
}

ADF_DIR *adf_createDirectory(FILE *fp, ADF_HEADER *h)
{
  ADF_DIR *d;
  int i, j = 0, n, obscnt = 0;
  int allocsize = 1024;
  int obs_alloced = allocsize;
  int *startblocks;
  int *stopblocks;
  int *startoffs;
  int *stopoffs;
  
  short *buffer;
  int size = h->nchannels*h->numconv;
  double ms_per_sample = h->us_per_sample/1000.;

  startblocks = (int *) calloc(obs_alloced, sizeof(int));
  startoffs = (int *) calloc(obs_alloced, sizeof(int));
  stopblocks = (int *) calloc(obs_alloced, sizeof(int));
  stopoffs = (int *) calloc(obs_alloced, sizeof(int));

  buffer = (short *) calloc(size, sizeof(short));
  if (!buffer) {
    fprintf(stderr, "adfapi: out of memory\n");
    return 0;
  }

  while ((n = (int)fread(buffer, sizeof(short), size, fp)) > 0) {
    /* We only need to check the first channel for the on/off markers */
    for (i = 0; i < h->numconv; i++) {
      if (buffer[i] & (1 << 15)) {
        startblocks[obscnt] = j;
        startoffs[obscnt] = i;
      }
      else if (buffer[i] & (1 << 14)) {
        stopblocks[obscnt] = j;
        stopoffs[obscnt] = i;
        obscnt++;
        
        /* Make sure there's still enough space */
        if (obscnt == obs_alloced) {
          obs_alloced += allocsize;
          startblocks = (int *) realloc(startblocks, sizeof(int)*obs_alloced);
          startoffs = (int *) realloc(startoffs, sizeof(int)*obs_alloced);
          stopblocks = (int *) realloc(stopblocks, sizeof(int)*obs_alloced);
          stopoffs = (int *) realloc(stopoffs, sizeof(int)*obs_alloced);
        }
      }
    }
    j++;
  }

  d = adf_newDirectory(obscnt);
  h->nobs = d->nobs = obscnt;

  for (i = 0; i < obscnt; i++) {
    d->startblocks[i] = startblocks[i];
    d->stopblocks[i] = stopblocks[i];
    d->startoffs[i] = startoffs[i];
    d->stopoffs[i] = stopoffs[i];
  }

  free(startblocks);
  free(startoffs);
  free(stopblocks);
  free(stopoffs);
  return d;
}

int adf_convertFile(ADF_HEADER *h, ADF_DIR *d, FILE *fp, FILE *ofp)
{
  char buf[ADF_HEADER_SIZE];
  unsigned short *samples, *s;
  int i, j, k, n, offset = 0, max = 0, bsize;
  h->channeloffs = (int *) calloc(h->nchannels, sizeof(int));
  h->obscounts = (int *) calloc(h->nobs, sizeof(int));
  h->offsets = (int *) calloc(h->nobs, sizeof(int));
  
  /* Create the new header counts/offsets */
  for (i = 0; i < h->nobs; i++) {
    n = (d->stopblocks[i]*h->numconv+d->stopoffs[i])-
      (d->startblocks[i]*h->numconv+d->startoffs[i]);
    
    h->obscounts[i] = n;
    h->offsets[i] = offset;
    if (n > max) max = n;	/* keep track of longest obsp */
    offset += n*sizeof(short);
  }
  
  /* Add offset to beginning of each channel's data */
  for (i = 0; i < h->nchannels; i++) {
    h->channeloffs[i] = offset*i+
      ADF_HEADER_SIZE+(sizeof(int)*(h->nchannels+2*h->nobs));
  }

  /*  Make room for largest obs period */
  samples = (unsigned short *) calloc(max, sizeof(short));
  if (!samples) {
    free(h->channeloffs);
    free(h->obscounts);
    free(h->offsets);
    return 0;
  }

  /* And now write out the header + directory */
  h->magic[0] = magic_two[0];
  h->magic[1] = magic_two[1];
  h->magic[2] = magic_two[2];
  h->magic[3] = magic_two[3];
  memset(buf, 0, ADF_HEADER_SIZE);
  memcpy(buf, h, sizeof(ADF_HEADER));

  fwrite(buf, 1, ADF_HEADER_SIZE, ofp);
  fwrite(h->channeloffs, sizeof(int), h->nchannels, ofp);
  fwrite(h->obscounts, sizeof(int), h->nobs, ofp);
  fwrite(h->offsets, sizeof(int), h->nobs, ofp);
  
  bsize = h->numconv*sizeof(short);
  for (i = 0; i < h->nchannels; i++) {
    for (j = 0; j < h->nobs; j++) {
      s = samples;

      /* special case (data all in one block) */
      if (d->startblocks[j] == d->stopblocks[j]) {
        offset = 
          ADF_HEADER_SIZE+
          ((d->startblocks[j]*h->nchannels+i)*bsize) +
          d->startoffs[j]*2;
        fseek(fp, offset, SEEK_SET);
        fread(s, sizeof(short), d->stopoffs[j]-d->startoffs[j], fp);
      }
      else {
        /* Get first block */
        offset = 
          ADF_HEADER_SIZE+
          ((d->startblocks[j]*h->nchannels+i)*bsize) +
          d->startoffs[j]*2;
        fseek(fp, offset, SEEK_SET);
        fread(s, sizeof(short), h->numconv-d->startoffs[j], fp);
        s+=h->numconv-d->startoffs[j];
	
        /* Get middle blocks */
        for (k = d->startblocks[j]+1; k < d->stopblocks[j]; k++) {
          offset = ADF_HEADER_SIZE+(k*h->nchannels+i)*bsize;
          fseek(fp, offset, SEEK_SET);
          fread(s, sizeof(short), h->numconv, fp);
          s+=h->numconv;
        }
        
        /* Get last block */
        offset = ADF_HEADER_SIZE+(d->stopblocks[j]*h->nchannels+i)*bsize;
        fseek(fp, offset, SEEK_SET);
        fread(s, sizeof(short), d->stopoffs[j], fp);
        s+=d->stopoffs[j];
      }
      
      /* Get rid of bit marker */
      samples[0] &= 0x3FFF;
      samples[h->obscounts[j]-1] &= 0x3FFF;

      /* Finally write it out */
      fwrite(samples, sizeof(short), h->obscounts[j], ofp);
    }
  }
  
  free(samples);
  return 1;
}

/*
 * thresholdObs: loop through an entire obs, searching for trigger
 *               regions where the signal passes threshold in the
 *               supplied direction (1: pos, -1: neg).
 *         
 *               Resulting samples are returned in a linear array
 *               with each region consisting of nsamps samples
 *
 *               If no regions are found, vals/times are set to NULL
 */

int adf_thresholdObs(ADF_HEADER *h, FILE *fp, int channel, int obsp,
		     int threshold, int dir, float pre, float post, 
		     float skiptime,
		     int *nsamps, int *nregs, short **regvals, float **times) 
{
#define MAX_STATIC_INDICES 1024
  static int indices[MAX_STATIC_INDICES];
  int alloced_indices = MAX_STATIC_INDICES;
  int *inds = indices;
  int i, j, nregions = 0, ns;
  short *vals = NULL, *v, *vp;
  float *t = NULL;
  float ms_per_sample = (float)(h->us_per_sample/1000.);
  int presamps = 1 + (int)(pre / ms_per_sample);
  int postsamps = (int)(post / ms_per_sample);
  int skipsamps = (int)(skiptime / ms_per_sample);
  int n = presamps+postsamps;
  int stop;
  int retval = -1;

  if (adf_getObsPeriod(h, fp, channel, obsp, &ns, &vals) != 1) return 0;
  if (ns <= n) goto done;

  stop = ns - postsamps;

  /* Find trigger events */
  if (dir > 0) {		/* positive slope */
    for (i = presamps; i < stop; i++) {
      if (vals[i] < threshold && vals[i+1] >= threshold) {
	/* We've run out of space */
	if (nregions == alloced_indices) {
	  alloced_indices += MAX_STATIC_INDICES;
	  if (inds == indices) {
	    /* need to alloc for the first time*/	  
	    inds = calloc(alloced_indices, sizeof(int));
	    memcpy(inds, indices, sizeof(indices));
	  }
	  else {
	    /* realloc */
	    inds = realloc(inds, sizeof(int)*alloced_indices);
	  }
	}
	inds[nregions++] = i;
	i += skipsamps;
      }
    }
  }
  else {			/* negative slope */
    for (i = presamps; i < stop; i++) {
      if (vals[i] > threshold && vals[i+1] <= threshold) {
	/* We've run out of space */
	if (nregions == alloced_indices) {
	  alloced_indices += MAX_STATIC_INDICES;
	  if (inds == indices) {
	    /* need to alloc for the first time*/	  
	    inds = calloc(alloced_indices, sizeof(int));
	    memcpy(inds, indices, sizeof(indices));
	  }
	  else {
	    /* realloc */
	    inds = realloc(inds, sizeof(int)*alloced_indices);
	  }
	}
	inds[nregions++] = i;
	i += skipsamps;
      }
    }
  }
  
  if (!nregions) {
    retval = 0;
    goto done;
  }

  /* Allocate space for return info */
  vp = v = (short *) calloc(n*nregions, sizeof(short));
  t = (float *) calloc(nregions, sizeof(float));

  /* Now extract trigger times and regions */
  for (i = 0; i < nregions; i++) {
    t[i] = (float)(inds[i]*ms_per_sample);
    stop = inds[i]+postsamps;
    for (j = inds[i]-presamps; j < stop; j++) {
      *vp++ = vals[j];
    }
  }
  
  if (inds != indices) free(inds);
  free(vals);
  retval = nregions;

 done:
  if (nsamps) *nsamps = n;
  if (nregs) *nregs = nregions;
  if (regvals) *regvals = v;
  if (times) *times = t;
  return retval;
  
#undef MAX_STATIC_INDICES
}

int adf_getObsPeriod(ADF_HEADER *h, FILE *fp, int channel, int obsp,
		     int *nsamps, short **vals)
{
  short *v;

  if (channel >= h->nchannels) return -1;
  if (obsp >= h->nobs) return -2;

  fseek(fp, h->channeloffs[channel]+h->offsets[obsp], SEEK_SET);

  //printf("\n chanoffs=%d, obscounts=%d, offsets=%d",
  //       h->channeloffs[channel],h->obscounts[obsp],h->offsets[obsp]);

  v = (short *) calloc(h->obscounts[obsp], sizeof(short));
  fread(v, sizeof(short), h->obscounts[obsp], fp);
  *nsamps = h->obscounts[obsp];
  *vals = v;
  return 1;
}

int adf_getObsPeriodPartial(ADF_HEADER *h, FILE *fp, int channel, int obsp,
		     int startindx, int totindx, int *nsamps, short **vals)
{
  short *v;

  if (channel >= h->nchannels) return -1;
  if (obsp >= h->nobs) return -2;
  if (totindx < 0)  totindx = h->obscounts[obsp]-startindx;
  if (startindx < 0 || startindx+totindx > h->obscounts[obsp]) {
    fprintf(stderr, "Invalid indexes [%d (tot %d), with obs %d long]\n", startindx, totindx, h->obscounts[obsp]);
    return -3;
  }
  fseek(fp, h->channeloffs[channel]+h->offsets[obsp]+startindx*sizeof(short), SEEK_SET);
  
  v = (short *) calloc(totindx, sizeof(short));
  fread(v, sizeof(short), totindx, fp);
  *nsamps = totindx;
  *vals = v;
  return 1;
}

void adf_printInfo(ADF_HEADER *h, FILE *fp)
{
  printf("%d Channels / %d Obs Periods / %d\n", h->nchannels, h->nobs,
	 h->offsets[0]);
}

void adf_printDirectory(ADF_HEADER *h, ADF_DIR *d)
{
  int i, n;
  double ms_per_sample = h->us_per_sample/1000.;

  if (!d) return;
  
  for (i = 0; i < d->nobs; i++) {
    n = (d->stopblocks[i]*h->numconv+d->stopoffs[i])-
      (d->startblocks[i]*h->numconv+d->startoffs[i]);
    printf("%d %.2fms [%d]\n", i, n*ms_per_sample);
  }
}


ADF_DIR *adf_readDir(FILE *fp, int offset)
{
  ADF_DIR *dir = (ADF_DIR *) calloc(1, ADF_DIR_SIZE);
  char *d = (char *) dir;
  size_t status;
  fseek(fp, offset, SEEK_SET);
  status = fread(d, ADF_DIR_SIZE, 1, fp);

  if (status == 1) {
	dir->startblocks = (int *) calloc(dir->nobs, sizeof(int));
	status = fread(dir->startblocks, sizeof(int), dir->nobs, fp);
	if (!status) goto error;

	dir->startoffs = (int *) calloc(dir->nobs, sizeof(int));
	status = fread(dir->startoffs, sizeof(int), dir->nobs, fp);
	if (!status) goto error;

	dir->stopblocks = (int *) calloc(dir->nobs, sizeof(int));
	status = fread(dir->stopblocks, sizeof(int), dir->nobs, fp);
	if (!status) goto error;

	dir->stopoffs = (int *) calloc(dir->nobs, sizeof(int));
	status = fread(dir->stopoffs, sizeof(int), dir->nobs, fp);
	if (!status) goto error;
	
	return dir;
  } else {
    return NULL;
  }
error:
  if (dir && dir->startblocks) free(dir->startblocks);
  if (dir && dir->startoffs) free(dir->startoffs);
  if (dir && dir->stopblocks) free(dir->stopblocks);
  if (dir && dir->stopoffs) free(dir->stopoffs);
  if (dir) free(dir);
  return NULL;
}

int adf_mkConvInfoFile(ADF_HEADER *h, ADF_DIR *d, FILE *fp, FILE *ofp)
{
  char buf[ADF_HEADER_SIZE];
  int i, n, offset = 0;
  h->channeloffs = (int *) calloc(h->nchannels, sizeof(int));
  h->obscounts = (int *) calloc(h->nobs, sizeof(int));
  h->offsets = (int *) calloc(h->nobs, sizeof(int));
  
  /* Create the new header counts/offsets */
  for (i = 0; i < h->nobs; i++) {
    n = (d->stopblocks[i]*h->numconv+d->stopoffs[i])-
      (d->startblocks[i]*h->numconv+d->startoffs[i]);
    h->obscounts[i] = n;
    h->offsets[i] = offset;
    offset += n*sizeof(short);
  }
  

  /* NOTE THAT THIS IS ALL CHANGED FROM THE ORIGINAL VERSION */
  /* And now write out the header + directory */
  h->magic[0] = magic_two[0];
  h->magic[1] = magic_two[1];
  h->magic[2] = magic_two[2];
  h->magic[3] = magic_two[3];
  memset(buf, 0, ADF_HEADER_SIZE);
  memcpy(buf, h, sizeof(ADF_HEADER));

  /* FIRST WRITE THE HEADER, THEN THE OBSCOUNTS AND OFFSETS */
  fwrite(buf, 1, ADF_HEADER_SIZE, ofp);
  fwrite(h->channeloffs, sizeof(int), h->nchannels, ofp);
  fwrite(h->obscounts, sizeof(int), h->nobs, ofp);
  fwrite(h->offsets, sizeof(int), h->nobs, ofp);
  
  /* AND FINALLY THE DIRECTORY, THE BLOCKS AND OFFSETS */
  memset(buf, 0, ADF_DIR_SIZE);
  memcpy(buf, d, sizeof(ADF_DIR));
  fwrite(buf, 1, ADF_DIR_SIZE, ofp);
  fwrite(d->startblocks,sizeof(int),h->nobs,ofp);
  fwrite(d->startoffs,sizeof(int),h->nobs,ofp);
  fwrite(d->stopblocks,sizeof(int),h->nobs,ofp);
  fwrite(d->stopoffs,sizeof(int),h->nobs,ofp);
  
  return 1;
}

int adf_getObsPeriodFromBlocks(ADF_HEADER *h, ADF_DIR *d, FILE *fp, 
			       int channel, int obsp,int *nsamps, short **vals)
{
  int bsize,offset = 0,k,n;
  unsigned short *s;

  if (channel >= h->nchannels) return 0;
  if (obsp >= h->nobs) return 0;

  n = (d->stopblocks[obsp]*h->numconv+d->stopoffs[obsp])-
    (d->startblocks[obsp]*h->numconv+d->startoffs[obsp]);

  *vals = (unsigned short *) calloc(n, sizeof(short));
  
  bsize = h->numconv*sizeof(short);
  s = *vals;
  /* special case (data all in one block) */
  if (d->startblocks[obsp] == d->stopblocks[obsp]) {
    offset = 
      ADF_HEADER_SIZE+
      ((d->startblocks[obsp]*h->nchannels+channel)*bsize) +
      d->startoffs[obsp]*2;
    fseek(fp, offset, SEEK_SET);
    fread(s, sizeof(short), d->stopoffs[obsp]-d->startoffs[obsp], fp);
  }
  else {
    /* Get first block */
    offset = 
      ADF_HEADER_SIZE+
      ((d->startblocks[obsp]*h->nchannels+channel)*bsize) +
      d->startoffs[obsp]*2;
    fseek(fp, offset, SEEK_SET);
    fread(s, sizeof(short), h->numconv-d->startoffs[obsp], fp);
    s+=h->numconv-d->startoffs[obsp];
    
    /* Get middle blocks */
    for (k = d->startblocks[obsp]+1; k < d->stopblocks[obsp]; k++) {
      offset = ADF_HEADER_SIZE+(k*h->nchannels+channel)*bsize;
      fseek(fp, offset, SEEK_SET);
      fread(s, sizeof(short), h->numconv, fp);
      s+=h->numconv;
    }
    
    /* Get last block */
    offset = ADF_HEADER_SIZE+(d->stopblocks[obsp]*h->nchannels+channel)*bsize;
    fseek(fp, offset, SEEK_SET);
    fread(s, sizeof(short), d->stopoffs[obsp], fp);
    s+=d->stopoffs[obsp];
  }

  /* Get rid of bit marker */
  (*vals)[0] &= 0x3FFF;
  (*vals)[n-1] &= 0x3FFF;
  *nsamps = n;
  return 1;
}

int adf_getPartialObsPeriodFromBlocks(ADF_HEADER *h, ADF_DIR *d, FILE *fp, 
				      int channel, int obsp, int start_samp, 
				      int samp_dur,int *nsamps, short **vals)
{
  int bsize,offset = 0,k,n;
  unsigned short *s, *tmps;

  if (channel >= h->nchannels) return 0;
  if (obsp >= h->nobs) return 0;

  n = (d->stopblocks[obsp]*h->numconv+d->stopoffs[obsp])-
    (d->startblocks[obsp]*h->numconv+d->startoffs[obsp]);

  if (samp_dur == -1) samp_dur = n;
  else if (samp_dur > n) {
    printf("adf_getPartialObsPeriodFromBlocks: Samp dur too large, truncating");
    samp_dur = n;
  }
  *vals = (unsigned short *) calloc(n, sizeof(short));
  tmps  = (unsigned short *) calloc(n, sizeof(short));
  
  bsize = h->numconv*sizeof(short);
  s = tmps;
  /* special case (data all in one block) */
  if (d->startblocks[obsp] == d->stopblocks[obsp]) {
    offset = 
      ADF_HEADER_SIZE+
      ((d->startblocks[obsp]*h->nchannels+channel)*bsize) +
      d->startoffs[obsp]*2;
    fseek(fp, offset, SEEK_SET);
    fread(s, sizeof(short), d->stopoffs[obsp]-d->startoffs[obsp], fp);
  }
  else {
    /* Get first block */
    offset = 
      ADF_HEADER_SIZE+
      ((d->startblocks[obsp]*h->nchannels+channel)*bsize) +
      d->startoffs[obsp]*2;
    fseek(fp, offset, SEEK_SET);
    fread(s, sizeof(short), h->numconv-d->startoffs[obsp], fp);
    s+=h->numconv-d->startoffs[obsp];
    
    /* Get middle blocks */
    for (k = d->startblocks[obsp]+1; k < d->stopblocks[obsp]; k++) {
      offset = ADF_HEADER_SIZE+(k*h->nchannels+channel)*bsize;
      fseek(fp, offset, SEEK_SET);
      fread(s, sizeof(short), h->numconv, fp);
      s+=h->numconv;
    }
    
    /* Get last block */
    offset = ADF_HEADER_SIZE+(d->stopblocks[obsp]*h->nchannels+channel)*bsize;
    fseek(fp, offset, SEEK_SET);
    fread(s, sizeof(short), d->stopoffs[obsp], fp);
    s+=d->stopoffs[obsp];
  }
  
  /* Get rid of bit marker */
  tmps[0] &= 0x3FFF;
  tmps[n-1] &= 0x3FFF;
  memcpy(*vals,&tmps[start_samp],samp_dur*sizeof(short));
  *nsamps = samp_dur;
  free(tmps);
  return 1;
}


#ifdef STAND_ALONE
int main(int argc, char **argv)
{
  ADF_HEADER *h;
  ADF_DIR *d;
  FILE *fp;

  if (argc < 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    exit(0);
  }

  fp = fopen(argv[1], "rb");
  if (!fp) {
    fprintf(stderr, "%s: unable to open file %s\n", argv[1]);
    exit(-1);
  }

  h = adf_readHeader(fp);
  if (!h) {
    fprintf(stderr, "%s: unable to read adf file %s\n", argv[0], argv[1]);
    return 1;
  }

  if (h->nobs == 0) {		/* Needs to be converted */
    FILE *ofp;
    if (argc < 3) ofp = stdout;
    else ofp = fopen(argv[2], "wb");
    if (!ofp) {
      fprintf(stderr, "%s: error opening output file\n", argv[0]);
      adf_freeHeader(h);
      fclose(fp);
    }

    d = adf_createDirectory(fp, h);
    adf_convertFile(h, d, fp, ofp);
    if (ofp != stdout) fclose(ofp);
  }

  else {
    adf_printInfo(h, fp);
  }

  adf_freeHeader(h);
  fclose(fp);
  return 0;
}
#endif

