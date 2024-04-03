/*
 * NAME
 *   adfapi.h
 *
 * DESCRIPTION
 *   Prototypes and structure definition for manipulating adf files.
 *
 * DETAILS
 *   The adfapi describes two versions of adf files, one unsorted, one
 * sorted.  The unsorted version (identified by the magic number in
 * the first four bytes) is the file saved directly by the streamer on
 * the QNX system.  It's format is as follows:
 *
 *                        Unconverted ADF File 
 * HEADER
 *   Bytes 0-40: Header
 *     char magic[4];  (10, 16, 19, 93)
 *     float version;
 *     char nchannels;
 *     char channels[16];
 *     int numconv;
 *     int prescale;
 *     int clock;
 *     float us_per_sample;
 *   Bytes 41-255: Reserved
 *
 * DATA
 *   Blocks of numconv*nchannels samples
 *   Each block contains the A/D points for each channel (12bit in shorts)
 *   In addition, markers appear in the 15th and 14th bit showing
 *     observation period start and stop
 *
 *
 *                          Converted ADF File
 *
 * HEADER
 *   Bytes 0-40: Header
 *     char magic[4];  ( 7, 8, 19, 67)
 *     float version;
 *     char nchannels;
 *     char channels[16];
 *     int numconv;
 *     int prescale;
 *     int clock;
 *     float us_per_sample;
 *     int nobs;
 *   Bytes 41-255: Reserved
 *
 * DIRECTORY
 *   Bytes 256-(256+(nchannels*4)): Offsets to beginning of each channel
 *   Bytes XXX-(XXX+(nobs*4))     : Sample counts for each obs
 *   Bytes XXX-(XXX+(nobs*4))     : Byte offsets for each obs
 *                                     (these are with respect to chn offs)
 * 
 * DATA
 *   Blocks of samples: (12 bit A/D vals in shorts)
 *    Channel 0
 *     Obs 0
 *     Obs 1
 *     Obs 2
 *     ...
 *    Channel 1
 *     Obs 0
 *     Obs 1
 *     ...
 *
 *
 * AUTHOR
 *   DLS 1/99
 *   YM  07-Oct-2002   moves APIs from adfapi2.c wrote by DAL
 *     adf_readDir, adf_mkConvInfoFile, adf_getObsPeriodFromBlocks
 *     adf_getPartialObsPeriodFromBlocks, adf_getObsPeriodPartial
 *
 */

#ifndef _ADFAPI_H_INCLUDED
#define _ADFAPI_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Identify state changes in the analog data */
#define STREAM_ON  (1<<15)
#define STREAM_OFF (1<<14)

enum { ADF_OBS_RANGE_ERROR = -2, ADF_CHANNEL_RANGE_ERROR = -1 };

#pragma pack(1)
typedef struct {
  int nobs;
  int *startblocks;
  int *startoffs;
  int *stopblocks;
  int *stopoffs;
} ADF_DIR;

typedef struct _adf_header { 
  char magic[4];
  float version;
  char nchannels;
  char channels[16];
  int numconv;
  int prescale;
  int clock;
  float us_per_sample;
  int nobs;           /* Here down only for converted adf files */
  int *channeloffs;   /* No. bytes to the beginning of each chn */
  int *obscounts;     /* No. samples in each obs                */
  int *offsets;       /* No. bytes to each sample from chnloff  */
} ADF_HEADER;
#pragma pack()

#define ADF_VERSION     (1.01)
#define ADF_HEADER_SIZE  (256)
#define ADF_DIR_SIZE     (128)

/* read header info and return pointer to header if it exists */

ADF_HEADER *adf_readHeader(FILE *fp);
void adf_freeHeader(ADF_HEADER *h);
ADF_DIR *adf_newDirectory(int n);
void adf_freeDirectory(ADF_DIR *d);
ADF_DIR *adf_createDirectory(FILE *fp, ADF_HEADER *h);
int  adf_convertFile(ADF_HEADER *h, ADF_DIR *d, FILE *fp, FILE *ofp);
int  adf_thresholdObs(ADF_HEADER *h, FILE *fp, int channel, int obsp,
					int threshold, int dir, float pre, float post, float skiptime,
					int *nsamps, int *nregs, short **regvals, float **times);
int  adf_getObsPeriod(ADF_HEADER *h, FILE *fp, int channel, int obsp,
					int *nsamps, short **vals);
void adf_printInfo(ADF_HEADER *h, FILE *fp);
void adf_printDirectory(ADF_HEADER *h, ADF_DIR *d);

ADF_DIR *adf_readDir(FILE *fp, int offset);
int adf_mkConvInfoFile(ADF_HEADER *h, ADF_DIR *d, FILE *fp, FILE *ofp);
int adf_getObsPeriodFromBlocks(ADF_HEADER *h, ADF_DIR *d, FILE *fp, 
                       int channel, int obsp,int *nsamps, short **vals);
int adf_getPartialObsPeriodFromBlocks(ADF_HEADER *h, ADF_DIR *d, FILE *fp, 
                       int channel, int obsp, int start_samp, 
                       int samp_dur,int *nsamps, short **vals);
int adf_getObsPeriodPartial(ADF_HEADER *h, FILE *fp, int channel, int obsp,
                       int startindx, int totindx, int *nsamps, short **vals);

#ifdef __cplusplus
}
#endif

#endif // end of _ADFAPI_H_INCLUDED
