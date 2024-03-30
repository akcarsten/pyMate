import h5py
from itertools import compress
import numpy as np
import pandas as pd
from sklearn.decomposition import NMF
import matplotlib.pyplot as plt
from scipy import signal
from mne.time_frequency import tfr_array_morlet

"""
Python module to process electro-physiological data. It contains of two classes: PrepareData and SignalProcessing.

The module is named after "Manis a trained orangutan that played Clyde, Clint Eastwood's sidekick in the 1978 box office 
hit Every Which Way But Loose. Its 1980 sequel, Any Which Way You Can (1980), did not feature Manis, as the child actor 
had grown too much between productions. In the sequel, two orangutans, C.J. and Buddha, shared the role. 
Manis also featured in the 1984 action comedy film Cannonball Run II as the 'limo driver'."
- Reference: https://en.wikipedia.org/wiki/Manis_(orangutan)

Also see: https://youtu.be/cycZPTQgERk?si=uY49RQUbMvDBMOLd
"""


class PrepareData:

    def __init__(self, signal_filename: str = None):

        self.signal_filename = signal_filename
        self.channels_of_interest = None
        self.lfp_range = [0.1, 250]
        self.frequency_band_of_interest = None

        self.meta = None
        self.raw_signal = None
        self.raw_signal_sampling_rate = None
        self.raw_signal_time = None

        self.lfp_signal = None
        self.filtered_signal = None
        self.signal_amplitude = None

        self.psd_raw = None
        self.psd_lfp = None
        self.psd_frequencies = None

    def read_ephys_data(self):
        """
        Read data and metadata from an HDF5 file.

        Reads raw signal data and metadata from an HDF5 file specified by `self.signal_filename`.

        Raises:
            OSError: If the HDF5 file cannot be opened or read.
            KeyError: If required keys are missing in the HDF5 file attributes.
        """
        try:
            with h5py.File(self.signal_filename, "r") as file:
                self.raw_signal = np.array(file['data'])
                self.meta = {key: value[0] for key, value in file.attrs.items()}
        except OSError as e:
            raise OSError(f"Error reading HDF5 file: {e}")
        except KeyError as e:
            raise KeyError(f"Required keys are missing in HDF5 file attributes: {e}")

    def create_time(self, start, stop, sampling_rate=None) -> np.ndarray:

        if sampling_rate is None:
            sampling_rate = self.raw_signal_sampling_rate

        step = 1 / sampling_rate

        return np.arange(start, stop, step)

    def create_raw_signal_time(self, start=0):
        stop = np.shape(self.raw_signal)[1] / self.raw_signal_sampling_rate

        self.raw_signal_time = self.create_time(start, stop)[:-1]

    def filter_data(self, foi: list, order: int = 2):

        nyquist_frequency = self.raw_signal_sampling_rate / 2

        low = foi[0] / nyquist_frequency
        high = foi[1] / nyquist_frequency

        # Calculate filter coefficients
        b, a = signal.butter(order, [low, high], btype='band')

        return signal.lfilter(b, a, self.raw_signal)

    def plot_it(self, channel=0):
        fig, ax = plt.subplots(2, 1, figsize=(15, 5))

        ax[0].plot(self.raw_signal_time, self.lfp_signal[channel], label='lfp signal')
        ax[0].autoscale(axis='x', tight=1)
        ax[0].set_title('LFP Time Course')
        ax[0].set_xlabel('time [s]')
        ax[0].set_ylabel('lfp [mV]')

        ax[0].legend()

        ax[1].semilogx(self.psd_frequencies, self.psd_raw[channel], label='raw signal')
        ax[1].semilogx(self.psd_frequencies, self.psd_lfp[channel],
                       label=f'lfp signal ({self.lfp_range[0]} to {self.lfp_range[1]} Hz)')
        ax[1].autoscale(axis='x', tight=1)
        ax[1].set_title('Power Spectra')
        ax[1].set_xlabel('frequency [Hz]')
        ax[1].set_ylabel('PSD [V**2/Hz]')

        ax[1].legend()

        fig.tight_layout()

    def prepare_ephys_data(self):
        self.read_ephys_data()

        self.raw_signal_sampling_rate = self.meta['sampling_rate']

        if self.channels_of_interest is not None:
            self.raw_signal = [self.raw_signal[channel] for channel in self.channels_of_interest]

        self.create_raw_signal_time()

        self.lfp_signal = self.filter_data(self.lfp_range)
        self.filtered_signal = self.filter_data(self.frequency_band_of_interest)

        self.signal_amplitude = SignalProcessing.calculate_hilbert_power(self.filtered_signal)

        self.psd_frequencies, self.psd_raw = \
            SignalProcessing.calculate_power_spectral_density(self.raw_signal,
                                                              self.raw_signal_sampling_rate)

        _, self.psd_lfp = SignalProcessing.calculate_power_spectral_density(self.lfp_signal,
                                                                            self.raw_signal_sampling_rate)


class SignalProcessing:

    def __init__(self, data=None):
        self.event_threshold = 3.5
        self.event_distance = 10
        self.event_window = 3

        self.data = data

        self.signal_threshold = None
        self.signal_distance = None

        self.peaks = None
        self.lfp_events = None
        self.filtered_events = None
        self.spectra_events = None
        self.event_time = None
        self.event_clusters = None

        self.tfr_frequency_range = [5, 150]
        self.tfr_lfp = []
        self.tfr_mean_lfp = []

    def calculate_signal_threshold(self):
        self.signal_threshold = np.mean(self.data.signal_amplitude, axis=1) + \
                                np.std(self.data.signal_amplitude, axis=1) * self.event_threshold

    def calculate_signal_distance(self):
        self.signal_distance = self.event_distance * self.data.raw_signal_sampling_rate

    def detect_peaks(self):
        self.peaks = []
        for amplitude, threshold in zip(self.data.signal_amplitude, np.array(self.signal_threshold)):
            channel_peaks, _ = signal.find_peaks(amplitude, height=threshold, distance=self.signal_distance)
            self.peaks.append(channel_peaks)

    def create_event_times(self, peak):

        half_duration = self.event_window / 2
        half_width = int(self.seconds_to_samples(half_duration, self.data.raw_signal_sampling_rate))

        event_times = np.array([peak - half_width, peak + half_width]).flatten()
        event_times.sort()

        event_times = event_times.reshape(int(event_times.shape[0] / 2), 2)

        event_length = np.unique(np.diff(event_times))
        if event_length.shape[0] != 1:
            raise ValueError('Event times differ between events.')

        return event_times, event_length

    @staticmethod
    def create_events(event_times, signals):
        return [signals[x[0]:x[1]] for x in event_times]

    @staticmethod
    def validate_and_format_events(events, event_length):
        is_valid = [len(x) == event_length for x in events]
        lfp_events = np.array(list(compress(events, is_valid)))

        return np.expand_dims(lfp_events, axis=1)

    def get_events(self):

        self.lfp_events = []
        self.filtered_events = []
        for peak, lfp, filtered in zip(self.peaks, self.data.lfp_signal, self.data.filtered_signal):

            event_times, event_length = self.create_event_times(peak)

            lfp_events = self.create_events(event_times, lfp)
            lfp_events = self.validate_and_format_events(lfp_events, event_length)

            filtered_events = self.create_events(event_times, filtered)
            filtered_events = self.validate_and_format_events(filtered_events, event_length)

            self.lfp_events.append(lfp_events)
            self.filtered_events.append(filtered_events)

    def create_event_time(self):
        half_duration = self.event_window / 2
        self.event_time = self.data.create_time(-half_duration, half_duration)[:-2]

    def calculate_event_spectra(self):
        self.spectra_events = []
        for channel in self.lfp_events:
            self.spectra_events.append(self.calculate_power_spectral_density(channel,
                                                                             self.data.raw_signal_sampling_rate))

    def cluster_event_spectra_with_nnmf(self, n_components: int = 3):

        self.event_clusters = []
        for channel in self.spectra_events:
            df = pd.DataFrame(channel[1].squeeze().transpose())

            model = NMF(n_components=n_components)
            model.fit(df)

            self.event_clusters.append(model.components_.argmax(axis=0))

            # If needed, the following code can be used to reconstruct the data
            # H = pd.DataFrame(model.components_)
            # W = pd.DataFrame(model.transform(df))
            # V = pd.DataFrame(np.dot(W, H), columns=df.columns)
            # V.index = df.index

    def calculate_time_frequency_spectra(self):
        frequency_range = np.arange(self.tfr_frequency_range[0], self.tfr_frequency_range[1])

        self.tfr_lfp = []
        for channel in self.lfp_events:
            self.tfr_lfp.append(tfr_array_morlet(channel,
                                                 self.data.raw_signal_sampling_rate,
                                                 frequency_range, output='power'))

    def calculate_mean_tfr(self):
        self.tfr_mean_lfp = [x.mean(axis=0) for x in self.tfr_lfp]

    @staticmethod
    def calculate_hilbert_power(ephys_signal):
        return np.abs(signal.hilbert(ephys_signal))

    @staticmethod
    def calculate_power_spectral_density(ephys_signal, signal_sampling_rate):

        frequencies, filtered_data = signal.welch(ephys_signal,
                                                  fs=signal_sampling_rate,
                                                  window='hann',
                                                  nperseg=2048,
                                                  scaling='spectrum')
        return frequencies, filtered_data

    def plot_peaks(self, channel=0):
        time = self.data.raw_signal_time
        lfp = self.data.lfp_signal[channel]
        filtered = self.data.filtered_signal[channel]
        amplitude = self.data.signal_amplitude[channel]
        peaks = self.peaks[channel]

        frequency_band = self.data.frequency_band_of_interest

        fig, ax = plt.subplots(3, 1, figsize=(15, 8))
        ax[0].plot(time, lfp)
        ax[0].plot(time[peaks], lfp[peaks], "x")
        ax[0].autoscale(axis='x', tight=1)

        ax[0].set_title(
            f'LFP Time Course with Detected Peaks Found in Frequency Range '
            f'{frequency_band[0]} to {frequency_band[1]} Hz')
        ax[0].set_ylabel('lfp [mV]')

        ax[1].plot(time, filtered)
        ax[1].plot(time[peaks], filtered[peaks], "x")
        ax[1].autoscale(axis='x', tight=1)

        ax[1].set_title(f'Filtered LFP Time Course with Detected Peaks ({frequency_band[0]} to {frequency_band[1]} Hz)')
        ax[1].set_ylabel('filtered lfp [mV]')

        ax[2].plot(time, amplitude)
        ax[2].plot(time[peaks], amplitude[peaks], "x")
        ax[2].autoscale(axis='x', tight=1)

        ax[2].set_title('Amplitude Time Course Based on Filtered LFP')
        ax[2].set_xlabel('time [s]')
        ax[2].set_ylabel('amplitude [mV]')

        fig.tight_layout()

    def plot_mean_event_tfr(self, channel=0, xlim=None):
        if xlim is None:
            xlim = [-0.25, 0.25]
        frequency_band = self.data.frequency_band_of_interest

        fig, ax = plt.subplots(1, 2, figsize=(15, 5))
        ax[0].imshow(self.tfr_mean_lfp[channel][0, :, :],
                     aspect='auto',
                     origin='lower',
                     cmap='jet',
                     extent=[self.event_time[0], self.event_time[-1], self.tfr_frequency_range[0],
                             self.tfr_frequency_range[1]])
        ax[0].set_xlabel('time [s]')
        ax[0].set_ylabel('frequency [Hz]')
        ax[0].set_title('Time Frequency Spectrum of LFP Event')
        ax[0].set_xlim(xlim)

        ax[1].plot(self.event_time, self.filtered_events[channel].squeeze().transpose(), color='gray', alpha=0.1)
        ax[1].plot(self.event_time, self.filtered_events[channel].mean(axis=0).squeeze(), color='red', alpha=0.5)
        ax[1].set_xlabel('time [s]')
        ax[1].set_ylabel('filtered signal [mV]')
        ax[1].set_title(
            f'Events Detected in Signals Filtered in Frequency Range {frequency_band[0]} to {frequency_band[1]} Hz')
        ax[1].set_xlim(xlim)

        fig.tight_layout()

    def plot_event_spectra(self, channel=0, xlim=None):
        if xlim is None:
            xlim = [0, 200]

        fig, ax = plt.subplots(1, 1, figsize=(15, 5))

        ax.plot(self.spectra_events[channel][0], self.spectra_events[channel][1].squeeze().transpose())
        ax.set_xlabel('frequency [Hz]')
        ax.set_ylabel('PSD [V**2/Hz]')
        ax.set_title('Event Power Spectra')
        ax.set_xlim(xlim)

        fig.tight_layout()

    def plot_cluster(self, channel: int = 0, xlim_lfp=None, xlim_freq=None):

        if xlim_freq is None:
            xlim_freq = [0, 100]

        if xlim_lfp is None:
            xlim_lfp = [-0.25, 0.25]

        n_clusters = np.max([np.max(x) for x in self.event_clusters]) + 1

        fig, ax = plt.subplots(n_clusters, 3, figsize=(15, 8))

        for cluster in range(0, n_clusters):
            ind = self.event_clusters[channel] == cluster
            lfp_data = self.filtered_events[channel][ind].squeeze().transpose()
            spectra_data = self.spectra_events[channel][1][ind].squeeze().transpose()

            ax[cluster][0].imshow(self.tfr_lfp[channel][ind].mean(axis=0).squeeze(),
                                  aspect='auto',
                                  origin='lower',
                                  cmap='jet',
                                  extent=[self.event_time[0], self.event_time[-1],
                                          self.tfr_frequency_range[0],
                                          self.tfr_frequency_range[1]])
            ax[cluster][0].set_xlim(xlim_lfp)
            ax[cluster][0].set_ylabel('frequency [Hz]')

            ax[cluster][1].plot(self.spectra_events[channel][0], spectra_data, color='gray', alpha=0.1)
            ax[cluster][1].plot(self.spectra_events[channel][0], np.mean(spectra_data, axis=1), color='gray', alpha=0.5)
            ax[cluster][1].set_xlim(xlim_freq)
            ax[cluster][1].set_ylabel('PSD [V**2/Hz]')

            ax[cluster][2].plot(self.event_time, lfp_data, color='gray', alpha=0.1)
            ax[cluster][2].plot(self.event_time, np.mean(lfp_data, axis=1), color='gray', alpha=0.5)
            ax[cluster][2].set_xlim(xlim_lfp)
            ax[cluster][2].set_ylabel('filtered signal [mV]')

        ax[n_clusters - 1][0].set_xlabel('time [s]')
        ax[n_clusters - 1][1].set_xlabel('frequency [Hz]')
        ax[n_clusters - 1][2].set_xlabel('time [s]')

        fig.tight_layout()

    @staticmethod
    def show_plots():
        plt.show()

    def wheres_the_party(self):
        self.calculate_signal_threshold()
        self.calculate_signal_distance()
        self.detect_peaks()

        self.create_event_time()
        self.get_events()
        self.calculate_event_spectra()
        self.cluster_event_spectra_with_nnmf()

        self.calculate_time_frequency_spectra()
        self.calculate_mean_tfr()

        self.show_plots()

    @staticmethod
    def seconds_to_samples(seconds, fs):
        return seconds * fs

    '''
    def add_empty_rows(df, insert_rows=2, start_row=0):
        new_df_length = insert_rows * len(df)
        df.index = range(start_row, new_df_length, insert_rows)

        return df.reindex(index=range(new_df_length))

    def get_invalid_index(df):
        invalid_index = np.where(df['onset'] < 0)

        if invalid_index[0][-1] == 0 or invalid_index[0][-1] % 2:
            invalid_index = np.append(invalid_index, invalid_index[0][-1] + 1)

        return invalid_index
    '''
