function check_pvphys(varargin)
%CHECK_PVPHYS - check PV triggers and recorded gradients by plotting begin/end.
%  CHECK_PVPHYS(SES)
%  CHECK_PVPHYS(SES,EXP) 
%  CHECK_PVPHYS(ADFFILE,GRADCH) 
%  CHECK_PVPHYS(DGZFILE,GRADCH)
%  CHECK_PVPHYS(DGZFILE,ADFFILE,GRADCH) checks ParaVision triggers and the recorded 
%  signal by plotting the grad. waveform at the beginning and ending period.  
%  Note that "GRADCH" can be any channel which shows chunks of MR acquisition
%  (e.g. true grad. signal or electrode signal with interferance noises).
%
%  NOTE :
%    Sampling time of ADF for plotting may differ from the original one, after matching 
%    lengths of ADF and DGZ.  e.g. The length of ADF is corrected into that of DGZ.
%
%  EXAMPLE :
%    >> check_pvphys('F12m01');     % for all exps
%    >> check_pvphys('F12m01',13);  % for exp13 alone
%    >> check_pvphys('test_001.adfx',6); % specified file/channel
%    >> check_pvphys('aaaa.adf','bbb.dgz',10)  % specified adf/dgz/channel
%
%  VERSION :
%    0.90 xx.10.16 YM  pre-release
%    0.91 28.10.16 YM  renamed as check_pvphys().
%    0.92 20.01.16 YM  accepts a filename of DGZ or ADF.
%
%  See also check_adfx_mrevent fix_adfx_shift

if nargin == 0,  eval(['help ' mfilename]); return;  end


if is_rawfile(varargin{1}),
  if nargin < 2,
    error('%: missing argument(s), %s(fname,gradch) or %s(dgz,adf,gradch).\n',mfilename,mfilename,mfilename);
  end
  if nargin == 2,
    % check_pvphys(fname,gradch)
    [fp,fr,fe] = fileparts(varargin{1});
    if any(strfind(fe,'.adf')),
      adffile = fullfile(fp,[fr,fe]);
      dgzfile = fullfile(fp,[fr,'.dgz']);
    else
      dgzfile = fullfile(fp,[fr,fe]);
      tmpdir = dir(fullfile(fp,[fr '.adf*']));
      tmpdir = tmpdir([tmpdir.isdir] == 0);
      if length(tmpdir) == 0,
        error(' %s: no .adf* found.',mfilename);
      end
      adffile = fullfile(fp,tmpdir.name);
    end
    gradch = varargin{2};
  else
    % check_pvphys(dgz,adf,gradch)
    [fp,fr,fe] = fileparts(varargin{1});
    if any(strcmpi(fe,'.dgz')),
      dgzfile = varargin{1};
      adffile = varargin{2};
    else
      adffile = varargin{1};
      dgzfile = varargin{2};
    end
    gradch = varargin{3};
  end
  [fp,fr,fe] = fileparts(adffile);
  [fp2,fr2,fe2] = fileparts(dgzfile);
  txttitle = sprintf('%s/%s',[fr fe],[fr2 fe2]);
  sub_check_pvphys(adffile,dgzfile,gradch,txttitle);
else
  ses = getses(varargin{1});
  if nargin < 2,
    exps = getexps(ses);
  else
    exps = varargin{2};
  end
  for iExp = 1:length(exps),
    if isimaging(ses,exps(iExp)) && isrecording(ses,exps(iExp)),
      ExpNo = exps(iExp);
      grp = getgrp(ses,ExpNo);
      adffile = expfilename(ses,ExpNo,'adf');
      dgzfile = expfilename(ses,ExpNo,'dgz');
      gradch = grp.gradch;
      [fp,fr,fe] = fileparts(adffile);
      txttitle = sprintf('%s exp=%d: %s/dgz',ses.name,ExpNo,[fr fe]);
      sub_check_pvphys(adffile,dgzfile,gradch,txttitle);
    end
  end
end
return


% ===============================================================
function flag = is_rawfile(arg)
flag = 0;
if ~ischar(arg),  return;  end
[fp,fr,fe] = fileparts(arg);

if any(strcmpi(fe,'.dgz')),  flag = 1;  end
if any(strfind(fe,'.adf')),  flag = 1;  end

return




% ===============================================================
function sub_check_pvphys(ADFFILE,DGZFILE,gradch,txttitle)
if ~exist(ADFFILE,'file')
  error(' %s:  file not found: %s.',mfilename,ADFFILE);
end
if ~exist(DGZFILE,'file'),
  error(' %s:  file not found: %s.',mfilename,DGZFILE);
end


[wgrad,npts,sampt] = adf_read(ADFFILE,0,gradch-1);
dg = dg_read(DGZFILE);

E_BeginObsp= 19;
E_EndObsp  = 20;
E_Mri      = 46;

t_BeginObsp = dg.e_times{1}(dg.e_types{1}==E_BeginObsp);
t_EndObsp   = dg.e_times{1}(dg.e_types{1}==E_EndObsp);
t_Mri       = dg.e_times{1}(dg.e_types{1}==E_Mri);


t_Mri = t_Mri - t_BeginObsp;

tfac=t_EndObsp/(length(wgrad)*sampt);

TWIN = 250;

figure('Name',txttitle);
tmpt = [0:length(wgrad)-1]*sampt*tfac;
% beginning...
subplot(2,1,1);
tmpsel = (tmpt <= TWIN);
plot(tmpt(tmpsel), wgrad(tmpsel));
hold on;
tmpmri = t_Mri(t_Mri <= TWIN);
for K=1:length(tmpmri),
  line([tmpmri(K) tmpmri(K)],[-8000 8000],'color','r');
end
set(gca,'xlim',[0  TWIN]);
set(gca,'ylim',[-8000 8000]);
title(strrep(txttitle,'_','\_'));
ylabel('ADC Unit');
xlabel('Time (ms)');
% end...
subplot(2,1,2);
tmpsel = (tmpt >= tmpt(end)-TWIN);
plot(tmpt(tmpsel), wgrad(tmpsel));
hold on;
tmpmri = t_Mri(t_Mri >= tmpt(end)-TWIN);
for K=1:length(tmpmri),
  line([tmpmri(K) tmpmri(K)],[-8000 8000],'color','r');
end
set(gca,'xlim',[tmpt(end)-TWIN tmpt(end)]);
set(gca,'ylim',[-8000 8000]);
title(strrep(txttitle,'_','\_'));
ylabel('ADC Unit');
xlabel('Time (ms)');



% if any(READ_TRIG),
%   plot(tmpt, [wtrig(:),wgrad(:)]);
%   legend('trig','grad');
% else
%   plot(tmpt, wgrad);
% end
% hold on;
% for K=1:length(t_Mri),
%   line([t_Mri(K) t_Mri(K)],[-8000 8000],'color','r');
% end
% set(gca,'xlim',[T0  tmpt(end)]);
% set(gca,'ylim',[-8000 8000]);
% title(strrep(DATAFILE,'_','\_'));
% ylabel('ADC Unit');
% xlabel('Time (ms)');
