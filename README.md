![pymates doing neuro science](/images/pyMate.png "pymates doing neuro science")
# pyMate: process (primate) fMRI and e-phys data

## What is it?
**pyMate** is a Python package that provides basic functionalities to process functional magnetic resonance imaging (fMRI)
and electro-physiological (e-phys) data.
The tooling has a focus on the analysis of simultaneous fMRI and e-phys recordings
in non-human primates (NHPs) as acquired in the AGLO lab at the MPI for Biological Cybernetics.

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

To customize the analysis the following attributes ca be set:

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

This will run the analysis with a smoothing kernel of 6mm and without a mask image.

### Visualizations

## Clyde
![Clyde in a bar](/images/clyde.png "Clyde in a bar")

## Zaius
![Zaius thinking](/images/zaius.png "Zaius thinking")