%ADF_INFO - Get info of ADF/ADFW/ADFX file.
%  [NChan NObs SampT ObslenPts Adc2Volts nport portwidth] = ADF_INFO(FILENAME) returns 
%  information of the given ADF/ADFW/ADFX file.
%
%  OUTPUT :
%    NChan     : num. of analog-input channels.
%    NObs      : num. of observation periods.
%    SampT     : sampling time in msec.
%    ObslenPts : obs. length in sample points. 
%    Adc2Volts : scaling factor to convert AD byinary to voltages.
%    NPort     : num. of digital ports
%    PortWidth : port width in bits
%
%  See also adf_read adf_readdi
