import os
from bruker2nifti.converter import Bruker2Nifti

"""
Python module to convert fMRI  and electro-physiological data into standard data formats.

The module is named after Zaius,, the minister of science in the Planet of the Apes movies. 
- Reference: https://en.wikipedia.org/wiki/List_of_Planet_of_the_Apes_characters
"""

class ConvertBruker:
    """
    Class to convert Bruker MRI data into NIFTI format.

    Args:
        study_folder (str): Path to the folder containing the Bruker MRI data.
                            The folder should contain the 'subject' folder.
        target_folder (str): Path to the folder where the converted NIFTI files will be saved.
        study_name (str): Name of the MRI study.

    Attributes:
        study_folder (str): Path to the folder containing the Bruker MRI data.
        target_folder (str): Path to the folder where the converted NIFTI files will be saved.
        study_name (str): Name of the MRI study.
        bru (Bruker2Nifti): Instance of Bruker2Nifti converter for handling the conversion.

    Methods:
        load_study: Loads the Bruker MRI study for conversion.
        convert_2_nifti: Converts the loaded Bruker MRI study to NIFTI format and saves the converted files.
    """
    def __init__(self, study_folder, target_folder, study_name):
        self.study_folder = study_folder
        self.target_folder = target_folder
        self.study_name = study_name
        self.bru = self.initiate_study()

    def initiate_study(self):
        """
        Initializes the Bruker2Nifti converter for the Bruker MRI study, preparing it for conversion.

        This method creates the target folder for the converted files if it does not already exist.

        Returns:
            Bruker2Nifti: A Bruker2Nifti converter object configured with the study folder, target folder,
            and study name.
        """

        if os.path.isdir(self.target_folder) == False:
            os.makedirs(self.target_folder, exist_ok=True)

        return Bruker2Nifti(self.study_folder, self.target_folder, study_name=self.study_name)

    def convert_2_nifti(self) -> None:
        """
        Converts the loaded Bruker MRI study to NIFTI format and saves the converted files.

        The configuration of the conversion is defined in the code. For more details visit:
        https://github.com/SebastianoF/bruker2nifti/wiki/Example:-use-bruker2nifti-in-a-python-(Ipython)-session
        """

        self.bru.verbose = 1
        self.bru.correct_slope = True
        self.bru.get_acqp = True
        self.bru.get_method = True
        self.bru.get_reco = True
        self.bru.nifti_version = 1
        self.bru.qform_code = 1
        self.bru.sform_code = 2
        self.bru.save_human_readable = True
        self.bru.save_b0_if_dwi = True

        self.bru.convert()
