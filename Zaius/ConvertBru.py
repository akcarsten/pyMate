from bruker2nifti.converter import Bruker2Nifti


class CON2NIFTI:
    """
    Class to convert Bruker MRI data into NIFTI format.

    Args:
        study_folder (str): Path to the folder containing the Bruker MRI data.
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
        self.bru = None

    def load_study(self):
        """
        Loads the Bruker MRI study for conversion.

        This method initializes the Bruker2Nifti converter with the provided study folder, target folder, and study name.
        """
        self.bru = Bruker2Nifti(self.study_folder, self.target_folder, study_name=self.study_name)

    def convert_2_nifti(self) -> None:
        """
        Converts the loaded Bruker MRI study to NIFTI format and saves the converted files.

        This method calls the 'convert' method of the Bruker2Nifti instance to perform the conversion.
        """
        self.bru.convert()
