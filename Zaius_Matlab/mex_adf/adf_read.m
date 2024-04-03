%ADF_READ - Read out waveform in adf/adfw/adfx file
%  [Wv Npts SampT Adc2Volts] = ADF_READ(FILENAME, Obs, Chan, [start], [nsamples]) reads 
%  the waveform of the 'Chan' in 'Obs' from the given ADF/ADFW/ADFX file.
%
%  INPUT :
%    FILENAME  : adf/adfw/adfx file to read
%    Obs       : observation to read, 0 to NObs-1
%    Chan      : channel to read, 0 to NChan-1
%    start     : start index in samples for patial reading, 0 to ObslenPts-1
%    nsampels  : num. samples for patial reading, 1 to ObslenPts
%
%  OUTPUT :
%    Wv        : waveform in ADC unit
%    Npts      : num. of samples
%    SampT     : sampling time in msec
%    Adc2Volts : scaling factor to convert AD byinary to voltages.
%
%  See also adf_info adf_readdi
