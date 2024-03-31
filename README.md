![pymates doing neuro science](/images/pyMate.png "pymates doing neuro science")
# pyMate: process (primate) fMRI and e-phys data

## What is it?
**pyMate** is a Python package that provides basic functionalities to process functional magnetic resonance imaging (fMRI)
and electro-physiological (e-phys) data.
The tooling has a focus on the analysis of simultaneous fMRI and e-phys recordings
in non-human primates (NHPs) as acquired in the AGLO lab at the MPI for Biological Cybernetics.

**Disclaimer**: Parts of the code and documentation were created with the help of 
[ChatGPT 3.5](https://chat.openai.com/), an AI language model.
Images were generated with [Midjourney](https://www.midjourney.com/home), 
an AI image generator.

## Table of Contents
- [Main Features](#main-features)
- [Background](#background)
- [Data Structures](#data-structures)
- [Data Formats](#data-formats)

## Main Features
- **[Gordo](#gordo)**: Toolkit to process fMRI data. Named after Gordo, a squirrel monkey, who traveled to space in 1958.
- **[Clyde](#clyde)**: Toolkit to process e-phys data. Named after Clyde, a character in Clint Eastwood movies, played by an Orangutan named Manis.
- **[Zaius](#zaius)** Various tools to convert fringe data formats into more common data structures. Named after Dr. Zaius, the minister of science in the Planet of the Apes movies.

## Background

## Data Structures
### Session Files

### fMRI Data

### E-Phys Data

## Data Formats

## Gordo
![Gordo in space](/images/gordo.png "Gordo in space")

### Basics
Gordo mainly provides wrapper functions for the [nilearn](https://nilearn.github.io/stable/index.html) package to 
facilitate the analysis of fMRI data. The module offers a simple, configurable interface to run statistical 
fMRI analysis and visualizations in a flexible yet structured manner.

In its simplest form the only input necessary is the link to a [session file(s)](#session-files) and the subject folder with the raw fMRI 
data. The example below outlines this process:

```python
from pyMate.Gordo import MriProcessing


subject_folder = r'c:\your\folder\structure\clyde_in_the_scanner'

session_files = [
    r'c:\your\folder\structure\session_file_001.csv',
    r'c:\your\folder\structure\session_file_002.csv']

fmri = MriProcessing(session_files, subject_folder)
```

After running the above code an HTML page will open in the systems default browser which gives an interactive view of 
the statistical map on-top the mean EPI image. 

To customize the analysis the following attributes can be set:

| Attribute      | Description | Default Value | Valid Values |
|----------------|-------------|---------------|--------------|
| hrf_model      |             | 'spm'         |              |
| drift_model    |             | 'cosine'      |              |
| high_pass      |             | 0.01          |              |
| noise_model    |             | 'ar1'         |              |
| smoothing_fwhm |             | 3             |              |
| threshold      |             | None          |              |
| mask_img       |             | False         |              |

Extending the example above with some of the attributes:

```python
fmri = MriProcessing()

fmri.smoothing_fwhm = 6
fmri.mask_img = None

fmri.session_files = session_files
fmri.subject_folder = subject_folder

fmri.lets_go()
```

This will run the analysis with a smoothing kernel of 6mm and with a mask image, so that a statistical map is 
restricted to the brain.

### Visualizations

#### Plot the Design Matrix
In fMRI (functional Magnetic Resonance Imaging) analysis, the design matrix is a fundamental component used in 
statistical modeling to represent the experimental design and its relationship to the observed fMRI data. 
It is a crucial part of the general linear model (GLM) framework, which is commonly used for analyzing fMRI data.
The design matrix essentially encodes the experimental paradigm or task design in a matrix format. Each column of 
the design matrix represents a specific condition or regressor in the experiment, while each row represents a 
time point or sample in the fMRI data. The values in the matrix indicate the expected activity or response of 
each voxel (3D pixel) in the brain for each condition at each time point.

Here's a breakdown of the components of a typical design matrix:
1. **Columns**: Each column corresponds to a different experimental condition or regressor. 
For example, if an experiment involves presenting visual stimuli of faces and houses, there would be two columns 
in the design matrix, one for faces and one for houses.
2. **Rows**: Each row corresponds to a time point or sample in the fMRI data. 
Typically, the fMRI data is divided into small time intervals called time bins or volumes. 
Each row in the design matrix represents the expected neural activity for each condition at each time bin.
3. **Values**: The values in the matrix represent the expected response of each voxel in the brain for each 
condition at each time point. These values are typically derived from a hypothesized model of the 
hemodynamic response function (HRF), which describes the relationship between neural activity and the 
observed fMRI signal.

The design matrix is used in conjunction with the observed fMRI data to perform statistical inference, 
typically through methods such as ordinary least squares (OLS) regression or its variants. 
By fitting the design matrix to the fMRI data, researchers can estimate the contribution of each experimental 
condition to the observed brain activity and infer which brain regions are involved in processing different 
cognitive tasks or stimuli.

```python
fmri.plot_design_matrix()
```

#### Plot the Contrast Matrix
```python
fmri.plot_contrast_matrix()
```
#### Plot the Expected BOLD Response
```python
fmri.plot_expected_response()
```

## Clyde
![Clyde in a bar](/images/clyde.png "Clyde in a bar")
### Basics

### Visualizations

#### Plot LFP time course and power spectrum
```python
from pyMate.Clyde import PrepareData


signal = r'c:\your\folder\structure\ephys_data.h5'

ephys_data = PrepareData(signal_filename=signal)
ephys_data.plot_it()
``` 

#### Plot LFP Time Course and Mark Detected Peaks
```python
from pyMate.Clyde import SignalProcessing


channel = 1
event_data = SignalProcessing(ephys_data)
event_data.wheres_the_party()

event_data.plot_peaks(channel)
```

#### Plot mean Time-Frequency Power Spectrum
```python
event_data.plot_mean_event_tfr(channel)
```

#### Plot Power Spectra of Detected Events
```python
event_data.plot_event_spectra(channel, xlim=[0, 200])
```

#### Plot Cluster Time-Frequency Power Spectrum and LFP traces
```python
event_data.plot_cluster(channel)
```

## Zaius
![Zaius thinking](/images/zaius.png "Zaius thinking")
#### Bruker Data to Nifti format
#### DGZ files to csv
#### ADFX files to HDF5 
#### Matlab files to HDF5