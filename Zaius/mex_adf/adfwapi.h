/*	adfwapi.h : This file represents api for adfw file
 *
 *  ver 1.00  30-May-2000  YM  modified from adfapi.h by DLS.
 *      1.01  01-Sep-2000  YM  add 'checkXXX' functions etc.
 *      1.02  17-Oct-2001  YM, make THRESHOLD adjustable
 *                             update ADFW_VERSION 1.00 to 1.01, adding threshold info to header
 *                             change THRESHOLD 23000(3.5V) to 16000(2.5V)
 *      1.03  26-Oct-2001 YM/DAL, new API called adfw_getObsPeriodPartial() by DAL
 *      1.04  07-Oct-2002 YM, adds 'adfw_' prefix to some APIs
 *      1.10  07-Mar-2013 YM, use 'int' instead of 'long'
 *
 *
 * NOTE :            win     linux    OSX
 *                  w32 w64 x32 x64 x32 x64
 *   sizeof(short)   2   2   2   2   2   2
 *   sizeof(int)     4   4   4   4   4   4
 *   sizeof(long)    4   4   4   8   4   8  <---- be careful...
 *   sizeof(double)  8   8   8   8   8   8
 */
/*
 *
 * HEADER : Unconverted ADFW File
 *   Bytes 0-75: Header
 *  4   char  magic[4];  (11, 17, 20, 94)
 *  4   float version;
 *  1   char  nchannels;
 * 16   char  channels[16];
 *  4   int   numconv;
 *  4   int   prescale;
 *  4   int   clock;
 *  4   float us_per_sample;
 *  4   int   nobs;
 *  1   char  resolution;
 *  1   char  input_range;
 * 16   char  chan_gains[16];
 *  4   float scan_rate;
 *  2   short samp_timebase;
 *  2   short samp_interval;
 *  2   short scan_timebase;
 *  2   short scan_interval;
 *  2   short trig_logic_high;
 *  2   short trig_logic_low;
 *   Bytes 80-255: Reserved
 *
 * DATA
 *   Each channel is aligned in every sampling.
 *   chan1,chan2,...,chan16--chan1,chan2,...,chan16--...
 *
 *
 *
 * HEADER : Converted ADFW File
 *   Bytes 0-75: Header
 *  4   char magic[4];  ( 8, 9, 20, 68)
 *  4   float version;
 *  1   char  nchannels;
 * 16   char  channels[16];
 *  4   int   numconv;
 *  4   int   prescale;
 *  4   int   clock;
 *  4   float us_per_sample;
 *  4   int   nobs;
 *  1   char  resolution;
 *  1   char  input_range;
 * 16   char  chan_gains[16];
 *  4   float scan_rate;
 *  2   short samp_timebase;
 *  2   short samp_interval;
 *  2   short scan_timebase;
 *  2   short scan_interval;
 *  2   short trig_logic_high;
 *  2   short trig_logic_low;
 *   Bytes 80-255: Reserved
 *
 * DIRECTORY
 *   Bytes 256-(256+(nchannels*4)): Offsets to beginning of each channel
 *   Bytes XXX-(XXX+(nobs*4))     : Sample counts for each obs
 *   Bytes XXX-(XXX+(nobs*4))     : Byte offsets for each obs
 *                                     (these are with respect to chn offs)
 * 
 * DATA
 *   Blocks of samples: (16 bit A/D vals in shorts)
 *    Channel 0
 *     Obs 0
 *     Obs 1
 *     Obs 2
 *     ...
 *    Channel 1
 *     Obs 0
 *     Obs 1
 *     ...
*/

#ifndef _ADFWAPI_H_INCLUDED
#define _ADFWAPI_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************/
/* file type */
#define ADF_UNKNOWN           (0)
#define ADF_WIN30_UNCONV      (10)
#define ADF_WIN30_CONV        (11)
#define ADF_PCI6052E_UNCONV   (20)
#define ADF_PCI6052E_CONV     (21)
/* threthold for conversion (cf. 2.5V (3072 in 12bits) for QNX streamer )*/
#define DAQ_ANALOG_HIGH       (16000)	// about 2.5V = 65536/10*2.5
#define DAQ_ANALOG_LOW        (6500)	// about 1.0V = 65536/10*1.0
/* threthold for streaming data */
#define DAQ_ANALOG_HIGH_STRMR (12000)	// about 1.8V = 65536/10*1.8
#define DAQ_ANALOG_LOW_STRMR  (5000)	// about 0.8V = 65536/10*0.8

/*************************************************/
/* adfw stuff */
#pragma pack(1)
typedef struct {
  int  nobs;
  int *startoffs;
  int *stopoffs;
} ADFW_DIR;

typedef struct _adfw_header { 
  char magic[4];
  float version;
  char nchannels;
  char channels[16];
  int numconv;
  int prescale;
  int clock;
  float us_per_sample;
  int nobs;                /* Here down only for converted adf files */
  /* extended part in ADFW_HEADER */
  char resolution;          /* AD converter resolution in bits        */
  char input_range;         /* input range in Volts                   */
  char chan_gains[16];      /* gain for each channel                  */
  float scan_rate;          /* sampling rate of each channel          */
  short samp_timebase;      /* sampling timebase of the NI boards     */
  short samp_interval;      /* sampling interval of the NI boards     */
  short scan_timebase;      /* timebase for scanning channels         */
  short scan_interval;      /* interval for scanning channels         */
  short trig_logic_high;    /* trigger threshold                      */
  short trig_logic_low;     /* trigger threshold                      */
  /* reserved part */
  int *channeloffs;        /* No. bytes to the beginning of each chn */
  int *obscounts;          /* No. samples in each obs                */
  int *offsets;            /* No. bytes to each sample from chnloff  */
} ADFW_HEADER;
#pragma pack()


#define ADFW_VERSION     (1.01)
#define ADFW_HEADER_SIZE (256)
#define ADFW_DIR_SIZE    (128)   // 01-Sep-2000  YM

/*************************************************/
/* prototypes */
ADFW_HEADER *adfw_readHeader(FILE *fp);
void adfw_freeHeader(ADFW_HEADER *h);
void adfw_initHeader(ADFW_HEADER *h, int converted);
ADFW_DIR *adfw_newDirectory(int n);
void adfw_freeDirectory(ADFW_DIR *d);
ADFW_DIR *adfw_createDirectory(FILE *fp, ADFW_HEADER *h);
ADFW_DIR *adfw_createDirectoryEx(FILE *fp, ADFW_HEADER *h, short logicH, short logicL);
int  adfw_convertFile(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, FILE *ofp);
int  adfw_thresholdObs(ADFW_HEADER *h, FILE *fp, int channel, int obsp,
                       int threshold, int dir, float pre, float post, float skiptime,
                       int *nsamps, int *nregs, short **regvals, float **times);
int  adfw_getObsPeriod(ADFW_HEADER *h, FILE *fp, int channel, int obsp,
                    int *nsamps, short **vals);
int  adfw_getObsPeriodPartial(ADFW_HEADER *h, FILE *fp, int channel, int obsp,
                    int startindx, int totindx, int *nsamps, short **vals);
void adfw_printInfo(ADFW_HEADER *h, FILE *fp);
void adfw_printDirectory(ADFW_HEADER *h, ADFW_DIR *d);


// file checker
int adfw_getFileFormat(FILE *fp);
int adfw_checkFileFormat(char *fname);
int adfw_checkMagicNumber(char *mgc);

// extended functions : 01-Sep-2000  YM made very small modification from DAL codes.
int adfw_mkConvInfoFile(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, FILE *ofp);
ADFW_DIR *adfw_readDir(FILE *fp, int offset);
int adfw_getObsPeriodFromRawFile(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, 
		       int channel, int obsp,int *nsamps, short **vals);
int adfw_getPartialObsPeriodFromBlocks(ADFW_HEADER *h, ADFW_DIR *d, FILE *fp, 
		       int channel, int obsp,int *nsamps, short **vals);

#ifdef __cplusplus
}
#endif

#endif	/* end of _ADFWAPI_H_INCLUDED	*/
