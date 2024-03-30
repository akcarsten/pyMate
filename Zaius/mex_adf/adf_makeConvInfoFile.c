/*=================================================================
 *
 * adf_makeConvInfoFile.c
 * Finds all the block offsets, etc, but doesn't rewrite the sample points.
 * DAL AUG-00
 * YM  01-Sep-2000  supports adfw format
 * YM  07-Oct-2002  moves APIs in adfapi2.c to adfapi.c
 * YM  06-Dec-2012  supports adfx format.
 * To compile in Matlab:
 *    >> mex adf_makeConvInfoFile.c adfapi.c adfwapi.c adfxapi.c
 *
 *=================================================================*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "adfapi.h"
#include "adfwapi.h"  // 01-Sep-2000 YM
#include "adfxapi.h"  // 01-Sep-2000 YM
#include "matrix.h"
#include "mex.h"

/* Input Arguments */
#define	FILE_IN	        prhs[0]           // input the file name

/* Output Arguments */

#define	ADF_NCHAN       plhs[0]          // output the info
#define	ADF_NOBS        plhs[1]
#define ADF_SAMPT       plhs[2]


void mexFunction( int nlhs, mxArray *plhs[], 
		  int nrhs, const mxArray*prhs[] )
{     
    ADF_HEADER *h;
    ADF_DIR *d;
    ADFW_HEADER *hw;  // 01-Sep-2000 YM
    ADFW_DIR *dw;
    ADFX_HEADER *hx;
    ADFX_DIR *dx;
    int ftype;
    FILE *fp, *ofp;
    char *filename, buf[256], *convfilename;
    int status, buflen;
    double *samptptr, *nobsptr, *nchanptr;

    if (nrhs != 1) { 
        mexErrMsgTxt("Usage: adf_makeConvInfoFile(filename)"); 
    }

    /* Get the filename */
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_conv: first arg must be filename string"); 
    }
    /* Get the filename */
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_conv: first arg must be filename string"); 
    }
    buflen = (mxGetM(FILE_IN) * mxGetN(FILE_IN)) + 1;
    filename = mxCalloc(buflen, sizeof(char));
    convfilename = mxCalloc(buflen+4, sizeof(char));
    status = mxGetString(FILE_IN, filename, buflen);
    strcpy(convfilename, filename);
    strcpy(&convfilename[buflen-1],"info");
    if (status != 0)
        mexWarnMsgTxt("adf_makeConvInfoFile: not enough space, filename string is truncated.");
    fp = fopen(filename, "rb");
    if (!fp) mexErrMsgTxt("adf_makeConvInfo: file not found"); 

    // read magic and check file format
    ftype = adfx_getFileFormat(fp);
    if (status == -1) {
        mexErrMsgTxt("adf_makeConvInfoFile: failed to read header");
    }
    h = NULL;  hw = NULL;  hx = NULL;
    switch (ftype) {
    case ADF_ADFX2013_UNCONV :
        hx = adfx_readHeader(fp);
        break;
    case ADF_PCI6052E_UNCONV :
        hw = adfw_readHeader(fp);
        break;
    case ADF_WIN30_UNCONV :
        h = adf_readHeader(fp);
        break;
    case ADF_ADFX2013_CONV :
    case ADF_PCI6052E_CONV :
    case ADF_WIN30_CONV :
        fclose(fp);
        sprintf(buf, "adf_makeConvInfoFile: file %s appears to be already converted", filename);
        mexErrMsgTxt(buf);
        break;
    default :
        fclose(fp);
        mexErrMsgTxt("adf_makeConvInfoFile: unknown file format");
        break;
    }

    if (h == NULL && hw == NULL && hx == NULL) {
        fclose(fp);
        sprintf(buf, "adf_makeConvInfoFile: unable to read adf/adfw/adfx file %s", filename);
        mexErrMsgTxt(buf);
        return;
    }

    ofp = fopen(convfilename, "wb");
    if (!ofp) {
        fclose(fp);
        if (h  != NULL)   adf_freeHeader(h);
        if (hw != NULL)  adfw_freeHeader(hw);
        if (hx != NULL)  adfx_freeHeader(hx);
        sprintf(buf,"adf_makeConvInfoFile: error opening output file %s", convfilename);
        mexErrMsgTxt(buf);
        return;
    }

    d = NULL; dw = NULL;  dx = NULL;
    if (ftype == ADF_ADFX2013_UNCONV) {
        dx = adfx_createDirectory(fp, hx);
        adfx_mkConvInfoFile(hx, dx, fp, ofp);
        adfx_freeDirectory(dx);
    } else if (ftype == ADF_PCI6052E_UNCONV) {
        dw = adfw_createDirectory(fp, hw);
        adfw_mkConvInfoFile(hw, dw, fp, ofp);
        adfw_freeDirectory(dw);
    } else {
        // ftype == ADF_WIN30_UNCONV) {
        d = adf_createDirectory(fp, h);
        adf_mkConvInfoFile(h, d, fp, ofp);
        adf_freeDirectory(d);
    }
    fclose(fp);   fclose(ofp);

    ADF_NCHAN = mxCreateDoubleMatrix(1, 1, mxREAL);
    ADF_NOBS  = mxCreateDoubleMatrix(1, 1, mxREAL);
    ADF_SAMPT = mxCreateDoubleMatrix(1, 1, mxREAL);
    nchanptr = mxGetPr(ADF_NCHAN); 
    nobsptr  = mxGetPr(ADF_NOBS);
    samptptr = mxGetPr(ADF_SAMPT);

    if (ftype == ADF_ADFX2013_UNCONV) {
        *nchanptr = (double)hx->nchannels_ai;
        *nobsptr  = (double)hx->nobs;
        *samptptr = hx->us_per_sample/1000.;  /* in milliseconds */
    } else if (ftype == ADF_PCI6052E_UNCONV) {
        *nchanptr = (double)hw->nchannels;
        *nobsptr  = (double)hw->nobs;
        *samptptr = (double)hw->us_per_sample/1000.;  /* in milliseconds */
    } else {
        //ftype == ADF_WIN30_UNCONV
        *nchanptr = (double)h->nchannels;
        *nobsptr  = (double)h->nobs;
        *samptptr = (double)h->us_per_sample/1000.;  /* in milliseconds */
    }

    if (h != NULL)  adf_freeHeader(h);
    if (hw != NULL) adfw_freeHeader(hw);
    if (hx != NULL) adfx_freeHeader(hx);

    return;
}
