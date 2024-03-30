function obslens = adf_obsLengths(filename)
% PURPOSE : To get an array of all observation lengths.
% USAGE :   obslens = adf_obsLengths(filename)
% SEEALSO : adf_info
% VERSION : 1.00 07-Oct-2002 YM/MPI

if nargin ~= 1, help adf_obsLengths;  return;  end

[nchan, nobs, sampt, obslens] = adf_info(filename);
