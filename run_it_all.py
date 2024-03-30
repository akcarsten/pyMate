from pyMate.Gordo import MriProcessing

root_folder = 'C:\\Users\\carst\\Google Drive\\05_Projects\\NET_fMRI\pyMate'
subject_folder = 'C:\\Users\carst\\Google Drive\\05_Projects\\NET_fMRI\\data\\CM033.zJ1'

micro_session_files = [
    f'{root_folder}\\micro\\cm033_zj1micro_010.csv',
    f'{root_folder}\\micro\\cm033_zj1micro_011.csv',
    f'{root_folder}\\micro\\cm033_zj1micro_012.csv',
    f'{root_folder}\\micro\\cm033_zj1micro_013.csv']

opto_session_files = [
    f'{root_folder}\\opto\\cm033_zj1opto_001.csv',
    f'{root_folder}\\opto\\cm033_zj1opto_002.csv',
    f'{root_folder}\\opto\\cm033_zj1opto_003.csv']

fmri = MriProcessing(micro_session_files, subject_folder)
fmri.threshold = None # 2.0  # Set to None (default) to get threshold based on statistical test
fmri.mask_img = None  # Turn on (None) or off (False) the masking of the EPI data
fmri.smoothing_fwhm = 4
fmri.lets_go()
