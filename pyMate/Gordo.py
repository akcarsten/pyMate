import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from nilearn import image, plotting
from nilearn.glm import threshold_stats_img
from nilearn.glm.first_level import make_first_level_design_matrix, FirstLevelModel
from typing import Dict

"""
Python module to process fMRI data. It contains one class, MriProcessing.

The module is named after Gordo, a squirrel monkey who traveled to space in 1958.
"Gordo was one of the first monkeys to travel into space. As part of the NASA space program, Gordo, also known
as Old Reliable, was launched from Cape Canaveral on December 13, 1958, in the U.S. PGM-19 Jupiter rocket on
its AM-13 mission. The rocket would travel over 1,500 miles and reach a height of 310 miles (500 km) before
returning to Earth and landing in the South Atlantic. A technical malfunction prevented the capsule's parachute
from opening and, despite a short search, neither his body nor the vessel were ever recovered."
- Reference: https://en.wikipedia.org/wiki/Gordo_(monkey)
"""


class MriProcessing:

    def __init__(self, session_files=None, subject_folder=None):
        self.session_files = session_files
        self.subject_folder = subject_folder

        self.csv_delimiter = ','
        self.hrf_model = 'spm'
        self.drift_model = 'cosine'
        self.high_pass = 0.01
        self.noise_model = "ar1"
        self.smoothing_fwhm = 3
        self.dgz_repetition_time_name = '-InterVolumeTime (ms)'

        self.session_info = pd.DataFrame
        self.filenames = pd.DataFrame
        self.fmri_img = []

        self.design_matrices = []
        self.basic_contrasts = {}
        self.contrasts = {}

        self.fmri_glm = FirstLevelModel
        self.z_map = []
        self.threshold = None
        self.mask_img = False
        self.mean_epi_image = []

    def load_session_files(self) -> None:
        """
        Load session files into a single pandas DataFrame.

        This method reads multiple CSV files specified in `self.session_files`,
        concatenates them into a single DataFrame, and returns the result.

        Returns:
            None

        Raises:
            ValueError: If `self.session_files` is empty or if any file fails to load.
            FileNotFoundError: If any file in `self.session_files` does not exist.
            pd.errors.ParserError: If any CSV file cannot be parsed.
        """
        if not self.session_files:
            raise ValueError("`session_files` is empty.")

        try:
            dfs = [pd.read_csv(session) for session in self.session_files]
            concatenated_df = pd.concat(dfs, ignore_index=True)
        except FileNotFoundError as e:
            raise FileNotFoundError(f"File not found: {e.filename}") from e
        except pd.errors.ParserError as e:
            raise pd.errors.ParserError(f"Failed to parse CSV file: {e}") from e

        self.session_info = concatenated_df

    def get_data_filenames(self) -> None:
        """
        Get data filenames from subject folder and session information DataFrame.

        Returns:
            None
        """
        self.filenames = {column: [os.path.join(self.subject_folder, x) for x in self.session_info[column]]
                          for column in self.session_info.columns if column.endswith('_file')}

    def load_fmri_data(self) -> None:
        """
        Load fMRI data from filenames.

        Returns:
            None
        """
        self.fmri_img = [image.load_img(x) for x in self.filenames['func_file']]

    def resample_fmri_data(self):
        """
        Resample fMRI data.

        Returns:
            None
        """
        self.fmri_img = [image.resample_to_img(x, self.fmri_img[0]) for x in self.fmri_img]

    def create_design_matrix(self) -> None:
        """
        Create design matrix for fMRI data.

        Returns:
            None
        """
        design_matrices = []
        for img, (event_file, info_file) in zip(self.fmri_img,
                                                zip(self.filenames['event_file'], self.filenames['info_file'])):
            events = pd.read_csv(event_file, delimiter=self.csv_delimiter)
            info_data = pd.read_csv(info_file, delimiter=self.csv_delimiter)

            repetition_time = float(
                info_data[info_data['parameter'] == self.dgz_repetition_time_name]['value'].iloc[0]) / 1000

            frame_times = np.arange(img.shape[-1]) * repetition_time

            matrix = make_first_level_design_matrix(
                frame_times,
                events,
                hrf_model=self.hrf_model,
                drift_model=self.drift_model,
                high_pass=self.high_pass,
            )
            design_matrices.append(matrix)

        self.design_matrices = design_matrices

    def fit_glm(self) -> None:
        """
        Fit GLM model to fMRI data.

        Returns:
            None
        """
        fmri_glm = FirstLevelModel(noise_model=self.noise_model,
                                   smoothing_fwhm=self.smoothing_fwhm,
                                   mask_img=self.mask_img)

        self.fmri_glm = fmri_glm.fit(self.fmri_img, design_matrices=self.design_matrices)

    def create_contrast_matrix(self) -> None:
        """
        Create contrast matrix from design matrices.

        Returns:
            Dict[str, np.ndarray]: Dictionary of contrast matrices.
        """
        self.basic_contrasts = \
            {column: np.eye(len(matrix.columns))[i] for matrix in self.design_matrices for i, column in
             enumerate(matrix.columns)}

    def create_contrasts(self) -> None:
        """
        Create contrasts from basic contrasts.

        Returns:
            None
        """
        rest, active = self.basic_contrasts["rest"], self.basic_contrasts["active"]

        self.contrasts = {
            "rest-active": rest - active,
            "active-rest": active - rest,
            "effects_of_interest": np.vstack((active, rest)),
        }

    def create_mean_epi_image(self) -> None:
        """
        Create a mean EPI (Echo Planar Imaging) image from the functional MRI (fMRI) data.

        This method computes the mean image from the provided functional MRI data,
        which typically consists of multiple volumes acquired over time.
        The mean EPI image represents the average intensity across all volumes
        and serves as a common reference for further analysis and visualization.

        The mean EPI image is stored in the `mean_epi_image` attribute of the object.

        Returns:
            None
        """
        self.mean_epi_image = image.mean_img(self.fmri_img)

    def create_z_map(self) -> None:
        """
        Create a Z-map (Z-score map) from the fitted fMRI General Linear Model (GLM).

        This method computes a Z-map representing the statistical significance of a contrast
        specified by the 'effects_of_interest' key in the contrast dictionary.
        The Z-map indicates the strength of activation or deactivation at each voxel based on
        the contrast specified. The Z-map is stored in the `z_map` attribute of the object.

        Returns:
            None
        """
        self.z_map = self.fmri_glm.compute_contrast(self.contrasts['effects_of_interest'], output_type="z_score")

    def create_threshold(self) -> None:
        """
        Create a threshold for the Z-map to identify significant activations.

        This method computes a statistical threshold for the Z-map to identify voxels
        with significant activations or deactivations based on the specified parameters.
        The thresholded Z-map is stored in the `threshold` attribute of the object.

        Returns:
            None
        """
        _, self.threshold = threshold_stats_img(self.z_map,
                                                alpha=0.05,
                                                height_control="bonferroni",
                                                cluster_threshold=10)

    def plot_activation_map(self):
        """
        View the activation map in a web browser.

        This method generates an HTML file containing an interactive visualization
        of the activation map derived from the Z-map (`z_map`) with the specified threshold.
        The activation map is overlaid on a background image (`mean_epi_image`) to provide context.
        The generated HTML file is opened in the default web browser for interactive exploration.

        Returns:
            None
        """

        html = plotting.view_img(
            stat_map_img=self.z_map,
            threshold=self.threshold,
            bg_img=self.mean_epi_image,
            title="Activation Map")

        html.open_in_browser()

    def plot_contrast_matrix(self):
        """
        Plot the contrast matrix.

        This method generates a visualization of the contrast matrix available for the analysis.
        The contrast matrix represents the linear combination of the estimated effects
        corresponding to the contrasts of interest. The plot provides a visual representation
        of the contrast weights assigned to each effect in the design matrix.

        Returns:
            None
        """
        num_rows = len(self.contrasts)
        fig, ax = plt.subplots(num_rows, 1, figsize=(15, 5))

        for i, (contrast_id, contrast_val) in enumerate(self.contrasts.items()):
            plotting.plot_contrast_matrix(contrast_val, design_matrix=self.design_matrices[0], ax=ax[i])
            ax[i].set_title(contrast_id)

        plt.tight_layout()

    def lets_go(self):
        """
        Process an FMRI session from start to finish. If attributes are not changed all analysis will be performed
        with default settings.
        """
        self.load_session_files()
        self.get_data_filenames()

        self.load_fmri_data()
        self.resample_fmri_data()
        self.create_mean_epi_image()

        self.create_design_matrix()

        self.create_contrast_matrix()
        self.create_contrasts()

        self.fit_glm()
        self.create_z_map()
        if self.threshold is None:
            self.create_threshold()

        self.plot_activation_map()
