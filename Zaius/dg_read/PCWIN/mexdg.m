  %
% mexdg.m : batch file to make all mex DLLs
%
% VERSION : 1.00  24-Nov-2001  YM
%
c = input('Link with static zlib? Y/N[N]: ','s');
if isempty(c), c = 'N';  end

fprintf('making dg_read... ');
switch lower(c)
 case 'y'
  % link with static zlib, no need of zlib.dll
  mex dg_read.c dynio.c df.c dfutils.c flip.c zlibstat.lib -I.` 
  fprintf(' done.\n');
 case 'n'
  % link with zlibdll.lib, requires zlib.dll somwhere
  mex dg_read.c dynio.c df.c dfutils.c flip.c zlibdll.lib -I.
  fprintf(' done.\n');
  fprintf('Make sure you have zlib.dll somewhere in your PATH.\n');
 otherwise
  fprintf('not supported yet\n');
end
