function [nchan nobs sampt obslens] = adf_getInfo(filename)
% PURPOSE : To get some info from adfinfo/adfwinfo file
% USAGE :   [nchan nobs sampt obslens] = adf_getInfo(filename)
% SEEALSO : adf_info
% VERSION :

if nargin ~= 1, help adf_getInfo;  return;  end

filename = sprintf('%sinfo',filename);
[nchan nobs sampt obslens] = adf_info(filename);
