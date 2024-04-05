import os
import h5py
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


class ConvertMatFile:
    """Converts Matlab files to HDF5 format and verifies the conversion."""

    def __init__(self, mat_file_folder, target_folder):
        """
        Initializes the ConvertMatFile class with the given filename.

        Parameters:
        - filename: A string containing the path to the input Matlab file.
        - target_folder: A string containing the path to the output path.
        """
        self.mat_file_folder = mat_file_folder
        self.target_folder = target_folder
        self.mat_files = None
        self.filename = None
        self.h5_filename = None
        self.data = None
        self.header = None

    def find_mat_files(self):
        """
        Finds all Matlab files in the given folder.
        """
        mat_files = []
        for root, dirs, files in os.walk(self.mat_file_folder):
            for file in files:
                if file.endswith('.mat'):
                    mat_files.append(os.path.join(root, file))

        self.mat_files = mat_files

    def read_mat(self):
        """
        Reads the Matlab file and extracts data and header information.
        """
        with h5py.File(self.filename, 'r') as f:
            data = f['Cln']
            sampling_rate = 1 / data['dx'][()]
            header = {'sampling_rate': sampling_rate}
            data = data['dat'][()]

        self.data = data
        self.header = header

    def verify_result(self):
        """
        Verifies the correctness of the conversion by comparing the saved HDF5 file with the original data.
        """
        with h5py.File(self.h5_filename, 'r') as h5:
            h5_data = h5['data'][()]
        if not (h5_data == self.data).all():
            raise ValueError("Conversion failed. Input does not match output.")

    def construct_output_filename(self):
        """
        Constructs the output HDF5 filename based on the input ADFX filename.
        """
        data_path = self.filename.split(os.path.sep)

        if not os.path.exists(self.target_folder):
            os.makedirs(self.target_folder)

        self.h5_filename = os.path.join(self.target_folder, os.path.splitext(data_path[-1])[0] + '.h5')

    def save_as_h5(self):
        """
        Saves data and header as an HDF5 file.
        """
        with h5py.File(self.h5_filename, 'w') as h5file:
            h5file.create_dataset('/data', data=self.data)
            for key, value in self.header.items():
                h5file.attrs[key] = value

    def convert(self):
        """
        Converts the ADFX file to HDF5 format and verifies the conversion.
        """
        self.find_mat_files()

        for mat_file in self.mat_files:
            self.filename = mat_file
            self.read_mat()
            self.construct_output_filename()
            self.save_as_h5()
            self.verify_result()
