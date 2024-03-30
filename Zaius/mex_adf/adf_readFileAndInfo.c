/*=================================================================
 *
 * adf_readFileAndInfo.c
 * Reads the raw file using the obsper block information from the 
 * accompanying 'info' file in the same directory.
 * DAL AUG-00
 * YM  01-Sep-2000  supports adfw format
 * YM  07-Oct-2002  moves APIs in adfapi2.c to adfapi.c
 * YM  06-Dec-2012  supports adfx format
 * To compile in Matlab:
 *    >> mex adf_readFileAndInfo.c adfapi.c adfwapi.c adfxapi.c
 *
 *=================================================================*/
/* $Revision: 1.0 $ */

#include <math.h>
#include <stdio.h>
#include "adfapi.h"
#include "adfwapi.h"  // 01-Sep-2000 YM
#include "adfxapi.h"  // 06-Dec-2012 YM
#include "matrix.h"
#include "mex.h"

/* Input Arguments */

#define	FILE_IN	        prhs[0]
#define	INDEX_IN        prhs[1]
#define	CHAN_IN	        prhs[2]
#define	DECIMATE_IN     prhs[3]

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
    ADFW_HEADER *hw, *ihw;      // 01-Sep-2000 YM
    ADFW_DIR *dw;
    ADFX_HEADER *hx, *ihx;
    ADFX_DIR *dx;
    int ftype, nchannels, inchannels, nobs;
    char *filename, *ifilename, buf[128];
    int obsindex, channel = 0;
    int status, buflen, i;
    short *vals;
    double *dp, *p;
    int n, decimate = 1;
    double *lengthptr, *rateptr;
    double offset = (double) (1<<(resolution-1));
    int diroff;
    char aitype;
    int32_t *ivals;
    int64_t *lvals;

    /* Check for proper number of arguments */
  
    if (nrhs < 3 || nlhs > 3)  
        mexErrMsgTxt("Usage: [vals length rate] = adf_readFileAndInfo file obs chan [decimate]"); 
  
    /* Get the filename */
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_readFileAndInfo: first arg must be filename string"); 
    }
    buflen = (int)(mxGetM(FILE_IN) * mxGetN(FILE_IN)) + 1;
    filename = mxCalloc(buflen, sizeof(char));
    ifilename = mxCalloc(buflen+4, sizeof(char));
    status = mxGetString(FILE_IN, filename, buflen);
    strcpy(ifilename, filename);
    strcpy(&ifilename[buflen-1],"info");
  
    if (status != 0)
        mexWarnMsgTxt("adf_readFileAndInfo: filename string is truncated.");
  
    /* Get the obs index */
    obsindex = (int) mxGetScalar(INDEX_IN);

    if (nrhs > 2) {
        channel = (int) mxGetScalar(CHAN_IN);
    }
    if (nrhs > 3) {
        decimate = (int) mxGetScalar(DECIMATE_IN);
    }

    fp = fopen(filename, "rb");
    if (!fp) mexErrMsgTxt("adf_readFileAndInfo: data file not found"); 

    ifp = fopen(ifilename, "rb");
    if (!ifp) {
        fclose(fp);  mexErrMsgTxt("adf_readFileAndInfo: info file not found"); 
    }

    h  = NULL;  ih  = NULL;  d  = NULL;
    hw = NULL;  ihw = NULL;  dw = NULL;
    hx = NULL;  ihx = NULL;  dx = NULL;
    ftype = adfx_getFileFormat(fp);
    switch (ftype) {
    case ADF_ADFX2013_UNCONV :
        hx = adfx_readHeader(fp);  ihx = adfx_readHeader(ifp);
        dx = adfx_readDirEx(ifp,ihx);       /* read the dir info    */
        nchannels = hx->nchannels_ai;  inchannels = ihx->nchannels_ai;  nobs = hx->nobs;
        break;
    case ADF_PCI6052E_UNCONV :
        hw = adfw_readHeader(fp);  ihw = adfw_readHeader(ifp);
        /* skip to the dir info */
        diroff = ADFW_HEADER_SIZE+sizeof(long)*(hw->nchannels+2*ihw->nobs);
        dw = adfw_readDir(ifp, diroff);       /* read the dir info    */
        nchannels = hw->nchannels;  inchannels = ihw->nchannels;  nobs = hw->nobs;
        break;
    case ADF_WIN30_UNCONV :
        h = adf_readHeader(fp);    ih = adf_readHeader(ifp);
        /* skip to the dir info */
        diroff = ADF_HEADER_SIZE+sizeof(int)*(h->nchannels+2*ih->nobs);
        d = adf_readDir(ifp, diroff);       /* read the dir info    */
        nchannels = h->nchannels;  inchannels = ih->nchannels;  nobs = h->nobs;
        break;
    case ADF_ADFX2013_CONV :
    case ADF_PCI6052E_CONV :
    case ADF_WIN30_CONV :
        fclose(fp);  fclose(ifp);
        sprintf(buf, "adf_readFileAndInfo: file %s appears to be already converted", 
                filename);
        mexErrMsgTxt(buf);
        break;
    default :
        fclose(fp);  fclose(ifp);
        mexErrMsgTxt("adf_readFieAndInfo: unknown file format");
        break;
    }

    if (inchannels != nchannels) {
        fclose(fp);  fclose(ifp);  freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);
        mexErrMsgTxt("Data file and Info file have different number of channels!");
    }

    if (channel >= nchannels) {
        fclose(fp);  fclose(ifp);  freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);
        mexErrMsgTxt("adf_readFileAndInfo: channel out of range.");
    }

    if (obsindex >= nobs) {
        fclose(fp);  fclose(ifp);  freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);
        sprintf(buf,"adf_readFileAndInfo: obs period (%d/%d) out of range.",
                obsindex, nobs); 
        mexErrMsgTxt(buf);
    }

    if (ftype == ADF_ADFX2013_UNCONV) {
        adfx_getObsPeriodFromRawFile(ihx, dx, fp, channel, obsindex, &n, &vals, &aitype);
    } else if (ftype == ADF_PCI6052E_UNCONV) {
        aitype = 's';
        adfw_getObsPeriodFromRawFile(ihw, dw, fp, channel, obsindex, &n, &vals);
    } else {
        // ftype == ADF_WIN30_UNCONV
        aitype = 's';
        adf_getObsPeriodFromBlocks(ih, d, fp, channel, obsindex, &n, &vals);
    }
    /* close files */
    fclose(fp);  fclose(ifp);

    /* Create a matrix for the return argument */ 
    ADF_OUT = mxCreateDoubleMatrix(1, n/decimate, mxREAL);
    ADF_LENGTH = mxCreateDoubleMatrix(1, 1, mxREAL);
    ADF_RATE = mxCreateDoubleMatrix(1, 1, mxREAL);
    lengthptr = mxGetPr(ADF_LENGTH); 
    rateptr = mxGetPr(ADF_RATE);
    *lengthptr = (double) n/decimate;
    dp = mxGetPr(ADF_OUT);
    if (ftype == ADF_WIN30_UNCONV) {
        for (p = dp, i = 0; i < n; i+=decimate) *p++ = vals[i]-offset;
        *rateptr = (double) ih->us_per_sample;
    } else {
        switch (aitype) {
        case 's':
            for (p = dp, i = 0; i < n; i+=decimate) *p++ = (double)vals[i];
            break;
        case 'i':
            ivals = (int32_t *)vals;
            for (p = dp, i = 0; i < n; i+=decimate) *p++ = (double)ivals[i];
            break;
        case 'l':
            lvals = (int64_t *)vals;
            for (p = dp, i = 0; i < n; i+=decimate) *p++ = (double)lvals[i];
            break;
        }
        *rateptr = (double) ihw->us_per_sample;
    }
    free(vals);
    freeBuffers(h,ih,d,hw,ihw,dw,hx,ihx,dx);
  
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
