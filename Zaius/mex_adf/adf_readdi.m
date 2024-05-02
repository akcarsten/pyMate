%ADF_READDI - Read out pattern of the digital port in adfx file
%  [patt npts sampt portwidth] = ADF_READDI(FILENAME, Obs, Port, [start], [nsamples]) reads 
%  the pattern of the digital 'Port' in 'Obs' from the given ADFX file.
%
%  INPUT :
%    FILENAME  : adfx file to read
%    Obs       : observation to read, 0 to NObs-1
%    Port      : digital port to read, 0 to NPort-1
%    start     : start index in samples for patial reading, 0 to ObslenPts-1
%    nsampels  : num. samples for patial reading, 1 to ObslenPts
%
%  OUTPUT :
%    Patt      : pattern of the digital port
%    Npts      : num. of samples
%    SampT     : sampling time in msec
%    PortWidth : port width in bits
%
%  NOTE :
%    bitget(Patt,X) may be used to extract the Xth bit (1<=X<=PortWidth).
%
%  See also adf_info adf_read bitget
