/*  adfxapi.c
 *  
 *  ver 1.00  12-Nov-2012 Yusuke MURAYAMA, MPI : derived from adfwapi.c
 *  ver 1.10  01-Mar-2013 Yusuke MURAYAMA, MPI : supports functions for Di.
 *  ver 1.12  26-Mar-2013 Yusuke MURAYAMA, MPI : cleanup/bug fix.
 *  ver 1.13  22-Jul-2013 Yusuke MURAYAMA, MPI : use int32_t/int64_t from "stdint.h".
 *  ver 1.14  24-Jul-2013 Yusuke MURAYAMA, MPI : support over 4GB files.
 *  ver 1.15  27-Aug-2013 Yusuke MURAYAMA, MPI : adc2volts as double, supports int32-Ai.
 *
 */

#ifdef _WIN32
//#elif defiend (__OSX__)
#elif defined __OSX__
#define off64_t   off_t
#define fopen64   fopen
#define fseeko64  fseek
#else
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS  64
#endif

#include <stdlib.h>
#include <stdio.h>
#include <memory.h> 
#include "adfxapi.h"

/*
 * Header support for streamer adfx files
 */

#define MAX_READ_BUFFER_SIZE (100000)  // will be multiplied by nchannels_bytes

static float adfx_version = (float)ADFX_VERSION;

// The first magic number is for the original - raw data
static char magic_adfx_raw[4] = { 12, 18, 21, 95 };

// This magic number says the file has be reorganized by obs/channel
static char magic_adfx_cnv[4] = {  9, 10, 21, 69 };


/////////////////////////////////////////////////////////////////////////
// local functions
#ifdef _WIN32
FILE *adfx_fopen(const char *fname, const char *mode)
{
    FILE *fp;
    if (fopen_s(&fp, fname, mode) != 0)  fp = NULL;
    return fp;
}

int adfx_fseek(FILE *fp, int64_t offset, int origin)
{
    return _fseeki64(fp, offset, origin);
}

int adfx_fclose(FILE *fp)
{
    return fclose(fp);
}
#else
FILE *adfx_fopen(const char *fname, const char *mode)
{
    return fopen64(fname,mode);
}

int adfx_fseek(FILE *fp, int64_t offset, int origin)
{
    //return fseek(fp, offset, origin);
    return fseeko64(fp, (off64_t)offset, origin);
}

int adfx_fclose(FILE *fp)
{
    return fclose(fp);
}
#endif



/////////////////////////////////////////////////////////////////////////
// ADFX APIs
void adfx_freeHeader(ADFX_HEADER *h)
{
    if (h == NULL) return;

    if (h->devices     != NULL)   free(h->devices);
    if (h->data_type   != NULL)   free(h->data_type);
    if (h->channels    != NULL)   free(h->channels);
    if (h->adc2volts   != NULL)   free(h->adc2volts);

    if (h->ai_channels != NULL)   free(h->ai_channels);
    if (h->di_channels != NULL)   free(h->di_channels);

    if (h->obscounts   != NULL)   free(h->obscounts);
    if (h->obsoffsets  != NULL)   free(h->obsoffsets);

    free(h);  h = NULL;
}

void adfx_initHeader(ADFX_HEADER *h, int converted)
{
    if (h == NULL)  return;
    memset(h, 0, sizeof(ADFX_HEADER));
    if (converted) {
        h->magic[0] = magic_adfx_cnv[0];
        h->magic[1] = magic_adfx_cnv[1];
        h->magic[2] = magic_adfx_cnv[2];
        h->magic[3] = magic_adfx_cnv[3];
    } else {
        h->magic[0] = magic_adfx_raw[0];
        h->magic[1] = magic_adfx_raw[1];
        h->magic[2] = magic_adfx_raw[2];
        h->magic[3] = magic_adfx_raw[3];
    }
    h->version = adfx_version;

    h->devices     = NULL;
    h->data_type   = NULL;
    h->channels    = NULL;
    h->adc2volts   = NULL;
    h->ai_channels = NULL;
    h->di_channels = NULL;

    h->obscounts   = NULL;
    h->obsoffsets  = NULL;

    return;
}

int adfx_channeloffs(int ch, ADFX_HEADER *h)
{
    int i;
    int chanoffs_bytes;

    if (h == NULL)  return 0;

    if (ch < 0)  ch = h->nchannels_ai + h->nchannels_di;

    chanoffs_bytes = 0;
    for (i = 0; i < ch; i++) {
        switch (h->data_type[i]) {
        case 'c':
            chanoffs_bytes = chanoffs_bytes + 1;    break;
        case 's':
            chanoffs_bytes = chanoffs_bytes + 2;    break;
        case 'i':
            chanoffs_bytes = chanoffs_bytes + 4;    break;
        case 'l':
            chanoffs_bytes = chanoffs_bytes + 8;    break;
        }
    }

    return chanoffs_bytes;
}

ADFX_HEADER *adfx_readHeader(FILE *fp)
{
    ADFX_HEADER *header;
    int i, ai, di, n, ftype;
	size_t status;
    float tmpf;

    header = (ADFX_HEADER *) calloc(1, ADFX_HEADER_STATIC_SIZE);
    if (header == NULL)  return NULL;
    
    adfx_fseek(fp, 0LL, SEEK_SET);

    if ( (status = fread((char *)header, ADFX_HEADER_STATIC_SIZE, 1, fp)) != 1) {
        free(header);
        return NULL;
    }
    header->devices     = NULL;
    header->data_type   = NULL;
    header->channels    = NULL;
    header->adc2volts   = NULL;
    header->ai_channels = NULL;
    header->di_channels = NULL;

    header->obscounts   = NULL;
    header->obsoffsets  = NULL;

    n = header->nchannels_ai + header->nchannels_di;

    ftype = adfx_checkMagicNumber(header->magic);

    if (ftype != ADF_ADFX2013_UNCONV && ftype != ADF_ADFX2013_CONV) {
        goto error;
    }

    adfx_fseek(fp, (int64_t)ADFX_HEADER_STATIC_SIZE, SEEK_SET);

    header->devices = (char *)calloc(n, sizeof(char));
    status = fread(header->devices, sizeof(char), n, fp);
    if (!status) goto error;

    header->data_type = (char *)calloc(n, sizeof(char));
    status = fread(header->data_type, sizeof(char), n, fp);
    if (!status) goto error;

    header->channels = (int32_t *)calloc(n, sizeof(int32_t));
    status = fread(header->channels, sizeof(int32_t), n, fp);
    if (!status) goto error;

    header->adc2volts = (double *)calloc(n, sizeof(double));
    if (header->version < 1.10f) {
        for (i = 0; i < n; i++) {
            status = fread(&tmpf, sizeof(float), 1, fp);
            if (!status) goto error;
            header->adc2volts[i] = (double)tmpf;
        }
    } else {
        status = fread(header->adc2volts, sizeof(double), n, fp);
	    if (!status) goto error;
	}

    header->ai_channels = (int32_t *)calloc(n, sizeof(int32_t));
    header->di_channels = (int32_t *)calloc(n, sizeof(int32_t));
    ai = 0;  di = 0;
    for (i = 0; i < n; i++) {
        if (header->channels[i] > 0)       header->ai_channels[ai++] = i;
        else if (header->channels[i] < 0)  header->di_channels[di++] = i;
    }

    switch (ftype) {
    case ADF_ADFX2013_UNCONV:
        header->nobs = 0;
        break;
    case ADF_ADFX2013_CONV:
        /* Get directory information */
        adfx_fseek(fp, (int64_t)header->offset2dir, SEEK_SET);
    
        header->obscounts  = (int32_t *) calloc(header->nobs, sizeof(int32_t));
        status = fread(header->obscounts, sizeof(int32_t),  header->nobs, fp);
        if (!status) goto error;
    
        header->obsoffsets = (int64_t *) calloc(header->nobs, sizeof(int64_t));
        status = fread(header->obsoffsets, sizeof(int64_t), header->nobs, fp);
        if (!status) goto error;

        break;
    default:
        goto error;
    }

    return header;

error:
    adfx_freeHeader(header);
    return NULL;
}

ADFX_DIR *adfx_newDirectory(int nchan, int nobs)
{
    ADFX_DIR *d = (ADFX_DIR *) calloc(1, sizeof(ADFX_DIR));
    if (!d) return NULL;
    if (!(d->chanoffs  = (int64_t *) calloc(nchan, sizeof(int64_t))))  goto error;
    if (!(d->startoffs = (int64_t *) calloc(nobs,  sizeof(int64_t))))  goto error;
    if (!(d->stopoffs  = (int64_t *) calloc(nobs,  sizeof(int64_t))))  goto error;
    return d;

error:
    if (d->chanoffs)  free(d->chanoffs);
    if (d->startoffs) free(d->startoffs);
    if (d->stopoffs)  free(d->stopoffs);
    free(d);
    return NULL;
}

void adfx_freeDirectory(ADFX_DIR *d)
{
    if (d) {
        if (d->chanoffs)  free(d->chanoffs);
        if (d->startoffs) free(d->startoffs);
        if (d->stopoffs)  free(d->stopoffs);
        free(d);
    }
}

ADFX_DIR *adfx_createDirectory(FILE *fp, ADFX_HEADER *h)
{
    return adfx_createDirectoryEx(fp,h,(double)DAQ_ANALOG_HIGH_VOLTS,(double)DAQ_ANALOG_LOW_VOLTS);
}

ADFX_DIR *adfx_createDirectoryEx(FILE *fp, ADFX_HEADER *h, double logicHvolts, double logicLvolts)
{
    ADFX_DIR *d;
    int i, n, nread, nchannels, obscnt;
    int64_t j;
    int chanoffs_bytes, bytes_per_sample;
    int allocsize = 1024;
    int obs_alloced = allocsize;
    int64_t *startoffs;
    int64_t *stopoffs;

    char  *buffer;
    char  cval;
    short sval;
    int   ival;
    int64_t lval;
    int   nbytes;
    double ms_per_sample;
    short  prevLevel, logicH, logicL;

    nchannels = h->nchannels_ai + h->nchannels_di;
    bytes_per_sample = adfx_channeloffs(-1, h);
    chanoffs_bytes   = adfx_channeloffs(h->obsp_chan,h);
  
    nbytes = bytes_per_sample*MAX_READ_BUFFER_SIZE;
    ms_per_sample = h->us_per_sample/1000.;
    prevLevel = 0;

    startoffs = (int64_t *) calloc(obs_alloced, sizeof(int64_t));
    stopoffs  = (int64_t *) calloc(obs_alloced, sizeof(int64_t));

    buffer = (char *) calloc(nbytes, sizeof(char));
    if (!buffer) {
        fprintf(stderr, "adfxapi: out of memory\n");
        return 0;
    }

    n = ADFX_HEADER_STATIC_SIZE + (1 + 1 + 4 + 8)*nchannels;  // char[],char[],int32[],double[]
    adfx_fseek(fp, (int64_t)n, SEEK_SET);
    // adfx_fseek(fp, (int64_t)h->offset2data, SEEK_SET);

    obscnt = 0;
    if (h->channels[h->obsp_chan] > 0) {
        // Obsp-TTL as Analog-Input
        logicH = (short)(logicHvolts/h->adc2volts[h->obsp_chan] + 0.5);
        logicL = (short)(logicLvolts/h->adc2volts[h->obsp_chan] + 0.5);

        h->obsp_logic_high = logicH;
        h->obsp_logic_low  = logicL;

        prevLevel = logicL-1;
        j = 0LL;
        while ((nread = (int)fread(buffer, sizeof(char), nbytes, fp)) > 0) {
#ifdef _DEBUG
            printf("\n n=%d/%d",nread,nbytes);
#endif
            /* We only need to check the trigger channel for the on/off markers */
            for (i = 0; i < nread; i+=bytes_per_sample) {
                sval = *((short *)&buffer[i + chanoffs_bytes]);
                if (sval > logicH && prevLevel < logicL) {
                    // goes Low to High
                    startoffs[obscnt] = (int64_t)i + j;  // index for the first channel
                    prevLevel = logicH+1;
#ifdef _DEBUG
                    printf("\n %8lld % 6d",startoffs[obscnt],sval);
#endif
                } else if (sval < logicL && prevLevel > logicH) {
                    // goes High to Low
                    stopoffs[obscnt] = (int64_t)i + j;  // index for the first channel
#ifdef _DEBUG
                    printf("\n %8lld % 6d: %lld",stopoffs[obscnt],sval,(stopoffs[obscnt]-startoffs[obscnt])/(int64_t)bytes_per_sample);
#endif
                    obscnt++;
                    prevLevel = logicL-1;
                    /* Make sure there's still enough space */
                    if (obscnt == obs_alloced) {
                        obs_alloced += allocsize;
                        startoffs = (int64_t *) realloc(startoffs, sizeof(int64_t)*obs_alloced);
                        stopoffs  = (int64_t *) realloc(stopoffs,  sizeof(int64_t)*obs_alloced);
                    }
                }
                //if (obscnt == 4) printf("\n %8d %d", i,buffer[i]);
            }
            j += (int64_t)nread;
        }

    } else {
        // Obsp-TTL as Digital-Input
        h->obsp_logic_high = 1;
        h->obsp_logic_low  = 0;

        prevLevel = 0;
        j = 0LL;
        while ((nread = (int)fread(buffer, sizeof(char), nbytes, fp)) > 0) {
#ifdef _DEBUG
            printf("\n nead=%d/%d",nread,nbytes);
#endif
#if 0
            for (i = 0; i < nread; i+=bytes_per_sample*100) {
                sval = *((short *)&buffer[i +  4]);
                cval = *((char  *)&buffer[i + 64]);
                printf("\n%+6d %+4d",sval,cval);
            }
#endif

            /* We only need to check the trigger channel for the on/off markers */
            for (i = 0; i < nread; i+=bytes_per_sample) {
                switch (h->data_type[h->obsp_chan]) {
                case 'c':
                    cval = *((char  *)&buffer[i + chanoffs_bytes]);
                    cval = cval & 0x01;
                    break;
                case 's':
                    sval = *((short *)&buffer[i + chanoffs_bytes]);
                    sval = sval & 0x01;
                    cval = (char)sval;
                    break;
                case 'i':
                    ival = *((int32_t *)&buffer[i + chanoffs_bytes]);
                    ival = ival & 0x01;
                    cval = (char)ival;
                    break;
                 case 'l':
                    lval = *((int64_t *)&buffer[i + chanoffs_bytes]);
                    lval = lval & 0x01;
                    cval = (char)lval;
                    break;
               }

                if (cval > 0 && prevLevel == 0) {
                    // goes Low to High
                    startoffs[obscnt] = (int64_t)i + j;  // index for the first channel
                    prevLevel = 1;
#ifdef _DEBUG
                    printf("\n %10lld ON  v=%d",startoffs[obscnt],cval);
#endif
                } else if (cval == 0 && prevLevel > 0) {
                    // goes High to Low
                    stopoffs[obscnt] = (int64_t)i + j;  // index for the first channel
#ifdef _DEBUG
                    printf("\n %10lld OFF v=%d: %lld",stopoffs[obscnt],cval,(stopoffs[obscnt]-startoffs[obscnt])/(int64_t)bytes_per_sample);
#endif
                    obscnt++;
                    prevLevel = 0;
                    /* Make sure there's still enough space */
                    if (obscnt == obs_alloced) {
                        obs_alloced += allocsize;
                        startoffs = (int64_t *) realloc(startoffs, sizeof(int64_t)*obs_alloced);
                        stopoffs  = (int64_t *) realloc(stopoffs,  sizeof(int64_t)*obs_alloced);
                    }
                }
                //if (obscnt == 4) printf("\n %8d %d", i,buffer[i]);
            }
            j += (int64_t)nread;
        }
    }

    d = adfx_newDirectory(nchannels, obscnt);
    h->nobs = d->nobs = obscnt;

    for (i = 0; i < nchannels; i++) {
        d->chanoffs[i] = adfx_channeloffs(i,h);
    }
    for (i = 0; i < obscnt; i++) {
        d->startoffs[i] = startoffs[i];
        d->stopoffs[i]  = stopoffs[i];
    }


    free(startoffs);
    free(stopoffs);

    return d;
}

int adfx_convertFile(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, FILE *ofp)
{
    char buf[ADFX_HEADER_STATIC_SIZE];
    char *readbuf, *samples;
    int ich, iobs, j, k, k0, kr, nr, ir;
    int nChanRaw, obspChan, nChanCnv;
    int nread_bytes, bytes_per_sample_raw, bytes_per_sample_cnv;
    int64_t n64, offset64, maxobs64, offset2data_raw;

    int32_t *chanoffs_raw;
    char    *cbuff, *datatype_raw;
    short   *sbuff;
    int32_t *ibuff;
    int64_t *lbuff;

    // keep the values before modifying the header....
    nChanRaw = h->nchannels_ai + h->nchannels_di;
    bytes_per_sample_raw = adfx_channeloffs(-1,h);
    offset2data_raw = h->offset2data;
    chanoffs_raw = (int32_t *) calloc(nChanRaw, sizeof(int32_t));
    datatype_raw = (char    *) calloc(nChanRaw, sizeof(char));
    for (ich = 0; ich < nChanRaw; ich++) {
        datatype_raw[ich] = h->data_type[ich];
        chanoffs_raw[ich] = adfx_channeloffs(ich,h);
    }

    // ok, modify the header
    obspChan = -1;
    if (h->obsp_chan >= 0) {
        if (h->channels[h->obsp_chan] > 0) {
            // we don't need the last Ai channel.
            obspChan = h->obsp_chan;
            for (ich = h->obsp_chan; ich < nChanRaw-1; ich++) {
                h->devices[ich]   = h->devices[ich+1];
                h->data_type[ich] = h->data_type[ich+1];
                h->channels[ich]  = h->channels[ich+1];
                h->adc2volts[ich] = h->adc2volts[ich+1];
            }
            h->nchannels_ai = h->nchannels_ai - 1;
            h->obsp_chan    = -1;
        }
    }
    nChanCnv = h->nchannels_ai + h->nchannels_di;
    bytes_per_sample_cnv = adfx_channeloffs(-1,h);


    h->obscounts  = (int32_t *) calloc(h->nobs, sizeof(int32_t));
    h->obsoffsets = (int64_t *) calloc(h->nobs, sizeof(int64_t));
  
    // Create the new header counts/offsets
    offset64 = 0;  maxobs64 = 0;
    for (iobs = 0; iobs < h->nobs; iobs++) {
        n64 = (d->stopoffs[iobs] - d->startoffs[iobs])/(int64_t)bytes_per_sample_raw;
        h->obscounts[iobs]  = (int32_t)n64;
        h->obsoffsets[iobs] = offset64;
        if ((int64_t)h->obscounts[iobs] > maxobs64) maxobs64 = (int64_t)h->obscounts[iobs];  /* keep track of longest obsp */
        offset64 += n64*(int64_t)bytes_per_sample_cnv;
    }

    //  Make room for largest obs period
    nread_bytes = MAX_READ_BUFFER_SIZE*bytes_per_sample_raw;
    readbuf = (char *) calloc(MAX_READ_BUFFER_SIZE*bytes_per_sample_raw, sizeof(char));
    samples = (char *) calloc(MAX_READ_BUFFER_SIZE*bytes_per_sample_cnv, sizeof(char));
    if (!readbuf || !samples) {
        if (readbuf) free(readbuf);
        free(h->obscounts);
        free(h->obsoffsets);
        return 0;
    }
    cbuff = (char  *)samples;
    sbuff = (short *)samples;
    ibuff = (int32_t *)samples;
    lbuff = (int64_t *)samples;


    // And now write out the header + counts/offsets
    h->magic[0] = magic_adfx_cnv[0];
    h->magic[1] = magic_adfx_cnv[1];
    h->magic[2] = magic_adfx_cnv[2];
    h->magic[3] = magic_adfx_cnv[3];
    h->version  = adfx_version;
    
    h->offset2dir  = ADFX_HEADER_STATIC_SIZE + (1 + 1 + 4 + 8)*nChanCnv;
    h->offset2data = ADFX_HEADER_STATIC_SIZE + (1 + 1 + 4 + 8)*nChanCnv + (4 + 8)*h->nobs;
    memset(buf, 0, ADFX_HEADER_STATIC_SIZE);
    memcpy(buf, h, sizeof(ADFX_HEADER));
    ((ADFX_HEADER *)buf)->devices     = NULL;
    ((ADFX_HEADER *)buf)->data_type   = NULL;
    ((ADFX_HEADER *)buf)->channels    = NULL;
    ((ADFX_HEADER *)buf)->obscounts   = NULL;
    ((ADFX_HEADER *)buf)->ai_channels = NULL;
    ((ADFX_HEADER *)buf)->di_channels = NULL;

    fwrite(buf, 1, ADFX_HEADER_STATIC_SIZE, ofp);
    fwrite(h->devices,    sizeof(char),    nChanCnv, ofp);
    fwrite(h->data_type,  sizeof(char),    nChanCnv, ofp);
    fwrite(h->channels,   sizeof(int32_t), nChanCnv, ofp);
    fwrite(h->adc2volts,  sizeof(double),  nChanCnv, ofp);

    fwrite(h->obscounts,  sizeof(int32_t), h->nobs,  ofp);
    fwrite(h->obsoffsets, sizeof(int64_t), h->nobs,  ofp);
  
    for (j = 0; j < h->nobs; j++) {
        for (ich = 0; ich < nChanRaw; ich++) {
            if (ich == obspChan)  continue;  // skip Ai-Obsp
            offset64 = offset2data_raw + d->startoffs[j];
            adfx_fseek(fp, offset64, SEEK_SET);

            k0 = (int)(h->obscounts[j]%MAX_READ_BUFFER_SIZE);
            nr = (int)(h->obscounts[j]/MAX_READ_BUFFER_SIZE);
            fread(readbuf,sizeof(char),k0*bytes_per_sample_raw,fp);
            switch (datatype_raw[ich]) {
            case 'c':
                kr = chanoffs_raw[ich];
                for (k = 0; k < k0; k++, kr+=bytes_per_sample_raw)  cbuff[k] = (char)readbuf[kr];
                fwrite(cbuff, sizeof(char), k0, ofp);
                for (ir=0; ir<nr; ir++) {
                    fread(readbuf,sizeof(char),nread_bytes,fp);
                    kr = chanoffs_raw[ich];
                    for (k=0; k<MAX_READ_BUFFER_SIZE; k++, kr+=bytes_per_sample_raw)  cbuff[k] = (char )readbuf[kr];
                    fwrite(cbuff, sizeof(char), MAX_READ_BUFFER_SIZE, ofp);
                }
                break;
            case 's':
                kr = chanoffs_raw[ich];
                for (k = 0; k < k0; k++, kr+=bytes_per_sample_raw)  sbuff[k] = *((short *)&readbuf[kr]);
                fwrite(sbuff, sizeof(short), k0, ofp);
                for (ir=0; ir<nr; ir++) {
                    fread(readbuf,sizeof(char),nread_bytes,fp);
                    kr = chanoffs_raw[ich];
                    for (k=0; k<MAX_READ_BUFFER_SIZE; k++, kr+=bytes_per_sample_raw)  sbuff[k] = *((short *)&readbuf[kr]);
                    fwrite(sbuff, sizeof(short), MAX_READ_BUFFER_SIZE, ofp);
                }
                break;
            case 'i':
                kr = chanoffs_raw[ich];
                for (k = 0; k < k0; k++, kr+=bytes_per_sample_raw)  ibuff[k] = *((int32_t *)&readbuf[kr]);
                fwrite(ibuff, sizeof(int32_t), k0, ofp);
                for (ir=0; ir<nr; ir++) {
                    fread(readbuf,sizeof(char),nread_bytes,fp);
                    kr = chanoffs_raw[ich];
                    for (k=0; k<MAX_READ_BUFFER_SIZE; k++, kr+=bytes_per_sample_raw)  ibuff[k] = *((int32_t *)&readbuf[kr]);
                    fwrite(ibuff, sizeof(int32_t), MAX_READ_BUFFER_SIZE, ofp);
                }
                break;
            case 'l':
                kr = chanoffs_raw[ich];
                for (k = 0; k < k0; k++, kr+=bytes_per_sample_raw)  lbuff[k] = *((int64_t *)&readbuf[kr]);
                fwrite(ibuff, sizeof(int64_t), k0, ofp);
                for (ir=0; ir<nr; ir++) {
                    fread(readbuf,sizeof(char),nread_bytes,fp);
                    kr = chanoffs_raw[ich];
                    for (k=0; k<MAX_READ_BUFFER_SIZE; k++, kr+=bytes_per_sample_raw)  lbuff[k] = *((int64_t *)&readbuf[kr]);
                    fwrite(ibuff, sizeof(int64_t), MAX_READ_BUFFER_SIZE, ofp);
                }
                break;
            }
        }
#ifdef _DEBUG
        printf(".");
#endif
    }

    free(samples);
    return 1;
}


int adfx_getObsPeriod(ADFX_HEADER *h, FILE *fp, int channel, int obsp,
                      int *nsamps, void **vals, char *datatype)
{
    void *v;
    int64_t chanoffs;

    if (h == NULL)  return 0;

    if (channel >= h->nchannels_ai) return -1;
    if (obsp >= h->nobs) return -2;

    chanoffs = adfx_channeloffs(h->ai_channels[channel],h);
    chanoffs = chanoffs*h->obscounts[obsp];

#ifdef _DEBUG
    printf("\n chanoffs[%d]=%lld, obscounts=%d, obsoffsets=%lld",
           channel, chanoffs,h->obscounts[obsp],h->obsoffsets[obsp]);
#endif

    adfx_fseek(fp, (int64_t)h->offset2data + h->obsoffsets[obsp] + chanoffs, SEEK_SET);

    *datatype = adfx_getAiDataType(h,channel);
    switch (*datatype) {
    case 's':
        v = calloc(h->obscounts[obsp], sizeof(short));
        fread(v, sizeof(short), h->obscounts[obsp], fp);
        break;
    case 'i':
        v = calloc(h->obscounts[obsp], sizeof(int32_t));
        fread(v, sizeof(int32_t), h->obscounts[obsp], fp);
        break;
    case 'l':
        v = calloc(h->obscounts[obsp], sizeof(int64_t));
        fread(v, sizeof(int64_t), h->obscounts[obsp], fp);
        break;
    }

    *nsamps = h->obscounts[obsp];
    *vals = v;
    return 1;
}

/* adfx_getObsPeriodPartial by DAL, 26-Oct-2001 */
int adfx_getObsPeriodPartial(ADFX_HEADER *h, FILE *fp, int channel, int obsp,
                             int startindx, int nread, int *nsamps, void **vals, char *datatype)
{
    void *v;
    int64_t chanoffs, nseek;

    if (h == NULL)  return 0;

    if (channel >= h->nchannels_ai) return -1;
    if (obsp >= h->nobs) return -2;
    if (nread < 0)  nread = h->obscounts[obsp]-startindx;
    if (startindx < 0 || startindx+nread > h->obscounts[obsp]) {
        fprintf(stderr, "Invalid indexes [%d (tot %d), with obs %d long]\n", startindx, nread, h->obscounts[obsp]);
        return -3;
    }

    chanoffs = (int64_t)adfx_channeloffs(h->ai_channels[channel],h);
    chanoffs = chanoffs*(int64_t)h->obscounts[obsp];

    nseek = (int64_t)h->offset2data + h->obsoffsets[obsp];

    *datatype = adfx_getAiDataType(h,channel);
    switch (*datatype) {
    case 's':
        nseek = nseek + chanoffs + (int64_t)(startindx*sizeof(short));
        adfx_fseek(fp, nseek, SEEK_SET);
        v = calloc(nread, sizeof(short));
        fread(v, sizeof(short), nread, fp);
        break;
    case 'i':
        nseek = nseek + chanoffs + (int64_t)(startindx*sizeof(int32_t));
        adfx_fseek(fp, nseek, SEEK_SET);
        v = calloc(nread, sizeof(int32_t));
        fread(v, sizeof(int32_t), nread, fp);
        break;
    case 'l':
        nseek = nseek + chanoffs + (int64_t)(startindx*sizeof(int64_t));
        adfx_fseek(fp, nseek, SEEK_SET);
        v = calloc(nread, sizeof(int64_t));
        fread(v, sizeof(int64_t), nread, fp);
        break;
    }
    
    *nsamps = nread;
    *vals = v;
    return 1;
}

void adfx_printInfo(ADFX_HEADER *h, FILE *fp)
{
    printf("Ai%d/Di%d Channels / %d Obs Periods / %lld\n", h->nchannels_ai, h->nchannels_di, h->nobs, h->obsoffsets[0]);
}

void adfx_printDirectory(ADFX_HEADER *h, ADFX_DIR *d)
{
    int i, bytes_per_sample;
    double ms_per_sample = h->us_per_sample/1000.;
    int64_t n;

    if (!d) return;
 
    bytes_per_sample = adfx_channeloffs(-1,h);

    for (i = 0; i < d->nobs; i++) {
        n = (d->stopoffs[i] - d->startoffs[i])/(int64_t)bytes_per_sample;
        printf("%d %.2lfms [%lld]\n", i, (double)n*ms_per_sample, n);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////
// Digital port
int  adfx_getDiObsPeriod(ADFX_HEADER *h, FILE *fp, int iport, int obsp,
                         int *nsamps, void **vals, char *datatype)
{
    void *v;
    int64_t chanoffs;

    if (h == NULL)  return 0;

    if (iport >= h->nchannels_di) return -1;
    if (obsp >= h->nobs) return -2;

    chanoffs = adfx_channeloffs(h->di_channels[iport],h);
    chanoffs = chanoffs*h->obscounts[obsp];

#ifdef _DEBUG
    printf("\n chanoffs[%d]=%d, obscounts=%d, obsoffsets=%lld",
           iport, chanoffs,h->obscounts[obsp],h->obsoffsets[obsp]);
#endif

    adfx_fseek(fp, (int64_t)h->offset2data + h->obsoffsets[obsp] + chanoffs, SEEK_SET);

    *datatype = adfx_getDiDataType(h,iport);
    switch (*datatype) {
    case 'c':
        v = calloc(h->obscounts[obsp], sizeof(unsigned char)); 
        fread(v, sizeof(unsigned char), h->obscounts[obsp], fp);
        break;
    case 's':
        v = calloc(h->obscounts[obsp], sizeof(unsigned short));
        fread(v, sizeof(unsigned short), h->obscounts[obsp], fp);
        break;
    case 'i':
        v = calloc(h->obscounts[obsp], sizeof(uint32_t));
        fread(v, sizeof(uint32_t), h->obscounts[obsp], fp);
        break;
    case 'l':
        v = calloc(h->obscounts[obsp], sizeof(uint64_t));
        fread(v, sizeof(uint64_t), h->obscounts[obsp], fp);
        break;
    }
    
    *nsamps = h->obscounts[obsp];
    *vals = v;
    return 1;
}

int  adfx_getDiObsPeriodPartial(ADFX_HEADER *h, FILE *fp, int iport, int obsp,
                                int startindx, int nread, int *nsamps, void **vals, char *datatype)
{
    void *v;
    int64_t chanoffs, nseek;

    if (h == NULL)  return 0;

    if (iport >= h->nchannels_di) return -1;
    if (obsp >= h->nobs) return -2;
    if (nread < 0)  nread = h->obscounts[obsp]-startindx;
    if (startindx < 0 || startindx+nread > h->obscounts[obsp]) {
        fprintf(stderr, "Invalid indexes [%d (tot %d), with obs %d long]\n", startindx, nread, h->obscounts[obsp]);
        return -3;
    }

    chanoffs = (int64_t)adfx_channeloffs(h->di_channels[iport],h);
    chanoffs = chanoffs*(int64_t)h->obscounts[obsp];

    nseek = (int64_t)h->offset2data + h->obsoffsets[obsp];
    nseek = nseek + chanoffs + (int64_t)(startindx*sizeof(short));

    adfx_fseek(fp, nseek, SEEK_SET);

    *datatype = adfx_getDiDataType(h,iport);
    switch (*datatype) {
    case 'c':
        v = calloc(nread, sizeof(char)); 
        fread(v, sizeof(char), nread, fp);
        break;
    case 's':
        v = calloc(nread, sizeof(short));
        fread(v, sizeof(short), nread, fp);
        break;
    case 'i':
        v = calloc(nread, sizeof(int32_t));
        fread(v, sizeof(int32_t), nread, fp);
        break;
    case 'l':
        v = calloc(nread, sizeof(int64_t));
        fread(v, sizeof(int64_t), nread, fp);
        break;
    }

    *nsamps = nread;
    *vals = v;
    return 1;
}



///////////////////////////////////////////////////////////////////////////////////////////
// file checker
int adfx_getFileFormat(FILE *fp)
{
    char m[4];
    size_t status;

    if (fp == NULL)  return -1;
    memset(m, 0, 4);

    adfx_fseek(fp, 0LL, SEEK_SET);
    status = fread(m, 4, sizeof(char), fp);
    adfx_fseek(fp, 0LL, SEEK_SET);
    if (status > 0) {
        return adfx_checkMagicNumber(m);
    } else {
        return -1;
    }
    return -1;
}

int adfx_checkFileFormat(char *fname)
{
    FILE *fp = NULL;
    size_t status;
    char m[4];

    memset(m, 0, 4);
    fp = adfx_fopen(fname, "rb");
    if (!fp) {
        fprintf(stderr, "adfx_checkFileFormat(): unable to open file %s\n", fname);
        return -1;
    }

    status = fread(m, 4, sizeof(char), fp);
    adfx_fclose(fp);
    if (status > 0) {
        return adfx_checkMagicNumber(m);
    }
    return ADF_UNKNOWN;
}

int adfx_checkMagicNumber(char *m)
{
    /* Check for magic number: 12 18 21 95 */
    if (m[0] == 12 && m[1] == 18 && m[2] == 21 && m[3] == 95)
        return ADF_ADFX2013_UNCONV;
    /* Check for magic number:  9 10 21 69 */
    if (m[0] ==  9 && m[1] == 10 && m[2] == 21 && m[3] == 69)
        return ADF_ADFX2013_CONV;

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


///////////////////////////////////////////////////////////////////////////////////////////
// properties
int  adfx_getAiNumChannels(ADFX_HEADER *h)  {  return h->nchannels_ai;  }
int  adfx_getDiNumPorts(ADFX_HEADER *h)     {  return h->nchannels_di;  }
char adfx_getAiDataType(ADFX_HEADER *h, int chan)   {  return h->data_type[h->ai_channels[chan]];  }
char adfx_getDiDataType(ADFX_HEADER *h, int iport)  {  return h->data_type[h->di_channels[iport]]; }
int  adfx_getDiPortWidth(ADFX_HEADER *h, int iport)
{
    int ich, w;
    
    if (iport < 0 || iport >= h->nchannels_di)  return 0;
    ich = h->di_channels[iport];
    w = 0;
    switch (h->data_type[ich]) {
    case 'c':
        w =  8;  break;
    case 's':
        w = 16;  break;
    case 'i':
        w = 32;  break;
    case 'l':
        w = 64;  break;
    }
    return w;
}



///////////////////////////////////////////////////////////////////////////////////////////
// extended functions
int adfx_mkConvInfoFile(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, FILE *ofp)
{
    char buf[ADFX_HEADER_STATIC_SIZE];
    int i, nch;
    int bytes_per_sample;
    int64_t n, offset;

    h->obscounts  = (int32_t *) calloc(h->nobs, sizeof(int32_t));
    h->obsoffsets = (int64_t *) calloc(h->nobs, sizeof(int64_t));
  
    bytes_per_sample = adfx_channeloffs(-1,h);

    /* Create the new header counts/offsets */
    offset = 0;
    for (i = 0; i < h->nobs; i++) {
        n = (d->stopoffs[i] - d->startoffs[i])/(int64_t)bytes_per_sample;
        h->obscounts[i]  = (int)n;
        h->obsoffsets[i] = offset;
        offset += n*bytes_per_sample;
    }

    nch = h->nchannels_ai + h->nchannels_di;
    h->offset2dir  = ADFX_HEADER_STATIC_SIZE + (1 + 1 + 4 + 8)*nch;
    //h->offset2data = h->offset2dir;  should not be updated to read the raw file. 

    /* NOTE THAT THIS IS ALL CHANGED FROM THE ORIGINAL VERSION */
    /* And now write out the header + directory */
    h->magic[0] = magic_adfx_cnv[0];
    h->magic[1] = magic_adfx_cnv[1];
    h->magic[2] = magic_adfx_cnv[2];
    h->magic[3] = magic_adfx_cnv[3];
    h->version  = adfx_version;
    
    memset(buf, 0, ADFX_HEADER_STATIC_SIZE);
    memcpy(buf, h, sizeof(ADFX_HEADER));
    ((ADFX_HEADER *)buf)->devices     = NULL;
    ((ADFX_HEADER *)buf)->data_type   = NULL;
    ((ADFX_HEADER *)buf)->channels    = NULL;
    ((ADFX_HEADER *)buf)->obscounts   = NULL;
    ((ADFX_HEADER *)buf)->ai_channels = NULL;
    ((ADFX_HEADER *)buf)->di_channels = NULL;

    /* FIRST WRITE THE HEADER, THEN THE OBSCOUNTS AND OFFSETS */
    fwrite(buf, 1, ADFX_HEADER_STATIC_SIZE, ofp);
    fwrite(h->devices,    sizeof(char),    nch, ofp);
    fwrite(h->data_type,  sizeof(char),    nch, ofp);
    fwrite(h->channels,   sizeof(int32_t), nch, ofp);
    fwrite(h->adc2volts,  sizeof(double),  nch, ofp);

    fwrite(h->obscounts,  sizeof(int32_t), h->nobs,  ofp);
    fwrite(h->obsoffsets, sizeof(int64_t), h->nobs,  ofp);

  
    /* AND FINALLY THE DIRECTORY, THE BLOCKS AND OFFSETS */
    memset(buf, 0, ADFX_DIR_STATIC_SIZE);
    memcpy(buf, d, sizeof(ADFX_DIR));
    fwrite(buf, 1, ADFX_DIR_STATIC_SIZE, ofp);
    fwrite(d->startoffs, sizeof(int64_t),h->nobs,ofp);
    fwrite(d->stopoffs,  sizeof(int64_t),h->nobs,ofp);
 
    return 1;
}

ADFX_DIR *adfx_readDirEx(FILE *fp, ADFX_HEADER *h)
{
    ADFX_DIR *dir = (ADFX_DIR *) calloc(1, ADFX_DIR_STATIC_SIZE);
    char *d = (char *) dir;
    size_t status;

    adfx_fseek(fp, (int64_t)h->offset2data, SEEK_SET);
    status = fread(d, ADFX_DIR_STATIC_SIZE, 1, fp);

    if (status == 1) {

        dir->startoffs = (int64_t *) calloc(dir->nobs, sizeof(int64_t));
        status = fread(dir->startoffs, sizeof(int64_t), dir->nobs, fp);
        if (!status) goto error;
    
        dir->stopoffs  = (int64_t *) calloc(dir->nobs, sizeof(int64_t));
        status = fread(dir->stopoffs,  sizeof(int64_t), dir->nobs, fp);
        if (!status) goto error;
    
        return dir;
    } else {
        return NULL;
    }
error:
    if (dir && dir->startoffs) free(dir->startoffs);
    if (dir && dir->stopoffs)  free(dir->stopoffs);
    if (dir) free(dir);
    return NULL;
}

int adfx_getObsPeriodFromRawFile(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, 
                                 int channel, int obsp,int *nsamps, void **vals, char *datatype)
{
    int bsize,k,n;
    char *readbuf;
    short *sdata;
    int32_t *idata;
    int64_t *ldata;
    int k0,kr,nr,ir,nChan, bytes_per_sample, chanoffs;
    int64_t offset;


    if (channel >= h->nchannels_ai) return 0;
    if (obsp >= h->nobs) return 0;

    nChan = h->nchannels_ai;
    n = h->obscounts[obsp];
    bytes_per_sample = adfx_channeloffs(-1,h);
    chanoffs = adfx_channeloffs(h->ai_channels[channel],h);

    bsize = MAX_READ_BUFFER_SIZE*bytes_per_sample;
    readbuf = (char *)  calloc(MAX_READ_BUFFER_SIZE*bytes_per_sample, sizeof(char));

    offset = (int64_t)h->offset2data + d->startoffs[obsp];
    adfx_fseek(fp, offset, SEEK_SET);
    k0 = h->obscounts[obsp]%MAX_READ_BUFFER_SIZE;
    nr = h->obscounts[obsp]/MAX_READ_BUFFER_SIZE;

    *datatype = adfx_getAiDataType(h,channel);
    switch (*datatype) {
    case 's':
        *vals = calloc(n, sizeof(short));
        sdata = (short *)*vals;
        fread(readbuf,sizeof(char),k0*bytes_per_sample,fp);
        for (k=0, kr=chanoffs; k<k0; k++, kr+=bytes_per_sample)  sdata[k] = *((short *)&readbuf[kr]);
        for (ir=0; ir<nr; ir++) {
            fread(readbuf,sizeof(char),bsize,fp);
            for (kr=chanoffs; k<MAX_READ_BUFFER_SIZE; k++, kr+=bytes_per_sample)
                sdata[k] = *((short *)&readbuf[kr]);
        }
        break;
    case 'i':
        *vals = calloc(n, sizeof(int32_t));
        idata = (int32_t *)*vals;
        fread(readbuf,sizeof(char),k0*bytes_per_sample,fp);
        for (k=0, kr=chanoffs; k<k0; k++, kr+=bytes_per_sample)  idata[k] = *((int32_t *)&readbuf[kr]);
        for (ir=0; ir<nr; ir++) {
            fread(readbuf,sizeof(char),bsize,fp);
            for (kr=chanoffs; k<MAX_READ_BUFFER_SIZE; k++, kr+=bytes_per_sample)
                idata[k] = *((int32_t *)&readbuf[kr]);
        }
        break;
    case 'l':
        *vals = calloc(n, sizeof(int64_t));
        ldata = (int64_t *)*vals;
        fread(readbuf,sizeof(char),k0*bytes_per_sample,fp);
        for (k=0, kr=chanoffs; k<k0; k++, kr+=bytes_per_sample)  ldata[k] = *((int64_t *)&readbuf[kr]);
        for (ir=0; ir<nr; ir++) {
            fread(readbuf,sizeof(char),bsize,fp);
            for (kr=chanoffs; k<MAX_READ_BUFFER_SIZE; k++, kr+=bytes_per_sample)
                ldata[k] = *((int64_t *)&readbuf[kr]);
        }
        break;
    }

    free(readbuf);

    *nsamps = n;
    return 1;
}

int adfx_getPartialObsPeriodFromRawFile(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, 
                                        int channel, int obsp, int start_samp, 
                                        int samp_dur,int *nsamps, void **vals, char *datatype)
{
//  int bsize,offset = 0,i,k,n;
//  short *s, *tmps;
    int offset = 0,n;
    char *tmps;
    int status;

    status = adfx_getObsPeriodFromRawFile(h,d,fp,channel,obsp,&n,&tmps,datatype);
    if (status != 1)  return status;

    if (samp_dur == -1) samp_dur = n;
    else if (samp_dur > n) {
        printf("adfx_readFileAndInfoByTime: Samp dur too large, truncating");
        samp_dur = n;
    }
    switch (*datatype) {
    case 's':
        *vals = malloc(n*sizeof(short));
        memcpy(*vals,&tmps[start_samp*sizeof(short)],samp_dur*sizeof(short));
        break;
    case 'i':
        *vals = malloc(n*sizeof(int32_t));
        memcpy(*vals,&tmps[start_samp*sizeof(int32_t)],samp_dur*sizeof(int32_t));
        break;
    case 'l':
        *vals = malloc(n*sizeof(int64_t));
        memcpy(*vals,&tmps[start_samp*sizeof(int64_t)],samp_dur*sizeof(int64_t));
        break;
    }
    *nsamps = samp_dur;
    free(tmps);
    return 1;
}



#ifdef STAND_ALONE
int main(int argc, char **argv)
{
    ADFX_HEADER *h;
    ADFX_DIR *d;
    FILE *fp = NULL;

    if (argc < 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        exit(0);
    }

    fp = adfx_fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "%s: unable to open file %s\n", argv[0], argv[1]);
        exit(-1);
    }

    h = adfx_readHeader(fp);
    if (!h) {
        fprintf(stderr, "%s: unable to read adfx file %s\n", argv[0], argv[1]);
        return 1;
    }

    if (h->nobs == 0) {    /* Needs to be converted */
        FILE *ofp = NULL;
        if (argc < 3) ofp = stdout;
        else {
            ofp = adfx_fopen(argv[2], "wb");
        }
        if (!ofp) {
            fprintf(stderr, "%s: error opening output file\n", argv[0]);
            adfx_freeHeader(h);
            adfx_fclose(fp);
        }

        d = adfx_createDirectory(fp, h);
        adfx_convertFile(h, d, fp, ofp);
        if (ofp != stdout) adfx_fclose(ofp);
    } else {
        adfx_printInfo(h, fp);
    }

    adfx_freeHeader(h);
    adfx_fclose(fp);
    return 0;
}
#endif
