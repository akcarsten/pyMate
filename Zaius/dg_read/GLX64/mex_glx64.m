  %
% mex_glx64.m : batch file to make all mex DLLs
%
% DAL 16-Sept-07

mex dg_read.c dynio.c df.c dfutils.c flip.c /usr/lib64/libz.so -I.

