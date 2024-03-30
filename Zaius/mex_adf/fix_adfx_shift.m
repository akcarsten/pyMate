function [nshifts_12, nshifts_13] = fix_adfx_shift(ADFFILE,varargin)
%FIX_ADFX_SHIFT - Fix time shifts of waveforms in ADFX file.
%  FIX_ADFX_SHIFT(ADFFILE,...) fixes time shifts of waveform in ADFX which observed in 7T setup.
%
%  Supported options are:
%    'check'     : 0/1, check shifts (no file writing):  DO NOT RELY ON THIS, CHECK BY YOURSELF.
%    'dev1chans' : a vector of channels in device1 to check 
%    'dev2chans' : a vector of channels in device2 to check 
%    'dev3chans' : a vector of channels in device3 to check 
%    'useEnding' : 0/1, use the ending period
%    'ndata'     : a number of data points used for checking
%
%    'write'     : 0/1, write to a new adfx file
%    'overwrite' : 0/1, overwrite or ask
%    'plot'      : 0/1, plot the figure or not
%    'plotchans' : a vector of channels to plot
%    'dev2shift' : shift-size for channels in device2
%    'dev3shift' : shift-size for channels in device3
%
%  NOTES :
%   - Default shits from 20170117_test_001.adfx are -1650 (dev2) and -1423 (dev3).  These
%     values may differ from session to session.
%   - To get shift values, DO NOT RELY ON THE CHEKING FUNCTION OF THIS. Check by your own codes.
%   - You may need to update the corresponiding session file to set a new file (EXPP(X).physfile).
%
%  EXAMPLE :
%    % rough estimation of shifts by internal checking function
%    >> fix_adfx_shift('d:/temp/20170117_test_001.adfx','check',1)
%    % shift dev2/3 data and write as a new adfx.
%    >> fix_adfx_shift('d:/temp/20170117_test_001.adfx','plot',1,'write',0)
%    >> fix_adfx_shift('d:/temp/20170117_test_001.adfx','plot',1,'overwrite',1)
%    % setting shifts as options
%    >> fix_adfx_shift('d:/temp/20170117_test_001.adfx','dev2shift',-1650,'dev3shift',-1423)
%
%  VERSION :
%    0.90 17.01.2017 YM  pre-release
%    0.91 20.01.2017 YM  plots MRI events, if dgz.
%
%  See also adf_readHeader circshift check_pvphys check_adfx_mrevent


if nargin < 1, eval(['help ' mfilename]); return;  end

% options
DO_CHECK_SHIFTS = 0;
USE_ENDING_PERIOD = 1;
NDATA_TO_CHECK  = [];

CHECK_DEV1CHANS = [];
CHECK_DEV2CHANS = [];
CHECK_DEV3CHANS = [];
ASK_OVERWRITE = 1;
DO_WRITE = 1;
DO_PLOT  = 0;
PLOT_CHANS = [];
SHIFT_DEV1 = 0;
SHIFT_DEV2 = -1650;  % determined from 20170117_test_001.adfx, may diffrent for others
SHIFT_DEV3 = -1423;  % determined from 20170117_test_001.adfx, may diffrent for others
VERBOSE    = 1;

for N = 1:2:length(varargin),
  switch lower(varargin{N})
   case {'check'}
    DO_CHECK_SHIFTS = any(varargin{N+1});
    if any(DO_CHECK_SHIFTS),  VERBOSE = 1;  end
   case {'dev1chans'}
    CHECK_DEV1CHANS = varargin{N+1};
   case {'dev2chans'}
    CHECK_DEV2CHANS = varargin{N+1};
   case {'dev3chans'}
    CHECK_DEV3CHANS = varargin{N+1};
   case {'endingperiod','ending_period'}
    USE_ENDING_PERIOD = any(varargin{N+1});
   case {'ndata'}
    NDATA_TO_CHECK = varargin{N+1};
   
   case {'overwrite'}
    ASK_OVERWRITE = ~any(varargin{N+1});
   case {'write'}
    DO_WRITE = any(varargin{N+1});
   case {'plot'}
    DO_PLOT = any(varargin{N+1});
   case {'plotchan' 'plotchans'}
    PLOT_CHANS = varargin{N+1};
   case {'shift1' 'shiftdev1' 'dev1shift'}
    SHIFT_DEV1 = varargin{N+1};
   case {'shift2' 'shiftdev2' 'dev2shift'}
    SHIFT_DEV2 = varargin{N+1};
   case {'shift3' 'shiftdev3' 'dev3shift'}
    SHIFT_DEV3 = varargin{N+1};
   case {'verbose'}
    VERBOSE = any(varargin{N+1});
  end
end


if ~exist(ADFFILE,'file'),
  error('%s: ''%s'' not found.\n',mfilename,ADFFILE);
end
[fp, fr, fe] = fileparts(ADFFILE);
if ~strcmpi(fe,'.adfx'),
  error('%s: file is not ADFX.',mfilename);
end
ADFFILE = fullfile(fp,[fr fe]);
newADFFILE = fullfile(fp,sprintf('%s.fixed%s',fr,fe));

if any(VERBOSE),
  fprintf('%s %s: %s:',datestr(now,'HH:MM:SS'), mfilename, ADFFILE);
end

if any(VERBOSE),  fprintf(' hdr.');  end
HDR = adf_readHeader(ADFFILE);

if sub_check_header(HDR) == 0,
  if any(VERBOSE),  fprintf(' skipped.\n');  end
  return;
end

nch = HDR.nchannels_ai + HDR.nchannels_di;
chanoffs = zeros(1,nch);
for iCh = 2:nch,
  switch lower(HDR.data_type(iCh-1))
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

dgz = [];
if any(DO_CHECK_SHIFTS) || any(DO_PLOT),
  dgzfile = fullfile(fp,[fr '.dgz']);
  if exist(dgzfile,'file')
    dgz = dg_read(dgzfile);
    t_end = dgz.e_times{1}(dgz.e_types{1} == 20);  % in msec
    dgz.tfactor = (HDR.obscounts(1)*(HDR.us_per_sample/1000))/t_end;
    t_mri = dgz.e_times{1}(dgz.e_types{1} == 46);  % in msec
    t_mri = t_mri * dgz.tfactor;
    dgz.t_mri_pts = round(t_mri/(HDR.us_per_sample/1000));
    clear t_mri;
  end
end



% check shifts then return (no file writing)
if any(DO_CHECK_SHIFTS),
  [nshifts_12, nshifts_13] = sub_check_shifts(ADFFILE,HDR,chanoffs,dgz,...
                   CHECK_DEV1CHANS,CHECK_DEV2CHANS,CHECK_DEV3CHANS,...
                   USE_ENDING_PERIOD,NDATA_TO_CHECK);
  fprintf(' done.\n');
  return
end



if any(DO_WRITE),
  if any(VERBOSE),
    [fp2,fr2,fe2] = fileparts(newADFFILE);
    fprintf('--> %s:',[fr2,fe2]);
  end
  if exist(newADFFILE,'file') && any(ASK_OVERWRITE),
    c = input(' Overwrite? Y/N[Y]: ', 's');
    if isempty(c), c = 'N';  end
    if ~isequal(lower(c),'y'),
      if any(VERBOSE),  fprintf(' user-aborted (no overwrite).\n');  end
      return
    end
  end
  if any(VERBOSE),  fprintf(' hdr.');  end
  sub_write_header(ADFFILE,newADFFILE,HDR);
end

if any(DO_PLOT),
  if isempty(PLOT_CHANS),
    PLOT_CHANS(1) = 1;
    if any(HDR.devices == 1),
      PLOT_CHANS(2) = min(find(HDR.devices == 1));
    end
    if any(HDR.devices == 2),
      PLOT_CHANS(3) = min(find(HDR.devices == 2));
    end
  end
  COLORS = [];
end

if any(VERBOSE), fprintf(' [%d:ai+di]',nch);  end

for iObs = 1:HDR.nobs,
  if any(DO_PLOT),
    hfig = figure('Name',sprintf('%s: obs=%d',[fr,fe], iObs));
    tmppos = get(hfig,'position');  tmppos(3) = tmppos(3)*1.5;
    set(hfig,'position',tmppos);  clear tmppos;
    if isempty(COLORS),  COLORS = lines(64);  end
    hax1 = subplot(2,2,1);
    hax2 = subplot(2,2,2);
    hax3 = subplot(2,2,3);
    hax4 = subplot(2,2,4);
  end
  for iCh = 1:nch,
    if ~any(DO_WRITE) && ~any(PLOT_CHANS == iCh),  continue;  end
    % get the precision
    switch lower(HDR.data_type(iCh))
     case 'c'
      tmpprecR = 'uint8=>uint8';
      tmpprecW = 'uint8';
     case 's'
      tmpprecR = 'int16=>int16';
      tmpprecW = 'int16';
     case 'i'
      tmpprecR = 'int32=>int32';
      tmpprecW = 'int32';
     case 'l'
      tmpprecR = 'int64=>int64';
      tmpprecW = 'int64';
    end
    if any(VERBOSE), fprintf('.');  end
    
    % read the waveform
    tmpoffs = HDR.offset2data + chanoffs(iCh)*HDR.obscounts(iObs);
    fid = fopen(ADFFILE,'rb');
    fseek(fid,tmpoffs,'bof');
    tmpwv = fread(fid,HDR.obscounts(iObs),tmpprecR);
    fclose(fid);
    
    if any(DO_PLOT),
      tmpc = find(PLOT_CHANS == iCh);
      if any(tmpc),
        set(hfig,'CurrentAxes',hax1);
        plot(tmpwv(1:5000),'color',COLORS(tmpc,:));
        hold on;
        set(hfig,'CurrentAxes',hax2);
        plot((-5000:0)+length(tmpwv),tmpwv(end-5000:end),'color',COLORS(tmpc,:));
        hold on;
      end
    end
    
    % shift it if needed
    tmpwv = tmpwv(:);
    if HDR.devices(iCh) == 0,
      shiftsize = SHIFT_DEV1;
    elseif HDR.devices(iCh) == 1,
      shiftsize = SHIFT_DEV2;
    elseif HDR.devices(iCh) == 2,
      shiftsize = SHIFT_DEV3;
    end
    if shiftsize ~= 0,
      tmpwv = circshift(tmpwv,shiftsize);
      % if negative shift (rightward), then fill out the noisy part by flipping.
      if shiftsize < 0,
        tmpi = (0:-1:shiftsize+1) + length(tmpwv)+shiftsize-1;
        tmpwv(end+shiftsize+1:end) = tmpwv(tmpi);
      end
    end
   
    if any(DO_WRITE),
      fid = fopen(newADFFILE,'a+');
      fseek(fid,0,'eof');
      fwrite(fid,tmpwv,tmpprecW);
      fclose(fid);
    end
    if any(DO_PLOT) && any(tmpc),
      set(hfig,'CurrentAxes',hax3);
      plot(tmpwv(1:5000),'color',COLORS(tmpc,:));
      hold on;
      set(hfig,'CurrentAxes',hax4);
      plot((-5000:0)+length(tmpwv),tmpwv(end-5000:end),'color',COLORS(tmpc,:));
      hold on;
    end
  end

  if any(DO_PLOT),
    tmptxt = {};
    for K = PLOT_CHANS,
      tmptxt{end+1} = sprintf('Dev%d-Ch%d',HDR.dev_numbers(HDR.devices(K)+1),K);
    end
    for h = [hax1 hax2 hax3 hax4],
      set(hfig,'CurrentAxes',h);
      if h == hax1,
        title('original BEGIN');
        set(h,'xlim',[1 5000]);
      elseif h == hax2,
        title('original END');
        set(h,'xlim',[length(tmpwv)-5000 length(tmpwv)]);
      elseif h == hax3,
        title(sprintf('after circshit (dev1=%d,dev2=%d,dev3=%d) BEGIN',SHIFT_DEV1,SHIFT_DEV2,SHIFT_DEV3));
        set(h,'xlim',[1 5000]);
      elseif h == hax4,
        title(sprintf('after circshit (dev1=%d,dev2=%d,dev3=%d) END',SHIFT_DEV1,SHIFT_DEV2,SHIFT_DEV3));
        set(h,'xlim',[length(tmpwv)-5000 length(tmpwv)]);
      end
      xlabel(sprintf('Time (1000pts=%gms)',HDR.us_per_sample));  ylabel('ADC Unit');
      grid on;
      legend(tmptxt);
      if iObs == 1 && ~isempty(dgz),
        xlim = get(h,'xlim');
        t_mri_pts = dgz.t_mri_pts;
        t_mri_pts = t_mri_pts(t_mri_pts >= xlim(1) & t_mri_pts <= xlim(2));
        for K = 1:length(t_mri_pts),
          t = t_mri_pts(K);
          line([t t],ylim,'color',[0.1 0.1 0.1]);
        end
      end
    end
  end

end


% write a log-file
if any(DO_WRITE),
  sub_write_infotxt(ADFFILE,newADFFILE,SHIFT_DEV1,SHIFT_DEV2,SHIFT_DEV3,HDR);
  if any(DO_PLOT),
    [fp3,fr3,fe3] = fileparts(newADFFILE);
    figfile = fullfile(fp3,[fr3 '.fig']);
    saveas(hfig,figfile);
  end
end


if any(VERBOSE),  fprintf(' done.\n');  end

return


% ======================================================
function IS_OK = sub_check_header(HDR)
IS_OK = 0;
% nobs must be 1
if HDR.nobs ~= 1,  
  fprintf(' nobs(%d) must be 1. ',HDR.nobs);
  return;
end
% check the last channel as DIO
if HDR.devices(end) ~= 0 || HDR.channels(end) ~= -1,  return;  end
% there should be dev2/dev3
if ~any(HDR.devices == 1) || ~any(HDR.devices == 2), return;  end

IS_OK = 1;
return

% ======================================================
function sub_write_header(OLDADF,NEWADF,HDR)
% just read and write the header part...
fid = fopen(OLDADF,'rb');
tmphdr = fread(fid,HDR.offset2data,'int8=>int8');
fclose(fid);

fid = fopen(NEWADF,'wb');
fwrite(fid,tmphdr,'int8');
fclose(fid);
return


% ======================================================
function sub_write_infotxt(ADFFILE,newADFFILE,SHIFT_DEV1,SHIFT_DEV2,SHIFT_DEV3,HDR)
[fp,fr,fe] = fileparts(newADFFILE);
txtfile = fullfile(fp,sprintf('%s.txt',fr));
fid = fopen(txtfile,'wt');
fprintf(fid,'date:       %s\n',datestr(now));
fprintf(fid,'program:    %s\n',mfilename);
fprintf(fid,'platform:   MATLAB %s\n',version());

fprintf(fid,'[input]\n');
fprintf(fid,'filename:   %s\n',ADFFILE);

fprintf(fid,'[process]\n');
fprintf(fid,'dev1shift:  %d\n',SHIFT_DEV1);
fprintf(fid,'dev2shift:  %d\n',SHIFT_DEV2);
fprintf(fid,'dev3shift:  %d\n',SHIFT_DEV3);

fprintf(fid,'[output]\n');
fprintf(fid,'filename:   %s\n',newADFFILE);

fclose(fid);

return


% ======================================================
function [nshifts_12, nshifts_13] = sub_check_shifts(ADFFILE,HDR,chanoffs,dgz,dev1chans,dev2chans,dev3chans,USE_ENDING_PERIOD,nread)

if ~any(nread),
  nread = round(HDR.numconv*7);  % longer period to avoid "biased" xcorr.
  nread = round(10000/(HDR.us_per_sample/1000));  % 10sec
  if any(USE_ENDING_PERIOD),
    nread = round(HDR.numconv*0.75);
  end
  if any(strfind(ADFFILE,'20170117_test_00')),
    nread = round(HDR.obscounts/2);
  end
end


nch =  HDR.nchannels_ai;
DAT = zeros(nread,nch);

fprintf(' read');
iObs = 1;
fid = fopen(ADFFILE,'rb');
for iCh = 1:nch,
  % get the precision
  switch lower(HDR.data_type(iCh))
   case 'c'
    tmpprec = 'uint8';
    tmpbytes = 1;
   case 's'
    tmpprec = 'int16';
    tmpbytes = 2;
   case 'i'
    tmpprec = 'int32';
    tmpbytes = 4;
   case 'l'
    tmpprec = 'int64';
    tmpbytes = 8;
  end
  fprintf('.');
    
  % read the waveform
  tmpoffs = HDR.offset2data + chanoffs(iCh)*HDR.obscounts(iObs);
  if USE_ENDING_PERIOD,
    tmpoffs = tmpoffs + tmpbytes*(HDR.obscounts(iObs)-nread);
  end
  try
    fseek(fid,tmpoffs,'bof');
    %tmpwv = fread(fid,HDR.obscounts(iObs),tmpprec);
    tmpwv = fread(fid,nread,tmpprec);
  catch
    fclose(fid);
    keyboard
  end
  
  DAT(:,iCh) = tmpwv(:);
end


devices = HDR.dev_numbers(HDR.devices(1:nch)+1);
if isempty(dev1chans),
  dev1chans = find(devices == 1);
  % try to select correlated ones
  if length(dev1chans) > 2,
    cc1 = corrcoef(DAT(:,dev1chans));
    cc1(cc1 < 0 | cc1 == 1) = NaN;
    mcc1 = nanmean(cc1);
    if sum(mcc1 > 0.5) > 2,
      dev1chans = dev1chans(mcc1 > 0.5);
    end
  end
end
if isempty(dev2chans),
  dev2chans = find(devices == 2);
  % try to select correlated ones
  if length(dev2chans) > 2,
    cc2 = corrcoef(DAT(:,dev2chans));
    cc2(cc2 < 0 | cc2 == 1) = NaN;
    mcc2 = nanmean(cc2);
    if sum(mcc2 > 0.5) > 2,
      dev2chans = dev2chans(mcc2 > 0.5);
    end
  end
end
if isempty(dev3chans),
  dev3chans = find(devices == 3);
  % try to select correlated ones
  if length(dev3chans) > 2,
    cc3 = corrcoef(DAT(:,dev3chans));
    cc3(cc3 < 0 | cc3 == 1) = NaN;
    mcc3 = nanmean(cc3);
    if sum(mcc3 > 0.5) > 2,
      dev3chans = dev3chans(mcc3 > 0.5);
    end
  end
end

nlags = round(HDR.numconv*0.5);

% % % testing: USE 'unbiased'!
% x = rand(7500,1);  %x(1000:2000) = x(1000:2000)+10;
% [cc,lags] = xcorr(x,circshift(x,-2000),nlags,'coef');
% [cu,lags] = xcorr(x,circshift(x,-2000),nlags,'unbiased');
% figure;
% plot(lags,[cc(:),cu(:)]);
% legend('xcorr(coef)','xcorr(unbiased)');


% try to select correlated dev1chans: not consistent...
% tmpc = corrcoef(DAT(:,dev1chans));
% tmpc(sub2ind(size(tmpc),1:size(tmpc,1),1:size(tmpc,1))) = NaN;
% tmpm = nanmean(tmpc,2);
% dev1chans = dev1chans(tmpm > 0.4);


% DAT0 = DAT;
% % enhance contrasts
% for K = 1:size(DAT,2),
%   tmpw = DAT(:,K);
%   tmpm = nanmean(tmpw);
%   tmps = nanstd(tmpw);
%   tmpw(tmpw > tmpm+tmps) =  32760;
%   tmpw(tmpw < tmpm-tmps) = -32760;
%   DAT(:,K) = tmpw(:);
% end

%DAT = abs(DAT);

fprintf(' check.');

[fp,fr,fe] = fileparts(ADFFILE);

% compute Dev1->Dev2
if any(dev1chans) && any(dev2chans),
  XC2 = zeros(2*nlags+1,length(dev2chans),length(dev1chans));
  for iCh2 = 1:length(dev2chans),
    for iCh1 = 1:length(dev1chans),
      [tmpc,lags] = xcorr(DAT(:,dev1chans(iCh1)),DAT(:,dev2chans(iCh2)),nlags,'coef');
      %[tmpc,lags] = xcorr(DAT(:,dev1chans(iCh1)),DAT(:,dev2chans(iCh2)),nlags,'unbiased');
      XC2(:,iCh2,iCh1) = tmpc(:);
    end
  end
  % for K = -nlags:nlags,
  %   DAT2 = circshift(DAT,K);
  %   %for iCh2 = 1:length(dev2chans),
  %   for iCh2 = 1:2,
  %     for iCh1 = 1:length(dev1chans),
  %       tmpr = corrcoef(DAT(:,dev1chans(iCh1)),DAT2(:,dev2chans(iCh2)));
  %       XC2(K+nlags+1,iCh2,iCh1) = tmpr(1,2);
  %     end
  %   end
  % end

  tmptitle = sprintf('%s: Dev1--Dev2',[fr,fe]);
  [nshifts_12] = sub_plot_figure(tmptitle,ADFFILE,dgz,HDR,DAT,XC2,lags,dev1chans,dev2chans);
end



% compute Dev1->Dev3
if any(dev1chans) && any(dev3chans),
  XC3 = zeros(2*nlags+1,length(dev3chans),length(dev1chans));
  for iCh2 = 1:length(dev3chans),
    for iCh1 = 1:length(dev1chans),
      [tmpc,lags] = xcorr(DAT(:,dev1chans(iCh1)),DAT(:,dev3chans(iCh2)),nlags,'coef');
      %[tmpc,lags] = xcorr(DAT(:,dev1chans(iCh1)),DAT(:,dev3chans(iCh2)),nlags,'unbiased');
      XC3(:,iCh2,iCh1) = tmpc(:);
    end
  end

  tmptitle = sprintf('%s: Dev1--Dev3',[fr,fe]);
  [nshifts_13] = sub_plot_figure(tmptitle,ADFFILE,dgz,HDR,DAT,XC3,lags,dev1chans,dev3chans);
end

return


% ======================================================
function [nshifts] = sub_plot_figure(txttitle,ADFFILE,dgz,HDR,DAT,XC,lags,dev1chans,devXchans)

txttitle = strrep(txttitle,'_','\_');

hfig = figure('Name',txttitle);
colors = lines;

hax1 = subplot(3,1,1);
% for K = 1:size(XC,2),
%   plot(lags,squeeze(XC(:,K,:)),'color',colors(K,:));
%   hold on;
% end
% this is not reliable...
% meanc = nanmean(reshape(XC,[size(XC,1) size(XC,2)*size(XC,3)]),2);
% medxc = nanmedian(reshape(XC,[size(XC,1) size(XC,2)*size(XC,3)]),2);
% maxxc = max(reshape(XC,[size(XC,1) size(XC,2)*size(XC,3)]),[],2);
% [maxv,maxi] = max(maxxc);
% plot(lags,meanc,'color','k','linewidth',2);
% plot(lags,maxxc,'color','r','linewidth',2);

%dev1chans(1) = 6;

% chose the best for each
XCbest = zeros(size(XC,1),size(XC,2));
for K = 1:size(XC,2),
  tmpxc = squeeze(XC(:,K,:));
  tmpmx = max(tmpxc,[],1);
  [tmpmx,tmpmi] = max(tmpmx);
  plot(lags,tmpxc(:,tmpmi),'color',colors(K,:));
  hold on;
  XCbest(:,K) = tmpxc(:,tmpmi);
end
meanXCbest = nanmean(XCbest,2);
mediXCbest = nanmedian(XCbest,2);
maxXCbest  = max(XCbest,[],2);
%plot(lags,meanXCbest,'color','k','linewidth',2);
%plot(lags,maxXCbest,'color','r','linewidth',2);

curxc = maxXCbest;  % use maxXCbest
% curxc = meanXCbest;  % use meanXCbest


nshifts = NaN(1,2);
maxval  = NaN(1,2);
% neg shifts
tmpsel = find(lags < 0);
tmplags = lags(tmpsel);
[maxv,maxi] = max(curxc(tmpsel));
nshifts(1) = tmplags(maxi);
maxval(1)  = maxv;
% pos shifts
tmpsel = find(lags >= 0);
tmplags = lags(tmpsel);
[maxv,maxi] = max(curxc(tmpsel));
nshifts(2) = tmplags(maxi);
maxval(2)  = maxv;


grid on;
xlabel(sprintf('Lags (1000pts=%gms)',HDR.us_per_sample));
ylabel('corr. coef.');
title(txttitle);
line([nshifts(1) nshifts(1)],get(gca,'ylim'),'color','r');
line([nshifts(2) nshifts(2)],get(gca,'ylim'),'color','r');
text(nshifts(1)-25,max(get(gca,'ylim'))-0.10,sprintf('NegMax(%.3f at %d)',maxval(1),nshifts(1)),'horizontalalignment','right');
text(nshifts(2)+25,max(get(gca,'ylim'))-0.10,sprintf('PosMax(%.3f at %d)',maxval(2),nshifts(2)));


nlen = round(HDR.numconv*0.5);

for K = 1:2,
  hax = subplot(3,1,K+1);
  
  if K == 1,
    % read data
    tmpdat1 = adf_read(ADFFILE,0,dev1chans(1)-1);  tmpdat1 = tmpdat1(:);
    tmpdatX = adf_read(ADFFILE,0,devXchans(1)-1);  tmpdatX = tmpdatX(:);
    tmpt = 1:length(tmpdat1);
    % shift data
    tmpnegX = circshift(tmpdatX,nshifts(1));
    tmpposX = circshift(tmpdatX,nshifts(2));
    % negative shift = rightward
    tmpnegX(end+nshifts(1)+1:end) = NaN;  % to avoid misunderstanding...
    % positive shift = leftward
    tmpposX(1:nshifts(2)) = NaN;  % to avoid misunderstanding...
    % beginning period
    tmpsel  = 1:nlen;
  elseif K == 2,
    % ending period
    tmpsel  = (-nlen-1:0)+length(tmpdat1);
  end
  plot(tmpt(tmpsel),tmpdat1(tmpsel),'color',[0.1 0.1 0.8]);  hold on;
  plot(tmpt(tmpsel),tmpdatX(tmpsel),'color',[0.2 0.2 0.2]);
  plot(tmpt(tmpsel),tmpposX(tmpsel),'color',[0.0 0.9 0.0]);
  plot(tmpt(tmpsel),tmpnegX(tmpsel),'color',[0.9 0.0 0.0]); % neg one as the top (most likely)

  devX = HDR.dev_numbers(HDR.devices(devXchans(1))+1);
  legend(sprintf('dev1:Ch%d reference',dev1chans(1)),...
         sprintf('dev%d:Ch%d original',devX,devXchans(1)),...
         sprintf('dev%d:Ch%d shifted [%+d]',devX,devXchans(1),nshifts(2)),...
         sprintf('dev%d:Ch%d shifted [%+d]',devX,devXchans(1),nshifts(1)),...
         'location','NorthEast');
  grid on;
  ylabel('ADC Unit');
  xlabel(sprintf('Time (1000pts=%gms)',HDR.us_per_sample));
  set(gca,'xlim',[tmpt(tmpsel(1)) tmpt(tmpsel(end))]);
  if K == 1,
    title('beginning period');
  else
    title('ending period');
  end

  if ~isempty(dgz),
    xlim = get(hax,'xlim');
    ylim = get(hax,'ylim');
    t_mri_pts = dgz.t_mri_pts;
    t_mri_pts = t_mri_pts(t_mri_pts >= xlim(1) & t_mri_pts <= xlim(2));
    for K = 1:length(t_mri_pts),
      t = t_mri_pts(K);
      line([t t],ylim,'color',[0.1 0.1 0.1]);
    end
    %keyboard
  end
end

return
