from EphysTools import PrepareData, SignalProcessing

signal_clean = r'C:\Users\carst\Google Drive\05_Projects\NET_fMRI\data\K07.Ef1\K07.Ef1_13\k07ef1_0001_cln.h5'

channel = 0

ephys_data = PrepareData(signal_filename=signal_clean)
ephys_data.frequency_band_of_interest = [40, 70]
ephys_data.channels_of_interest = [0]

ephys_data.prepare_ephys_data()
# ephys_data.plot_it(channel)

event_data = SignalProcessing(ephys_data)
event_data.wheres_the_party()

event_data.plot_peaks(channel)
event_data.plot_mean_event_spectra()

event_data.show_plots()
