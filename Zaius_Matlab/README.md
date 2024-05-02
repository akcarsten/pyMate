# Zaius Class

The `Zaius` class is a MATLAB class designed for handling data from the AGLO lab. It provides methods for reading, converting, and saving data from AGLO data files, as well as verifying the conversion results.

## Features

- Reads data from both ADFX files (.adfx) and MAT files (.mat).
- Converts data to HDF5 format (.h5) for easier storage and manipulation.
- Verifies the result of the conversion process to ensure accuracy.
- Supports customization of target folder, filename, and data type.

## Installation

1. Clone or download the `Zaius` folder from this repository.
2. Add the folder and its sub-folders to your MATLAB project directory.

## Example Usage

```matlab
targetFolder = 'C:\data\converted';

adfxFile = 'C:\data\CM033zJ1_071.adfx';
z = Zaius(adfxFile, targetFolder, 'dataType', 'int16');
z.runConversion


matFile = 'C:\data\k07ef1_0001_cln.mat';
z = Zaius(matFile, targetFolder, 'dataType', 'double');
z.runConversion

dgzFile = 'C:\data\CM033zJ1_071.dgz';
z = Zaius(dgzFile, targetFolder);
z.runConversion
