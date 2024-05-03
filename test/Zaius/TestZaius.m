classdef TestZaius < matlab.unittest.TestCase

    methods(Test)

        function testReadMatData(testCase)

            tempDir = tempname;
            mkdir(tempDir);

            % Create a sample .mat file with cleaned electrophysiological data
            matFilename = fullfile(tempDir, 'testfile.mat');
            data = struct('Cln', struct('dx', 0.001, 'dat', rand(10, 10)));
            save(matFilename, '-struct', 'data');

            z = Zaius(matFilename, tempDir);
            z.read_mat();

            verifyEqual(testCase, z.header.sampling_rate, 1 / data.Cln.dx, 'AbsTol', 1e-6);
            verifyEqual(testCase, z.data, data.Cln.dat, 'AbsTol', 1e-6);

            rmdir(tempDir, 's');

        end

        function testGetDgzParametersWithEmptyData(testCase)

            z = Zaius('inputfile.dgz', 'outputfolder');
            z.data.e_times = [];
            z.getDgzParameters();

            verifyEmpty(testCase, z.dgzParameter);

        end

        function testGetDgzParametersWithData(testCase)

            time = [1, 2, 3, 4, 5]' * 1000; % Time in milliseconds
            types = [19, 29, 27, 22, 20]'; % Event types
            names = repmat({''}, 1, 30);
            names(types + 1) = {'StartObs', 'StimulusType', 'Stimulus', 'TrialType', 'EndObs'};
            names = names';

            z = Zaius('inputfile.dgz', 'outputfolder');
            z.data.e_times = {time};
            z.data.e_types = {types};
            z.data.e_names = names;

            z.getDgzParameters();

            verifyNotEmpty(testCase, z.dgzParameter);
            verifyEqual(testCase, z.dgzParameter.time, time);
            verifyEqual(testCase, z.dgzParameter.types, types);
            verifyEqual(testCase, z.dgzParameter.names, names);

        end

        function testGetDgzMetaInfoWithEmptyData(testCase)

            z = Zaius('inputfile.dgz', 'outputfolder');
            z.data.e_pre = {}; % Set e_pre to empty
            z.getDgzMetaInfo();

            verifyEmpty(testCase, z.dgzMetaInfo);

        end

        function testGetDgzMetaInfoWithData(testCase)

            e_pre = {{[], 'param1'}, {[], 'value1'}, {[], 'param2'}, {[], []}};

            z = Zaius('inputfile.dgz', 'outputfolder');
            z.data.e_pre = e_pre;
            z.getDgzMetaInfo();

            verifyNotEmpty(testCase, z.dgzMetaInfo);
            verifyEqual(testCase, z.dgzMetaInfo.parameter, {'param1'; 'param2'});
            verifyEqual(testCase, z.dgzMetaInfo.value, {'value1'; 'empty'});

        end

        function testGetDgzEventInfoWithEmptyParameters(testCase)

            z = Zaius('inputfile.dgz', 'outputfolder');
            z.dgzParameter = struct('stim_times', [], 'stim_duration', []);
            z.getDgzEventInfo();

            verifyEmpty(testCase, z.dgzEventInfo);

        end

        function testGetDgzEventInfoWithData(testCase)

            stim_times = [0.5, 1.5, 2.5, 3.5]';
            stim_duration = [0.1, 0.2, 0.1, 0.2]';

            z = Zaius('inputfile.dgz', 'outputfolder');
            z.dgzParameter = struct('stim_times', stim_times, 'stim_duration', stim_duration);
            z.getDgzEventInfo();

            verifyNotEmpty(testCase, z.dgzEventInfo);
            verifyEqual(testCase, z.dgzEventInfo.Properties.VariableNames, {'onset', 'duration', 'trial_type'});
            verifySize(testCase, z.dgzEventInfo, [length(stim_times), 3]);
            verifyEqual(testCase, z.dgzEventInfo.onset, stim_times);
            verifyEqual(testCase, z.dgzEventInfo.duration, stim_duration);
            verifyEqual(testCase, z.dgzEventInfo.trial_type, ["rest", "active", "rest", "active"]');

        end

        function testMakeOutputFilenameWithoutSuffix(testCase)

            z = Zaius('inputfile.mat', 'outputfolder');
            z.makeOutputFilename('.h5');

            expectedFilename = fullfile(z.targetFolder, 'inputfile.h5');
            verifyEqual(testCase, z.targetFilename, expectedFilename);

        end

        function testMakeOutputFilenameWithSuffix(testCase)

            z = Zaius('inputfile.mat', 'outputfolder');
            z.makeOutputFilename('.h5', 'suffix', '_processed');

            expectedFilename = fullfile(z.targetFolder, 'inputfile_processed.h5');
            verifyEqual(testCase, z.targetFilename, expectedFilename);

        end

        function testMakeOutputFilenameWithSuffixAndNonDotExtension(testCase)

            z = Zaius('inputfile.mat', 'outputfolder');
            z.makeOutputFilename('h5', 'suffix', '_processed');

            expectedFilename = fullfile(z.targetFolder, 'inputfile_processed.h5');
            verifyEqual(testCase, z.targetFilename, expectedFilename);
        end

        function testSaveH5Overwrite(testCase)

            tempDir = tempname;

            z = Zaius('inputfile.mat', tempDir);
            z.data = rand(10, 5);
            z.header = struct('sampling_rate', 1000);
            z.targetFilename = fullfile(z.targetFolder, 'test.h5');
            z.saveH5();

            verifyTrue(testCase, exist(z.targetFilename, 'file') == 2);
            rmdir(tempDir, 's');

        end

        function testSaveH5NoOverwrite(testCase)

            tempDir = tempname;
            mkdir(tempDir);

            z = Zaius('inputfile.mat', tempDir);
            z.data = rand(10, 5);
            z.header = struct('sampling_rate', 1000);
            z.targetFilename = fullfile(z.targetFolder, 'test.h5');

            h5create(z.targetFilename, '/data', size(z.data), 'Datatype', 'double');

            z.overwrite = false;
            z.saveH5();

            verifyTrue(testCase, exist(z.targetFilename, 'file') == 2);
            assertWarningFree(testCase, @()z.saveH5());

            rmdir(tempDir, 's');
        end

        function testSaveCsvWithVariableNames(testCase)

            tempDir = tempname;
            mkdir(tempDir);

            data = table(rand(5), 'VariableNames', {'Column1'});

            z = Zaius('inputfile.mat', tempDir);
            z.targetFilename = fullfile(z.targetFolder, 'test.csv');
            z.saveCsv(data);

            verifyTrue(testCase, exist(z.targetFilename, 'file') == 2);
            rmdir(tempDir, 's')

        end

        function testSaveCsvWithoutVariableNames(testCase)

            tempDir = tempname;
            mkdir(tempDir);

            data = table(rand(5), 'VariableNames', {'Column1'});

            z = Zaius('inputfile.mat', tempDir);
            z.targetFilename = fullfile(z.targetFolder, 'test.csv');
            z.saveCsv(data, 'WriteVariableNames', false);

            verifyTrue(testCase, exist(z.targetFilename, 'file') == 2);
            rmdir(tempDir, 's')

        end

        function testVerifyResultConversionSuccessful(testCase)

            tempDir = tempname;
            mkdir(tempDir);

            z = Zaius('inputfile.mat', tempDir);
            z.data = rand(10, 3);
            z.targetFilename = fullfile(z.targetFolder, 'test.h5');
            h5create(z.targetFilename, '/data', size(z.data), 'Datatype', 'double');
            h5write(z.targetFilename, '/data', z.data);

            z.verifyResult;

            rmdir(tempDir, 's')

        end

        function testVerifyResultConversionFailed(testCase)

            tempDir = tempname;
            mkdir(tempDir);

            z = Zaius('inputfile.mat', tempDir);
            z.data = rand(10, 3);
            z.targetFilename = fullfile(z.targetFolder, 'test.h5');

            h5create(z.targetFilename, '/data', size(z.data), 'Datatype', 'double');
            h5write(z.targetFilename, '/data', rand(10, 3));

            try
                z.verifyResult;
            catch ME
                assertEqual(testCase, ME.message, 'Conversion failed. Input does not match output.');
            end

            rmdir(tempDir, 's')

        end

    end
    
end
