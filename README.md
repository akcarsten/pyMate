![pymates doing neuro science](/images/pyMate.png "pymates doing neuro science")
# pyMate: process (primate) fMRI and e-phys data

## What is it?
**pyMate** is a Python package that provides basic functionalities to process 
functional magnetic resonance imaging (fMRI) and electro-physiological (e-phys) data.
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
  - [fMRI Data](#fMRI-Data)
  - [E-Phys Data](#e-phys-data)
  - [Session Files](#session-files)
- [Data Formats](#data-formats)

- [Gordo](#[gordo])
  - [Basics](#basics)
  - [Concepts & Visualizations](#concepts-&-visualizations)
    - [The General Linear Model (GLM)](#the-general-linear-model-(glm))
    - [The Design Matrix](#the-design-matrix)
    - [The Contrast Matrix](#the-contrast-matrix)
    - [The Expected BOLD Response for a single voxel](#the-expected-bold-response-for-a-single-voxel)
- [Clyde](#clyde)
  - [Basics](#basics)
  - [Visualizations](#visualizations)
    - [Plot LFP time course and power spectrum](#plot-lfp-time-course-and-power-spectrum)
    - [Plot LFP Time Course and Mark Detected Peaks](#plot-lfp-time-course-and-mark-detected-peaks)
    - [Plot mean Time-Frequency Power Spectrum](#plot-mean-time-frequency-power-spectrum)
    - [Plot Power Spectra of Detected Events](#plot-power-spectra-of-detected-events)
    - [Plot Cluster Time-Frequency Power Spectrum and LFP traces](#plot-cluster-time-frequency-power-spectrum-and-lfp-traces)
- [Zaius](#zaius)
  - [Bruker Data to Nifti format](#bruker-data-to-nifti-format)
  - [DGZ files to csv](#dgz-files-to-csv)
  - [ADFX files to HDF5](#adfx-files-to-hdf5)
  - [Matlab files to HDF5](#matlab-files-to-hdf5)

## Main Features
- **[Gordo](#gordo)**: Toolkit to process fMRI data. Named after Gordo, a squirrel monkey, who traveled to space in 1958.
- **[Clyde](#clyde)**: Toolkit to process e-phys data. Named after Clyde, a character in Clint Eastwood movies, played 
by an Orangutan named Manis.
- **[Zaius](#zaius)** Various tools to convert fringe data formats into more common data structures. Named after 
Dr. Zaius, the minister of science in the Planet of the Apes movies.

## Background
The pyMate package was developed to facilitate the analysis of fMRI and e-phys data recorded from non-human primates.
The package consists of three main modules: [Gordo](#gordo), [Clyde](#clyde), and [Zaius](#zaius). Each module provides 
tools to process and analyze different aspects of the simultaneously recorded fMRI and e-phys data. While this is the 
original purpose of the package, the tools can also be used separately to only process fMRI or e-phys data.

The [Zaius](#zaius) module however is not designed for data analysis but is a collection of tools to convert between data formats.
Due to the heterogeneity of data formats not all the code in the [Zaius](#zaius) module is written in Python. Re-write it in 
Python if you feel like it and push it here ;)

## Data Formats
The pyMate package relies on various data formats for fMRI and e-phys data. The main data formats are:
- **fMRI Data**: Nifti format
- **E-Phys Data**: HDF5 format
- **Session Files**: CSV format
In the following the data formats and how to structure them is explained in more detail.

## Data Structures
![Data Structures](/images/data_structures.png "Data Structures")
Handling the various data formats for the fMRI and e-phys data is challenging. The pyMate package relies on a structure
that may not be optimal... but it works. **Ideas for different concepts are appreciated**.

### fMRI Data
The fMRI data is the easiest data to handle as it adheres to the Nifti format. The main challenge might be to convert 
the data from the Bruker format to Nifti. The [Zaius](#zaius) module provides tools to do this. 
While it is not important where or how the Nifti files are stored it is recommended to keep a folder structure where
each scan is located in its owb sub-folder of the subject folder.

### E-Phys Data

### Session Files

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

### Concepts & Visualizations
The analysis of the fMRI data that is followed by the Gordo code is based on the General Linear Model (GLM). A typical 
approach to analyzing fMRI data. In the following the most important terms are explained and commands to visualize key 
aspects with the Gordo framework are shown.

#### The General Linear Model (GLM)
The General Linear Model (GLM) is a statistical framework widely used in fMRI (functional Magnetic Resonance Imaging) 
analysis to study the relationship between experimental stimuli or tasks and brain activity. It provides a flexible 
and powerful approach for analyzing fMRI data by modeling the observed BOLD (Blood Oxygen Level Dependent) signal 
as a linear combination of explanatory variables.

Here's an explanation of the components and principles of the GLM in the context of fMRI analysis:

1. **Design Matrix**: At the core of the GLM is the design matrix, also known as the design matrix or model matrix. 
The design matrix encodes the experimental design and represents the relationship between the experimental conditions 
or stimuli and the observed fMRI data. Each column of the design matrix corresponds to a different explanatory variable,
typically representing experimental conditions, and each row corresponds to a different time point or sample in the 
fMRI data. The values in the design matrix indicate the expected response of each voxel (3D pixel) in the brain for 
each condition at each time point.
2. **Parameter Estimation**: The GLM estimates parameters that quantify the relationship between the explanatory 
variables in the design matrix and the observed fMRI data. This is typically done using methods such as ordinary 
least squares (OLS) regression or its variants. The goal is to find the set of parameter estimates that best explains 
the variability in the fMRI data given the design matrix.
3. **Contrast Specification**: Once the parameters are estimated, researchers can specify contrasts of interest 
within the GLM framework. Contrasts are linear combinations of the parameter estimates that test specific hypotheses 
about the effects of experimental conditions on brain activity. For example, a contrast might compare brain activity 
during one experimental condition to another or isolate the effects of interest relative to a baseline or 
control condition.
4. **Statistical Inference**: After specifying contrasts, statistical inference is performed to determine whether 
the observed patterns of brain activity are statistically significant. This typically involves testing the contrast 
estimates against a null hypothesis of no effect using methods such as t-tests or F-tests. Correcting for multiple 
comparisons is essential to control for false positives in fMRI data analysis.
5. **Model Assumptions**: The GLM makes several assumptions, including linearity, independence, 
homoscedasticity (constant variance), and normality of errors. Violations of these assumptions can lead to biased 
parameter estimates and incorrect statistical inferences.

Overall, the GLM provides a flexible and widely used framework for analyzing fMRI data, allowing researchers to model 
complex experimental designs, test specific hypotheses, and draw conclusions about the neural correlates of 
cognition, perception, behavior, and various mental processes.

A simple from scratch implementation in Python is 
[described in this Medium article](https://medium.com/coinmonks/visualizing-brain-imaging-data-fmri-with-python-3e1899d1e212).

#### The Design Matrix
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

With the following command from Gordo you can visualize the design matrix:
```python
fmri.plot_design_matrix()
```
![Design Matrix](/images/design_matrix.png "Design Matrix")

#### The Contrast Matrix
In fMRI (functional Magnetic Resonance Imaging) analysis, the contrast matrix is a crucial tool used to test specific 
hypotheses about the effects of experimental conditions on brain activity. It is closely related to the design matrix 
and is used within the framework of the general linear model (GLM).
While the design matrix represents the experimental design and the relationship between experimental conditions and 
observed fMRI data, the contrast matrix specifies the specific comparisons or contrasts of interest within the context 
of the design matrix.

Here's how the contrast matrix is typically structured and used:

1. **Structure**: The contrast matrix is a mathematical matrix where each row represents a different contrast or 
comparison of interest. The number of rows in the contrast matrix corresponds to the number of contrasts being tested. 
Each contrast is specified by a set of weights assigned to the columns of the design matrix.
2. **Weights**: The values in each row of the contrast matrix specify the weights assigned to each column of the 
design matrix. These weights determine how much each condition contributes to the contrast being tested. Positive 
weights indicate conditions that are expected to increase brain activity, while negative weights indicate conditions
that are expected to decrease brain activity.
3. **Interpretation**: Once the contrast matrix is defined, it is used to compute contrast images or contrast 
estimates by multiplying it with the parameter estimates obtained from fitting the design matrix to the observed 
fMRI data. These contrast images represent the specific patterns of brain activity associated with each contrast of 
interest.
4. **Hypothesis Testing**: The contrast images obtained from the contrast matrix are then used for statistical 
inference to determine whether the observed patterns of brain activity are statistically significant. This is typically 
done using methods such as t-tests or F-tests, which compare the contrast images to a null hypothesis of no effect.

In summary, the contrast matrix in fMRI analysis allows researchers to specify and test specific hypotheses about the 
effects of experimental conditions on brain activity by defining contrasts of interest within the framework of the 
design matrix and the GLM. It provides a flexible and powerful tool for examining the neural correlates of different 
cognitive tasks, stimuli, or experimental manipulations.

With the following command from Gordo you can visualize the contrast matrix:
```python
fmri.plot_contrast_matrix()
```
![Contrast Matrix](/images/contrast_matrix.png "Contrast Matrix")

#### The Expected BOLD Response for a single voxel
The expected fMRI (functional Magnetic Resonance Imaging) response refers to the anticipated pattern of brain activity 
observed in fMRI data in response to experimental stimuli or cognitive tasks. It's essential to understand that 
fMRI measures changes in blood flow and oxygenation in the brain, known as the blood oxygen level-dependent 
(BOLD) signal. These changes are associated with neural activity, primarily reflecting synaptic activity and the 
metabolic demands of neurons.

Several factors contribute to the expected fMRI response:

1. **Hemodynamic Response Function (HRF)**: The HRF describes the relationship between neural activity and the 
observed BOLD signal. It represents the time course of the hemodynamic changes that occur following neural 
activation, including initial increases in blood flow and oxygenation followed by a gradual return to baseline. 
The shape and timing of the HRF vary across brain regions and individuals but typically peak a few seconds after 
the onset of neural activity.
2. **Experimental Design**: The design of the fMRI experiment, including the timing and nature of the experimental 
stimuli or tasks, influences the expected fMRI response. Different experimental paradigms elicit different patterns 
of neural activity and corresponding changes in the BOLD signal. For example, visual stimulation may evoke activity in
visual cortex regions, while auditory tasks may activate auditory cortex regions.
3. **Modeling Cognitive Processes**: Researchers often develop hypotheses about the cognitive processes underlying 
specific experimental manipulations. These hypotheses guide the interpretation of the expected fMRI response. 
For instance, if a task involves working memory, researchers might expect increased activity in prefrontal cortex 
regions associated with executive control and maintenance of information.
4. **Contrast Effects**: The expected fMRI response may also depend on the specific contrasts or comparisons of 
interest within the experimental design. Contrasts compare brain activity between different experimental conditions 
or groups and are often used to test specific hypotheses. For example, a contrast might compare brain activity during 
a task condition to a baseline or control condition to isolate the effects of interest.

Overall, the expected fMRI response reflects the complex interplay between neural activity, hemodynamic changes, 
experimental design, and cognitive processes. By understanding and modeling these factors, researchers can interpret 
fMRI data to draw conclusions about the neural correlates of cognition, perception, behavior, and various
mental processes.

With the following command from Gordo you can visualize the expected BOLD response for a single voxel:
```python
fmri.plot_expected_response()
```
![Expected BOLD response](/images/expected_response.png "Expected BOLD response")

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