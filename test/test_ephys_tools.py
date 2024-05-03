import unittest
from unittest.mock import MagicMock, patch
import numpy as np
from parameterized import parameterized
from EphysTools import PrepareData


class TestPrepareData(unittest.TestCase):

    @parameterized.expand([
        # Test case 1: Successful reading of HDF5 file
        #('example.h5', np.array([[1, 2, 3], [4, 5, 6]]), {'attr1': 1, 'attr2': 2}, None),
        # Test case 2: OSError raised while reading HDF5 file
        ('nonexistent_file.h5', None, None, OSError("Error reading HDF5 file")),
        # Test case 3: KeyError raised while reading HDF5 file
        ('example.h5', None, None, KeyError("Required keys are missing")),
    ])
    @patch('h5py.File')
    def test_read_ephys_data(self, filename, expected_raw_signal, expected_meta, expected_exception, mock_file):
        # Mocking the return value of h5py.File
        if expected_exception:
            mock_file.side_effect = expected_exception
        else:
            mock_data = MagicMock()
            mock_data.__getitem__.return_value = expected_raw_signal
            mock_attrs = {'attr1': [1], 'attr2': [2]}
            mock_file.return_value = MagicMock(__enter__=MagicMock(return_value=mock_data),
                                               attrs=mock_attrs)

        # Setting up the object
        prepare_data = PrepareData(signal_filename=filename)

        # Testing the method
        if expected_exception:
            with self.assertRaises(type(expected_exception)):
                prepare_data.read_ephys_data()
        else:
            prepare_data.read_ephys_data()
            # Assertions
            self.assertTrue(np.array_equal(prepare_data.raw_signal, expected_raw_signal))
            self.assertEqual(prepare_data.meta, expected_meta)


if __name__ == '__main__':
    unittest.main()
