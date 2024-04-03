function RET = debug_adfx_raw(adffile)

hdr = adf_readHeader(adffile);

nch = hdr.nchannels_ai + hdr.nchannels_di;

if all(hdr.magic == [12 18 21 95]),
  unconv = 1;
elseif all(hdr.magic == [ 9 10 21 69]),
  unconv = 0;
else
  keyboard
end


chanoffs = zeros(1,nch);
for iCh = 2:nch,
  switch lower(hdr.data_type(iCh-1))
   case 'c'
    chanoffs(iCh) = chanoffs(iCh-1) + 1;
   case 's'
    chanoffs(iCh) = chanoffs(iCh-1) + 2;
   case 'i'
    chanoffs(iCh) = chanoffs(iCh-1) + 4;
   case 'l'
    chanoffs(iCh) = chanoffs(iCh-1) + 8;
  end
end



fid = fopen(adffile,'rb');

try
  

fseek(fid,hdr.offset2data,'bof');

if unconv,
  nread = 5000*2;
  DAT = zeros(nread,nch);
  for S = 1:size(DAT,1),
    for iCh = 1:nch,
      switch lower(hdr.data_type(iCh))
       case 'c'
        tmpv = fread(fid, 1, 'uint8');
       case 's'
        tmpv = fread(fid, 1, 'int16');
       case 'i'
        tmpv = fread(fid, 1, 'int32');
       case 'l'
        tmpv = fread(fid, 1, 'int64');
      end
      DAT(S,iCh) = tmpv;
    end
  end
else
  DAT = zeros(hdr.obscounts(1),nch);
  for iCh = 1:nch,
    tmpoffs = hdr.offset2data + chanoffs(iCh)*hdr.obscounts(1);
    fseek(fid, tmpoffs, 'bof');
    if hdr.channels(iCh) > 0
      switch lower(hdr.data_type(iCh))
       case 'c'
        tmpv = fread(fid, size(DAT,1), 'int8');
       case 's'
        tmpv = fread(fid, size(DAT,1), 'int16');
       case 'i'
        tmpv = fread(fid, size(DAT,1), 'int32');
       case 'l'
        tmpv = fread(fid, size(DAT,1), 'int64');
      end
    else
      switch lower(hdr.data_type(iCh))
       case 'c'
        tmpv = fread(fid, size(DAT,1), 'uint8');
       case 's'
        tmpv = fread(fid, size(DAT,1), 'uint16');
       case 'i'
        tmpv = fread(fid, size(DAT,1), 'uint32');
       case 'l'
        tmpv = fread(fid, size(DAT,1), 'uint64');
      end
    end
    DAT(:,iCh) = tmpv(:);
  end
end

catch
  fclose(fid);
  keyboard
end


fclose(fid);



RET.hdr = hdr;
RET.dat = DAT;
