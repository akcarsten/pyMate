function convadfx(patt,varargin)
%CONVADFX - To convert raw ADF/ADFW/ADFX file(s) in Matlab.
%  CONVADFX(patt,...) converts raw ADF/ADFW/ADFX file(s) in Matlab.
%    'patt' can be a filename or wild card expression like '*.adfx'.
%
%  Supported options are :
%    'savedir' : directory for the converted file(s).
%
%  EXAMPLE :
%    >> convadfx('d:/mydata/E10ha1_001.adfw')
%    >> convadfx('d:/mydata_raw/*.adfw','savedir','d:/mydata')
%
%  REQUIREMENT :
%    cnvadfx.mexw32/mexw64
%
%  VERSION :
%    0.90  07-Dec-2012  YM  pre-release
%
%  See also adf_info adf_read

if nargin < 1,  eval(['help ' mfilename]); return;  end


SAVE_DIR = '';

for N = 1:2:length(varargin)
  switch lower(varargin{N})
   case {'savedir' 'save' 'folder'}
    SAVE_DIR = varargin{N+1};
  end
end

adffiles = dir(patt);
rpath = fileparts(patt);

if isempty(SAVE_DIR),
  SAVE_DIR = fullfile(rpath,'cvt');
end

if ~exist(SAVE_DIR,'dir')
  fprintf(' making dir ''%s''...',SAVE_DIR);
  mkdir(SAVE_DIR);
  fprintf(' done.\n');
end


for N = 1:length(adffiles)
  if any(adffiles(N).isdir),  continue;  end
  rfile = fullfile(rpath,    adffiles(N).name);
  sfile = fullfile(SAVE_DIR, adffiles(N).name);
  if strcmpi(rfile,sfile),
    error('ERROR %s: the same filename for read and save',mfilename);
  end
  ts = tic;
  fprintf('%s %3d/%d cnvadfx(%s, %s)...',...
          datestr(now,'HH:MM:SS'), N, length(adffiles), rfile, sfile);  drawnow;
  cnvadfx(rfile,sfile);
  te = toc(ts);
  fprintf(' done (%.3fs).\n',te);
end



return

