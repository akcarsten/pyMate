function [] = conv_adf(filename)
% Converts ADFX file to HDF5 format and verifies the conversion.
%
% Parameters:
%   - filename: A string containing the path to the input ADFX file.

[data, header] = read_adfx(filename);

h5_filename = construct_output_filename(filename);
save_as_h5(data, header, h5_filename);

verify_result(h5_filename, data);
end

function [] = verify_result(h5_filename, data)
% Verifies the correctness of the conversion by comparing the saved HDF5 file with the original data.
%
% Parameters:
%   - h5_filename: A string containing the path to the saved HDF5 file.
%   - data: The original data read from the ADFX file.

h5 = h5read(h5_filename, '/data');

if ~all(all(h5 == data))
    error("conversion failed. Input does not match output")
end
end

function [h5_filename] = construct_output_filename(filename)
% Constructs the output HDF5 filename based on the input ADFX filename.
%
% Parameters:
%   - filename: A string containing the path to the input ADFX file.
%
% Returns:
%   - h5_filename: A string containing the path to the output HDF5 file.

current_path = split(pwd, '\');
data_path= split(filename, '\');

h5_path = fullfile(current_path{1:end-2}, data_path{end-1});

if ~exist(h5_path, 'dir')
    mkdir(h5_path)
end

h5_filename = fullfile(h5_path, [data_path{end}(1:end-4), 'h5']);
end

function [data, header] = read_adfx(filename)
% Reads the header and data from an ADFX file.
%
% Parameters:
%   - filename: A string containing the path to the input ADFX file.
%
% Returns:
%   - data: A matrix containing the data read from the ADFX file.
%   - header: A structure containing the header information.

header = adf_readHeader(filename);

if header.nobs ~= 0

    n_channels = header.nchannels_ai;
    n_samples = header.obscounts;

    data = zeros(n_samples, n_channels);
    for i_channel = 1:n_channels
        data(:, i_channel) = adf_read(filename, 0 , i_channel - 1);
    end
else
    data = zeros(1, 1);
end

end

function [] = save_as_h5(data, header, filename)
% Saves data and header as an HDF5 file.
%
% Parameters:
%   - data: A matrix containing the data to be saved.
%   - header: A structure containing header information.
%   - filename: A string containing the path to the output HDF5 file.

h5create(filename, '/data', size(data), 'Datatype','int16')
h5write(filename,'/data', data)

parameters = fieldnames(header);

for i_par = 1 : length(parameters)

    parameter_name = parameters{i_par};
    h5writeatt(filename,'/', parameter_name, header.(parameter_name));

end
end

