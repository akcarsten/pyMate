function checkobslens(froot)
% PURPOSE : To check whether obs-lengths are the same in dgz/adf.
% USAGE :   checkobslens
% NOTES :   If nobs is not the same, draw obs-lengths in figure.
%           Otherwise, prints difference.
%           You may modify 'datapath' below to find datafile.
% VERSION :
global dgzf adff dgzlens adflens
  
dgzpath = 'e:/Data/AnesRiv/dgz';
adfpath = 'e:/Data/AnesRiv/adf';

dgzpath = 'l:/projects/yusuke';
adfpath = 'd:/@shared/cvt';


if nargin == 0,
  f = pickfile('Select a DGZ file',dgzpath,'*.dgz');
  if ~exist(f,'file'), return;  end
  dgzpath = getFileDirectory(f);
  froot = getFileRoot(f);
end

fprintf('FILE: %s\n',froot);

dgzf = sprintf('%s/%s.dgz',dgzpath,froot);
adff = sprintf('%s/%s.adfw',adfpath,froot);
%adff = sprintf('%s/adf/%s.eeg',datapath,froot);
%adff2 = sprintf('%s/adf/%s.adfw.corrected',datapath,froot);


dgzlens = getObsLengths(dgzf)
adflens = getObsLengths(adff)
adflens/1000.
%adflens2 = getObsLengths(adff2);

if length(dgzlens) == length(adflens),
  dlens = dgzlens-adflens
  dlens2 = dlens/dgzlens*100;
  fprintf('nobs=%d: diff(dgz-adf)\n', length(dgzlens));
  fprintf('mean= %.2f msec,   max:%.2f   min %.2f\n',...
	  mean(dlens(:)),max(dlens(:)),min(dlens(:)));
  fprintf('mean= %.4f %%,    max:%.4f min %.4f\n',...
	  mean(dlens2(:)),max(dlens2(:)),min(dlens2(:)));
  
  %[dgzlens-adflens, dgzlens-adflens2]
  %[dgzlens, adflens, dgzlens-adflens, (dgzlens-adflens)./dgzlens.*100];

else
  tmptxt = sprintf('FILE: %s',froot);
  figure('Name',tmptxt);
  plot(dgzlens,'color','blue'); hold on;
  plot(adflens,'color','red');
  legend('by DGZ','by ADF/ADFW');
  title(strrep(tmptxt,'_','\_'));
  xlabel('observation');
  ylabel('obs-lengths in msec');
  grid on;
end

