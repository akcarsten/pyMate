/*=================================================================
 *
 * adf_readFileAndInfo.c
 * Reads the raw file using the obsper block information from the 
 * accompanying 'info' file in the same directory.
 * DAL AUG-00
 * YM  02-Sep-2000  supports adfw format
 * YM  07-Oct-2002  moves APIs in adfapi2.c to adfapi.c
 * YM  06-Dec-2012  supports adfx format
 *
 *=================================================================*/
/* $Revision: 1.0 $ */

#include <math.h>
#include <stdio.h>
#include "adfapi.h"
#include "adfwapi.h"  // 02-Sep-2000 YM
#include "adfxapi.h"  // 06-Dec-2012 YM
#include "matrix.h"
#include "mex.h"

/* Input Arguments */

#define	FILE_IN	        prhs[0]
#define	INDEX_IN        prhs[1]
#define	CHAN_IN	        prhs[2]
#define	START_TIME      prhs[3]
#define	DURATION        prhs[4]
#define	DECIMATE_IN     prhs[5]

/* Output Arguments */

#define	ADF_OUT	        plhs[0]
#define	ADF_LENGTH      plhs[1]
#define ADF_RATE        plhs[2]

static int resolution = 12;

/* prototype */
void freeBuffers(ADF_HEADER *h, ADF_HEADER *ih, ADF_DIR *d,
                 ADFW_HEADER *hw, ADFW_HEADER *ihw, ADFW_DIR *dw,
                 ADFX_HEADER *hx, ADFX_HEADER *ihx, ADFX_DIR *dx);


void mexFunction( int nlhs, mxArray *plhs[], 
                  int nrhs, const mxArray*prhs[] )
{ 
    FILE *fp,*ifp;
    ADF_HEADER *h, *ih;         /* header and 'info' header */
    ADF_DIR *d;
    ADFW_HEADER *hw, *ihw;      /* 02-Sep-2000 YM           */
    ADFW_DIR *dw;
    ADFX_HEADER *hx, *ihx;
    ADFX_DIR *dx;
    int ftype, nchannels, inchannels, nobs;
    double us_per_sample;
    char *filename, *ifilename, buf[128];
    int obsindex, channel = 0;
    int status, buflen, i;
    short *vals;
    double *dp, *p;
    int n, decimate = 1;
    double startt = 0, dur = 0;
    int start_samp = -1, samp_dur = -1;
    double *lengthptr, *rateptr;
    double offset = (double) (1<<(resolution-1));
    int diroff;

    /* Check for proper number of arguments */
    if (nrhs < 5 || nlhs > 3)  
        mexErrMsgTxt("Usage: [vals length rate] = adf_readFileAndInfoByTime file obs chan startt stopt [decimate]"); 
  
    /* Get the filename */
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_readFileAndInfo: first arg must be filename string"); 
    }
    buflen = (mxGetM(FILE_IN) * mxGetN(FILE_IN)) + 1;
    filename = mxCalloc(buflen, sizeof(char));
    ifilename = mxCalloc(buflen+4, sizeof(char));
    status = mxGetString(FILE_IN, filename, buflen);
    strcpy(ifilename, filename);
    strcpy(&ifilename[buflen-1],"info");
  
    if (status != 0)
        mexWarnMsgTxt("adf_readFileAndInfoByTime: filename string is truncated.");
  
    /* Get the obs index */
    obsindex = (int) mxGetScalar(INDEX_IN);

    if (nrhs > 2) {
        channel = (int) mxGetScalar(CHAN_IN);
    }
    if (nrhs > 3) {
        startt = mxGetScalar(START_TIME);
    }
    if (nrhs > 4) {
        dur = mxGetScalar(DURATION);
    }
    if (nrhs > 5) {
        decimate = (int) mxGetScalar(DECIMATE_IN);
    }

    fp = fopen(filename, "rb");
    if (!fp) mexErrMsgTxt("adf_readFileAndInfo: data file not found"); 

    ifp = fopen(ifilename, "rb");
    if (!ifp) {
        fclose(fp);
        mexErrMsgTxt("adf_readFileAndInfo: info file not found"); 
    }
    ftype = adfw_getFileFormat(ifp);
    h  = NULL;  ih  = NULL;  d  = NULL;
    hw = NULL;  ihw = NULL;  dw = NULL;
    hx = NULL;  ihx = NULL;  dx = NULL;
    switch (ftype) {
    case ADF_ADFX2013_CONV :
        hx = adfx_readHeader(fp);    ihx = adfx_readHeader(ifp);
        dx = adfx_readDirEx(ifp, ihx);                     /* read the dir info    */
        nchannels = hx->nchannels_ai;  inchannels = ihx->nchannels_ai;
        nobs = ihx->nobs;  us_per_sample = ihx->us_per_sample;
        break;
    case ADF_PCI6052E_CONV :
        hw = adfw_readHeader(fp);    ihw = adfw_readHeader(ifp);
        /* skip to the dir info */
        diroff = ADFW_HEADER_SIZE+sizeof(long)*(hw->nchannels+2*ihw->nobs);
        dw = adfw_readDir(ifp, diroff);                     /* read the dir info    */
        nchannels = hw->nchannels;  inchannels = ihw->nchannels;
        nobs = ihw->nobs;  us_per_sample = (double)ihw->us_per_sample;
        break;
    case ADF_WIN30_CONV :
        h = adf_readHeader(fp);    ih = adf_readHeader(ifp);
        /* skip to the dir info */
        diroff = ADF_HEADER_SIZE+sizeof(int)*(h->nchannels+2*ih->nobs);
        d = adf_readDir(ifp, diroff);                     /* read the dir info    */
        nchannels = h->nchannels;  inchannels = ih->nchannels;
        nobs = ih->nobs;  us_per_sample = (double)ih->us_per_sample;
        break;
    case ADF_ADFX2013_UNCONV :
    case ADF_PCI6052E_UNCONV :
    case ADF_WIN30_UNCONV :
        fclose(fp);  fclose(ifp);
        mexErrMsgTxt("adf_readFileAndInfoByTime: not info file");
        break;    
    default:
        fclose(fp);  fclose(ifp);
        mexErrMsgTxt("adf_readFileAndInfoByTime: unknown file format");
        break;
    }

    if (inchannels != nchannels) {
        fclose(fp);  fclose(ifp);   freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);
        mexErrMsgTxt("Data file and Info file have different number of channels!");
    }

    if (channel >= nchannels) {
        fclose(fp);  fclose(ifp);   freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);
        mexErrMsgTxt("adf_readFileAndInfoByTime: channel out of range.");
    }
  
    if (obsindex >= nobs) {
        fclose(fp);  fclose(ifp);   freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);
        sprintf(buf,"adf_readFileAndInfoByTime: obs period (%d/%d) out of range.",
                obsindex, ih->nobs); 
        mexErrMsgTxt(buf);
    }
  
    start_samp = (int)(1000*startt/us_per_sample);
    if (dur > 0) {
        /* defaults to -1 --> whole obs*/
        samp_dur  = (int)(1000*dur/us_per_sample);  
    }

    if (ftype == ADF_ADFX2013_CONV) {
        adfx_getPartialObsPeriodFromRawFile(ihx, dx, fp, channel, obsindex, 
                                            start_samp, samp_dur, &n, &vals);
    } else if (ftype == ADF_PCI6052E_CONV) {
        adfw_getPartialObsPeriodFromRawFile(ihw, dw, fp, channel, obsindex, 
                                            start_samp, samp_dur, &n, &vals);
    } else {
        // ftype == ADF_WIN30_CONV
        adf_getPartialObsPeriodFromBlocks(ih, d, fp, channel, obsindex, 
                                          start_samp, samp_dur, &n, &vals);
    }
    fclose(fp);  fclose(ifp);   freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);

    /* Create a matrix for the return argument */ 
    ADF_OUT = mxCreateDoubleMatrix(1, n/decimate, mxREAL);
    dp = mxGetPr(ADF_OUT);
    if (ftype == ADF_WIN30_CONV) {
        for (p = dp, i = 0; i < n; i+=decimate) *p++ = vals[i]-offset;
    } else {
        for (p = dp, i = 0; i < n; i+=decimate) *p++ = vals[i];
    }
    free(vals);

    ADF_LENGTH = mxCreateDoubleMatrix(1, 1, mxREAL);
    ADF_RATE = mxCreateDoubleMatrix(1, 1, mxREAL);
    lengthptr = mxGetPr(ADF_LENGTH); 
    rateptr = mxGetPr(ADF_RATE);
    *lengthptr = (double) n/decimate;
    *rateptr = (double) us_per_sample;
  
    return;
}


void freeBuffers(ADF_HEADER  *h,  ADF_HEADER  *ih,  ADF_DIR  *d,
                 ADFW_HEADER *hw, ADFW_HEADER *ihw, ADFW_DIR *dw,
                 ADFX_HEADER *hx, ADFX_HEADER *ihx, ADFX_DIR *dx )
{
    if (h   != NULL)   adf_freeHeader(h);
    if (ih  != NULL)   adf_freeHeader(ih);
    if (d   != NULL)   adf_freeDirectory(d);
    if (hw  != NULL)  adfw_freeHeader(hw);
    if (ihw != NULL)  adfw_freeHeader(ihw);
    if (dw  != NULL)  adfw_freeDirectory(dw);
    if (hx  != NULL)  adfx_freeHeader(hx);
    if (ihx != NULL)  adfx_freeHeader(ihx);
    if (dx  != NULL)  adfx_freeDirectory(dx);

    return;
}
