folderpath = 'D:\Backup_recordings\K07\K07.Ef1\';
              
file_list = dir([folderpath, '*.adfx']);

for i = 1:length(file_list)

    filename = fullfile(file_list(i).folder, file_list(i).name);
    disp(filename)
    conv_adf(filename);

end


file_list = dir([folderpath, '*.dgz']);

for i = 1:length(file_list)

    filename = fullfile(file_list(i).folder, file_list(i).name);
    disp(filename)
    conv_dgz(filename)

end


filename = 'C:\Users\carst\Google Drive\05_Projects\NET_fMRI\data\processed\K07.Ef1\cln\k07ef1_0001_cln.mat';

conv_mat(filename)