function [nport nobs sampt obslens portwidth] = adf_infodi(ADFFILE)
%ADF_INFODI - Get info about digital inputs.
%  [nport nobs sampt obslens portwidth] = adf_infodi(ADFFILE) gets information about 
%  digital input of the  given file.
%
%  EXAMPLE :
%    [nport nobs sampt obslens portwidth] = adf_infodi('d:/test.adfx')
%
%  VERSION :
%    0.90 01.03.13 YM  pre-release
%
%  See also adf_info adf_readdi

if nargin < 1,  eval(['help ' mfilename]); return;  end

if ~exist(ADFFILE,'file'),
  error('\nERROR %s: file not found, ''%s''.\n',mfilename,ADFFILE);
end

[nchan nobs sampt obslens adc2volts nport portwidth] = adf_info(ADFFILE);

return;
