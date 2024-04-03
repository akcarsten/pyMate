function convadfw(patt)
%CONVADFW - To convert raw ADF/ADFW data in Matlab.
% PURPOSE : To convert raw ADF/ADFW data in Matlab.
% USAGE   : convadfw(patt)
% ARGS    : 'patt': A filename or wild card expression like '*.adf'
% REQUIREMENT : cnvadfw.dll
% VERSION : 1.00  28-Mar-2001  YM
%
% NOTE : this function is obsolete, use convadfx() instead.
%
% See also ADF_INFO ADF_READ


convadfx(patt)


% if nargin < 1
%   fprintf('Usage: convadfw(patt)\n');
%   return;
% end

% adffiles = dir(patt);
% rpath  = subGetDirname(patt);
% for i=1:length(adffiles)
%   rfile = adffiles(i).name;
%   if ~adffiles(i).isdir
%     sfile = sprintf('%s/cvt/%s',rpath,rfile);
%     cnvadfw(rfile,sfile);
%     fprintf('%s converted %s to %s\n',subGetTimeStr,rfile,sfile);
%   end
% end


% %%% subfunction to get a directory from 'patt'
% function dirname = subGetDirname(patt)
% dirname = '.';
% token = findstr(strrep(patt,'\','/'), '/');
% if ~isempty(token)
%   dirname = patt(1:token(length(token))-1);
% end

% %%% subfunction to get the time as a string
% function tstr = subGetTimeStr()
% t = fix(clock);
% if length(t) == 1
%   h = fix(t/3600);
%   m = mod(fix(t/60),60);
%   s = mod(t,60);
% else
%   h = t(4);
%   m = t(5);
%   s = t(6);
% end
% tstr = sprintf('%02d:%02d:%02d',h,m,s);
