function obslens = adf_getInfoObsLengths(filename)
% PURPOSE :
% USAGE :
% SEEALSO : adf_getInfo
% VERSION :

if nargin ~= 1, help adf_getInfoObsLengths;  return;  end

[nchan nobs sampt obslens] = adf_getInfo(filename);
