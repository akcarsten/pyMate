import os
import unittest
from parameterized import parameterized
from unittest.mock import MagicMock
import pandas as pd
from MriTools import FMRIDataProcessor


class TestLoadSessionFiles(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mock_input1 = MagicMock()
        cls.mock_input2 = MagicMock()

    @parameterized.expand([
        (["perfect_sessions.csv", "great_subject.csv"],),  # Test case with multiple files
        ([],),  # Test case with empty file list
        # Add more test cases as needed
    ])
    def test_load_session_files(self, session_files):
        # Initialize the class instance with mocks and session files
        obj = FMRIDataProcessor(self.mock_input1, self.mock_input2)
        obj.session_files = session_files

        # Mocking pd.read_csv to return a DataFrame with some sample data
        def mock_read_csv(filepath):
            return pd.DataFrame({"A": [1, 2], "B": [3, 4]})

        pd.read_csv = mock_read_csv

        if not obj.session_files:
            with self.assertRaises(Exception):
                obj.load_session_files()
        else:
            obj.load_session_files()
            self.assertIsInstance(obj.session_info, pd.DataFrame)

            if session_files:
                self.assertEqual(obj.session_info.shape[0], len(session_files) * 2)

    if __name__ == "__main__":
        unittest.main()
