/*	adfwapi.c
 *	
 *	ver 1.00  30-May-2000 Yusuke MURAYAMA, MPI : derived from adfapi.c
 *      1.01  01-Sep-2000 YM, adds 'checkXXX' functions
 *      1.02  17-Oct-2001 YM, make THRESHOLD adjustable
 *                            update ADFW_VERSION 1.00 to 1.01, adding threshold info to header
 *                            change THRESHOLD 23000(3.5V) to 16000(2.5V)
 *		  1.03  26-Oct-2001 YM/DAL, new API called adfw_getObsPeriodPartial() by DAL
 *		  1.04  07-Oct-2002 YM, adds 'adfw_' prefix to some APIs
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h> 
#include "adfwapi.h"

/*
 * Header support for streamer adf files
 */

#define MAX_READ_BUFFER_SIZE (100000)	/* will be multiplied by nchannels */
										/* for examples, 100000*16chans*2byte = 3.2Mbyte */

static float adfw_version = (float)ADFW_VERSION;

/* The first magic number is for the original - raw data */
static char magic_numbers2[4] = { 11, 17, 20, 94 };

/* This magic number says the file has be reorganized by obs/channel */
static char magic_two2[4] = { 8, 9, 20, 68 };

void adfw_freeHeader(ADFW_HEADER *h)
{
	if (!h) return;

	if (!h->nobs) {
		free(h); h = NULL;
		return;
	}

	if (h->channeloffs)	free(h->channeloffs);
	if (h->obscounts)	free(h->obscounts);
	if (h->offsets)		free(h->offsets);
	free(h);	h = NULL;
}

void adfw_initHeader(ADFW_HEADER *h, int converted)
{
	if (h == NULL)	return;
	 memset(h, 0, sizeof(ADFW_HEADER));
	if (converted) {
		h->magic[0] = magic_two2[0];
		h->magic[1] = magic_two2[1];
		h->magic[2] = magic_two2[2];
		h->magic[3] = magic_two2[3];
	} else {
		h->magic[0] = magic_numbers2[0];
		h->magic[1] = magic_numbers2[1];
		h->magic[2] = magic_numbers2[2];
		h->magic[3] = magic_numbers2[3];
	}
	h->version = adfw_version;

	return;
}

ADFW_HEADER *adfw_readHeader(FILE *fp)
{
	ADFW_HEADER *header = (ADFW_HEADER *) calloc(1, ADFW_HEADER_SIZE);
	char *h = (char *) header;
	int i;
	size_t status;

	fseek(fp, 0, SEEK_SET);
	status = fread(h, ADFW_HEADER_SIZE, 1, fp);

	if (status == 1) {
		/* Check for magic number: 11 17 20 94 */
		if (h[0] == 11 && h[1] == 17 && h[2] == 20 && h[3] == 94) {
			if (header->version <= adfw_version) {
				header->nobs = 0;
				return header;
			}
			/* Other byte format */
			else {
				free(header);
				return NULL;
			}
		}
	    /* Check for magic number two: 8 9 20 68 */
		if (h[0] == 8 && h[1] == 9 && h[2] == 20 && h[3] == 68) {
			if (header->version <= adfw_version) {
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
			else {
				free(header);
				return NULL;
			}
		}
	}
error:
  if (header && header->channeloffs) free(header->channeloffs);
  if (header && header->obscounts) free(header->obscounts);
  if (header && header->offsets) free(header->offsets);
  if (header) free(header);
  return NULL;
}

ADFW_DIR *adfw_newDirectory(int n)
{
  ADFW_DIR *d = (ADFW_DIR *) calloc(1, sizeof(ADFW_DIR));
  if (!d) return NULL;
  if (!(d->startoffs = (int *) calloc(n, sizeof(n)))) goto error;
  if (!(d->stopoffs = (int *) calloc(n, sizeof(n)))) goto error;
  return d;

error:
  if (d->startoffs) free(d->startoffs);
  if (d->stopoffs) free(d->stopoffs);
  free(d);
  return NULL;
}

void adfw_freeDirectory(ADFW_DIR *d)
{
  if (d) {
    if (d->startoffs) free(d->startoffs);
    if (d->stopoffs) free(d->stopoffs);
    free(d);
  }
}

ADFW_DIR *adfw_createDirectory(FILE *fp, ADFW_HEADER *h)
{
	return adfw_createDirectoryEx(fp,h,DAQ_ANALOG_HIGH,DAQ_ANALOG_LOW);
}

ADFW_DIR *adfw_createDirectoryEx(FILE *fp, ADFW_HEADER *h, short logicH, short logicL)
{
	ADFW_DIR *d;
	int i, j = 0, n, obscnt = 0;
	int allocsize = 1024;
	int obs_alloced = allocsize;
	int *startoffs;
	int *stopoffs;

	short *buffer;
//  int size = h->nchannels*h->numconv;
	int size = h->nchannels*MAX_READ_BUFFER_SIZE;
	double ms_per_sample = h->us_per_sample/1000.;
	short prevLevel = 0;

	h->trig_logic_high = logicH;
	h->trig_logic_low  = logicL;

	startoffs = (int *) calloc(obs_alloced, sizeof(int));
	stopoffs = (int *) calloc(obs_alloced, sizeof(int));

	buffer = (short *) calloc(size, sizeof(short));
	if (!buffer) {
		fprintf(stderr, "adfwapi: out of memory\n");
		return 0;
	}

	fseek(fp, ADFW_HEADER_SIZE, SEEK_SET);
	prevLevel = logicL-1;
	while ((n = (int)fread(buffer, sizeof(short), size, fp)) > 0) {
		//printf("\n n=%d/%d",n,size);
		/* We only need to check the trigger channel for the on/off markers */
		for (i = h->nchannels-1; i < n; i+=h->nchannels) {
			if (buffer[i] > logicH && prevLevel < logicL) {
				// goes Low to High
				startoffs[obscnt] = i-h->nchannels+1 + j;	// index for the first channel
				prevLevel = logicH+1;
				//printf("\n %8d % 6d",startoffs[obscnt],buffer[i]);
			} else if (buffer[i] < logicL && prevLevel > logicH) {
				// goes High to Low
				stopoffs[obscnt] = i-h->nchannels+1 + j;	// index for the first channel
				//printf("\n %8d % 6d: %d",stopoffs[obscnt],buffer[i],(stopoffs[obscnt]-startoffs[obscnt])/h->nchannels);
				obscnt++;
				prevLevel = logicL-1;
        		/* Make sure there's still enough space */
				if (obscnt == obs_alloced) {
					obs_alloced += allocsize;
					startoffs = (int *) realloc(startoffs, sizeof(int)*obs_alloced);
					stopoffs = (int *) realloc(stopoffs, sizeof(int)*obs_alloced);
				}
			}
			//if (obscnt == 4) printf("\n %8d %d", i,buffer[i]);
		}
		j += n;
	}

	d = adfw_newDirectory(obscnt);
	h->nobs = d->nobs = obscnt;

	for (i = 0; i < obscnt; i++) {
		d->startoffs[i] = startoffs[i];
		d->stopoffs[i] = stopoffs[i];
	}

	free(startoffs);
	free(stopoffs);
	return d;
}

int adfw_convertFile(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, FILE *ofp)
{
	char buf[ADFW_HEADER_SIZE];
	short *readbuf, *samples;
	int i, j, k, k0, kr, nr, ir, n, offset = 0, max = 0, bsize;
	int nChan;

	nChan = h->nchannels;
	h->nchannels = h->nchannels - 1;	// we don't need the last channel.

	h->channeloffs = (int *) calloc(h->nchannels, sizeof(int));
	h->obscounts = (int *) calloc(h->nobs, sizeof(int));
	h->offsets = (int *) calloc(h->nobs, sizeof(int));
  
	/* Create the new header counts/offsets */
	for (i = 0; i < h->nobs; i++) {
		n = (d->stopoffs[i] - d->startoffs[i])/nChan;
		h->obscounts[i] = n;
		h->offsets[i] = offset;
		if (n > max) max = n;	/* keep track of longest obsp */
		offset += n*sizeof(short);
	}
  
	/* Add offset to beginning of each channel's data */
	for (i = 0; i < h->nchannels; i++) {
		h->channeloffs[i] = offset*i+
				ADFW_HEADER_SIZE+(sizeof(int)*(h->nchannels+2*h->nobs));
	}

	/*  Make room for largest obs period */
	readbuf = (short *) calloc(MAX_READ_BUFFER_SIZE*nChan, sizeof(short));
	samples = (short *) calloc(MAX_READ_BUFFER_SIZE, sizeof(short));
	if (!readbuf || !samples) {
		if (readbuf) free(readbuf);
		free(h->channeloffs);
		free(h->obscounts);
		free(h->offsets);
		return 0;
	}

	/* And now write out the header + directory */
	h->magic[0] = magic_two2[0];
	h->magic[1] = magic_two2[1];
	h->magic[2] = magic_two2[2];
	h->magic[3] = magic_two2[3];
	memset(buf, 0, ADFW_HEADER_SIZE);
	memcpy(buf, h, sizeof(ADFW_HEADER));

	fwrite(buf, 1, ADFW_HEADER_SIZE, ofp);
	fwrite(h->channeloffs, sizeof(int), h->nchannels, ofp);
	fwrite(h->obscounts, sizeof(int), h->nobs, ofp);
	fwrite(h->offsets, sizeof(int), h->nobs, ofp);
  
	bsize = MAX_READ_BUFFER_SIZE*nChan;
	for (i = 0; i < h->nchannels; i++) {
		for (j = 0; j < h->nobs; j++) {
			offset = ADFW_HEADER_SIZE + (d->startoffs[j] * sizeof(short));
			fseek(fp, offset, SEEK_SET);

			k0 = h->obscounts[j]%MAX_READ_BUFFER_SIZE;
			nr = h->obscounts[j]/MAX_READ_BUFFER_SIZE;
			fread(readbuf,sizeof(short),k0*nChan,fp);
			for (k=0, kr=i; k<k0; k++, kr+=nChan)	samples[k] = readbuf[kr];
			fwrite(samples, sizeof(short), k0, ofp);
			for (ir=0; ir<nr; ir++) {
				fread(readbuf,sizeof(short),bsize,fp);
				for (k=0,kr=i; k<MAX_READ_BUFFER_SIZE; k++, kr+=nChan)	samples[k] = readbuf[kr];
				fwrite(samples, sizeof(short), MAX_READ_BUFFER_SIZE, ofp);
			}
			//printf(" %d:",h->obscounts[j]);	
			//if (nr) {
			//	printf("%d:%d ",readbuf[MAX_READ_BUFFER_SIZE*nChan-(nChan-i)],readbuf[MAX_READ_BUFFER_SIZE*nChan-1]);
			//} else {
			//	printf("%d:%d ",readbuf[k0*nChan-(nChan-i)],readbuf[k0*nChan-1]);
			//}
		}
		//printf("\n");
#ifdef _DEBUG
		printf(".");
#endif
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

int adfw_thresholdObs(ADFW_HEADER *h, FILE *fp, int channel, int obsp,
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
  double ms_per_sample = h->us_per_sample/1000.;
  int presamps = (int)(1 + (pre / ms_per_sample));
  int postsamps = (int)(post / ms_per_sample);
  int skipsamps = (int)(skiptime / ms_per_sample);
  int n = presamps+postsamps;
  int stop;
  int retval = -1;

  if (adfw_getObsPeriod(h, fp, channel, obsp, &ns, &vals) != 1) return 0;
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

int adfw_getObsPeriod(ADFW_HEADER *h, FILE *fp, int channel, int obsp,
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

/* adfw_getObsPeriodPartial by DAL, 26-Oct-2001 */
int adfw_getObsPeriodPartial(ADFW_HEADER *h, FILE *fp, int channel, int obsp,
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

void adfw_printInfo(ADFW_HEADER *h, FILE *fp)
{
  printf("%d Channels / %d Obs Periods / %d\n", h->nchannels, h->nobs,
	 h->offsets[0]);
}

void adfw_printDirectory(ADFW_HEADER *h, ADFW_DIR *d)
{
  int i, n;
  float ms_per_sample = (float)(h->us_per_sample/1000.);

  if (!d) return;
  
  for (i = 0; i < d->nobs; i++) {
    n = (d->stopoffs[i] - d->startoffs[i])/h->nchannels;
    printf("%d %.2fms [%d]\n", i, n*ms_per_sample);
  }
}

int adfw_getFileFormat(FILE *fp)
{
  char m[4];
  size_t status;

  if (fp == NULL)  return -1;
  memset(m, 0, 4);
  fseek(fp,0,SEEK_SET);
  status = fread(m, 4, sizeof(char), fp);
  fseek(fp,0,SEEK_SET);
	if (status > 0) {
    return adfw_checkMagicNumber(m);
  } else {
    return -1;
  }
  return -1;
}

int adfw_checkFileFormat(char *fname)
{
	FILE *fp;
	size_t status;
	char m[4];

  memset(m, 0, 4);
#ifdef _WIN32
	if (fopen_s(&fp, fname, "rb") != 0) {
		fprintf(stderr, "adfw_checkFileFormat(): unable to open file %s\n", fname);
		return -1;
	}
#else
	fp = fopen(fname, "rb");
	if (!fp) {
		fprintf(stderr, "adfw_checkFileFormat(): unable to open file %s\n", fname);
		return -1;
	}
#endif

	status = fread(m, 4, sizeof(char), fp);
	fclose(fp);
	if (status > 0) {
		return adfw_checkMagicNumber(m);
	}
	return ADF_UNKNOWN;
}

int adfw_checkMagicNumber(char *m)
{
  /* Check for magic number: 10 16 19 93 */
  if (m[0] == 10 && m[1] == 16 && m[2] == 19 && m[3] == 93)
    return ADF_WIN30_UNCONV;
  /* Check for magic number:  7  8 19 67 */
  if (m[0] ==  7 && m[1] ==  8 && m[2] == 19 && m[3] == 67)
    return ADF_WIN30_CONV;
  /* Check for magic number: 11 17 20 94 */
  if (m[0] == 11 && m[1] == 17 && m[2] == 20 && m[3] == 94)
    return ADF_PCI6052E_UNCONV;
  /* Check for magic number:  8  9 20 68 */
  if (m[0] ==  8 && m[1] ==  9 && m[2] == 20 && m[3] == 68)
    return ADF_PCI6052E_CONV;

	return ADF_UNKNOWN;
}

int adfw_mkConvInfoFile(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, FILE *ofp)
{
  char buf[ADFW_HEADER_SIZE];
  int i, n, offset = 0;

  h->channeloffs = (int *) calloc(h->nchannels, sizeof(int));
  h->obscounts = (int *) calloc(h->nobs, sizeof(int));
  h->offsets = (int *) calloc(h->nobs, sizeof(int));
  
  /* Create the new header counts/offsets */
  for (i = 0; i < h->nobs; i++) {
    n = (d->stopoffs[i] - d->startoffs[i])/h->nchannels;
    h->obscounts[i] = n;
    h->offsets[i] = offset;
    offset += n*sizeof(short);
  }
	/* Add offset to beginning of each channel's data */
	for (i = 0; i < h->nchannels; i++) {
		h->channeloffs[i] = offset*i+
				ADFW_HEADER_SIZE+(sizeof(int)*(h->nchannels+2*h->nobs));
	}


  /* NOTE THAT THIS IS ALL CHANGED FROM THE ORIGINAL VERSION */
  /* And now write out the header + directory */
  h->magic[0] = magic_two2[0];
  h->magic[1] = magic_two2[1];
  h->magic[2] = magic_two2[2];
  h->magic[3] = magic_two2[3];
  memset(buf, 0, ADFW_HEADER_SIZE);
  memcpy(buf, h, sizeof(ADFW_HEADER));

  /* FIRST WRITE THE HEADER, THEN THE OBSCOUNTS AND OFFSETS */
  fwrite(buf, 1, ADFW_HEADER_SIZE, ofp);
  fwrite(h->channeloffs, sizeof(int), h->nchannels, ofp);
  fwrite(h->obscounts, sizeof(int), h->nobs, ofp);
  fwrite(h->offsets, sizeof(int), h->nobs, ofp);
  
  /* AND FINALLY THE DIRECTORY, THE BLOCKS AND OFFSETS */
  memset(buf, 0, ADFW_DIR_SIZE);
  memcpy(buf, d, sizeof(ADFW_DIR));
  fwrite(buf, 1, ADFW_DIR_SIZE, ofp);
  fwrite(d->startoffs,sizeof(int),h->nobs,ofp);
  fwrite(d->stopoffs,sizeof(int),h->nobs,ofp);
  
  return 1;
}

ADFW_DIR *adfw_readDir(FILE *fp, int offset)
{
  ADFW_DIR *dir = (ADFW_DIR *) calloc(1, ADFW_DIR_SIZE);
  char *d = (char *) dir;
  size_t status;

  fseek(fp, offset, SEEK_SET);
  status = fread(d, ADFW_DIR_SIZE, 1, fp);

  if (status == 1) {

    dir->startoffs = (int *) calloc(dir->nobs, sizeof(int));
    status = fread(dir->startoffs, sizeof(int), dir->nobs, fp);
    if (!status) goto error;
    
    dir->stopoffs = (int *) calloc(dir->nobs, sizeof(int));
    status = fread(dir->stopoffs, sizeof(int), dir->nobs, fp);
    if (!status) goto error;
    
    return dir;
  } else {
    return NULL;
  }
 error:
  if (dir && dir->startoffs) free(dir->startoffs);
  if (dir && dir->stopoffs) free(dir->stopoffs);
  if (dir) free(dir);
  return NULL;
}

int adfw_getObsPeriodFromRawFile(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, 
			       int channel, int obsp,int *nsamps, short **vals)
{
  int bsize,offset = 0,k,n;
  short *s, *readbuf;
  int k0,kr,nr,ir,nChan;

  if (channel >= h->nchannels) return 0;
  if (obsp >= h->nobs) return 0;

  nChan = h->nchannels;
  n = h->obscounts[obsp];
	bsize = MAX_READ_BUFFER_SIZE*nChan;
	readbuf = (short *) calloc(MAX_READ_BUFFER_SIZE*nChan, sizeof(short));
  *vals = (short *) calloc(n, sizeof(short));
  s = *vals;

  offset = ADFW_HEADER_SIZE + (d->startoffs[obsp] * sizeof(short));
  fseek(fp, offset, SEEK_SET);

  k0 = h->obscounts[obsp]%MAX_READ_BUFFER_SIZE;
  nr = h->obscounts[obsp]/MAX_READ_BUFFER_SIZE;
  fread(readbuf,sizeof(short),k0*nChan,fp);
  for (k=0, kr=channel; k<k0; k++, kr+=nChan)	s[k] = readbuf[kr];
  for (ir=0; ir<nr; ir++) {
    fread(readbuf,sizeof(short),bsize,fp);
    for (kr=channel; k<MAX_READ_BUFFER_SIZE; k++, kr+=nChan)
      s[k] = readbuf[kr];
  }
  free(readbuf);

  *nsamps = n;
  return 1;
}

int adfw_getPartialObsPeriodFromRawFile(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, 
				      int channel, int obsp, int start_samp, 
				      int samp_dur,int *nsamps, short **vals)
{
//  int bsize,offset = 0,i,k,n;
//  short *s, *tmps;
  int offset = 0,n;
  short *tmps;
  int status;

  status = adfw_getObsPeriodFromRawFile(h,d,fp,channel,obsp,&n,&tmps);
  if (status != 1)  return status;

  if (samp_dur == -1) samp_dur = n;
  else if (samp_dur > n) {
    printf("adfw_readFileAndInfoByTime: Samp dur too large, truncating");
    samp_dur = n;
  }
  *vals = (short *) calloc(n, sizeof(short));
  memcpy(*vals,&tmps[start_samp],samp_dur*sizeof(short));
  *nsamps = samp_dur;
  free(tmps);
  return 1;
}



#ifdef STAND_ALONE
int main(int argc, char **argv)
{
  ADFW_HEADER *h;
  ADFW_DIR *d;
  FILE *fp;

  if (argc < 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    exit(0);
  }

  fp = fopen(argv[1], "rb");
  if (!fp) {
    fprintf(stderr, "%s: unable to open file %s\n", argv[0], argv[1]);
    exit(-1);
  }

  h = adfw_readHeader(fp);
  if (!h) {
    fprintf(stderr, "%s: unable to read adfw file %s\n", argv[0], argv[1]);
    return 1;
  }

  if (h->nobs == 0) {		/* Needs to be converted */
    FILE *ofp;
    if (argc < 3) ofp = stdout;
    else ofp = fopen(argv[2], "wb");
    if (!ofp) {
      fprintf(stderr, "%s: error opening output file\n", argv[0]);
      adfw_freeHeader(h);
      fclose(fp);
    }

    d = adfw_createDirectory(fp, h);
    adfw_convertFile(h, d, fp, ofp);
    if (ofp != stdout) fclose(ofp);
  }

  else {
    adfw_printInfo(h, fp);
  }

  adfw_freeHeader(h);
  fclose(fp);
  return 0;
}
#endif
