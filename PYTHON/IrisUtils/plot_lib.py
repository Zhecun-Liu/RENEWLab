import numpy as np
import matplotlib
import matplotlib.pyplot as plt

# Plots IQ samples in one single frame as well as all frames for
# selected cell,  BS ant, and user
def plot_iq_samps(samps, frm_st, frame_i, cells, users, ants, data_str="Pilots"):
    # Samps Dimensions: (Frame, Cell, User, Antenna, Sample)
    for i in cells:
        for j in users:
            for k in ants:
                cell_i = i
                user_i = j
                ant_i = k
                fig, axes = plt.subplots(nrows=2, ncols=1, squeeze=False, figsize=(10, 8))
                axes[0, 0].set_title(data_str + " IQ - Cell %d - Antenna %d - User %d"%(cell_i, ant_i, user_i))
                axes[0, 0].set_ylabel('Frame %d (IQ)' %( (frame_i + frm_st)) )
                axes[0, 0].plot(np.real(samps[frame_i, cell_i, user_i, ant_i, :]))
                axes[0, 0].plot(np.imag(samps[frame_i, cell_i, user_i, ant_i, :]))

                axes[1, 0].set_ylabel('All Frames (IQ)')
                axes[1, 0].plot(np.real(samps[:, cell_i, user_i, ant_i, :]).flatten())
                axes[1, 0].plot(np.imag(samps[:, cell_i, user_i, ant_i, :]).flatten())

def plot_csi(csi, corr, bs_nodes, good_frames, frame_i, cell_i, user_i, subcarrier_i, offset, data_str="Uplink"):
    fig, axes = plt.subplots(nrows=3, ncols=1, squeeze=False, figsize=(10, 8))
    axes[0, 0].set_title(data_str + " Pilot CSI Stats Across Frames- Cell %d - User %d - Subcarrier %d" % (cell_i, user_i, subcarrier_i))
    axes[0, 0].set_ylabel('Magnitude')
    for i in range(csi.shape[2]):
        axes[0, 0].plot(np.abs(csi[:, user_i, i, subcarrier_i]).flatten(), label="ant %d" % bs_nodes[i])
    axes[0, 0].legend(loc='lower right', frameon=False)
    axes[0, 0].set_xlabel('Frame')

    axes[1, 0].set_ylabel('Phase')
    for i in range(csi.shape[2]):
        axes[1, 0].plot(np.angle(csi[:, user_i, i, subcarrier_i]).flatten(), label="ant %d" % bs_nodes[i])
    axes[1, 0].legend(loc='lower right', frameon=False)
    axes[1, 0].set_ylim(-np.pi, np.pi)
    axes[1, 0].set_xlabel('Frame')

    axes[2, 0].set_ylabel('Correlation with Frame %d' % frame_i)
    axes[2, 0].set_ylim([0, 1.1])
    axes[2, 0].set_title('Cell %d offset %d' % (0, offset))
    for u in range(corr.shape[1]):
        axes[2, 0].plot(corr[good_frames, u], label="user %d"%u)
    axes[2, 0].legend(loc='lower right', frameon=False)
    axes[2, 0].set_xlabel('Frame')

def plot_calib(calib_mat, bs_nodes, frame_i, ant_i, subcarrier_i):
    fig, axes = plt.subplots(nrows=4, ncols=1, squeeze=False, figsize=(10, 8))
    axes[0, 0].set_title('Reciprocity Calibration Factor Across Frames - Cell 0 - Subcarrier %d' % subcarrier_i)

    axes[0, 0].set_ylabel('Magtinute (ant %d)' % (ant_i))
    axes[0, 0].plot(np.abs(calib_mat[:, ant_i, subcarrier_i]).flatten(), label='')
    axes[0, 0].set_xlabel('Frame')
    axes[0, 0].legend(frameon=False)

    axes[1, 0].set_ylabel('Phase (ant %d)' % (ant_i))
    axes[1, 0].plot(np.angle(calib_mat[:, ant_i, subcarrier_i]).flatten())
    axes[1, 0].set_ylim(-np.pi, np.pi)
    axes[1, 0].set_xlabel('Frame')
    axes[1, 0].legend(frameon=False)
    axes[1, 0].grid()

    axes[2, 0].set_ylabel('Magnitude')
    for i in range(calib_mat.shape[1]):
        axes[2, 0].plot(np.abs(calib_mat[:, i, subcarrier_i]).flatten(), label="ant %d" % bs_nodes[i])
    axes[2, 0].set_xlabel('Frame')
    axes[2, 0].legend(loc='lower right', frameon=False)

    axes[3, 0].set_ylabel('Phase')
    for i in range(calib_mat.shape[1]):
        axes[3, 0].plot(np.angle(calib_mat[:, i, subcarrier_i]).flatten(), label="ant %d" % bs_nodes[i])
    axes[3, 0].set_xlabel('Frame')
    axes[3, 0].set_ylim(-np.pi, np.pi)
    axes[3, 0].legend(loc='lower right', frameon=False)
    axes[3, 0].grid()

    fig, axes = plt.subplots(nrows=4, ncols=1, squeeze=False, figsize=(10, 8))
    axes[0, 0].set_title('Reciprocity Calibration Factor Across Subcarriers - Cell 0 - Frame %d' % frame_i)
    axes[0, 0].set_ylabel('Magnitude ant %d' % (ant_i))
    axes[0, 0].plot(np.abs(calib_mat[frame_i, ant_i, :]).flatten())
    axes[0, 0].set_xlabel('Subcarrier')

    axes[1, 0].set_ylabel('Phase ant %d' % (ant_i))
    axes[1, 0].plot(np.angle(calib_mat[frame_i, ant_i, :]).flatten())
    axes[1, 0].set_ylim(-np.pi, np.pi)
    axes[1, 0].set_xlabel('Subcarrier')

    axes[2, 0].set_ylabel('Magnitude')
    for i in range(calib_mat.shape[1]):
        axes[2, 0].plot(np.abs(calib_mat[frame_i, i, :]).flatten(), label="ant %d" % bs_nodes[i])
    axes[2, 0].set_xlabel('Subcarrier')
    axes[2, 0].legend(loc='lower right', frameon=False)

    axes[3, 0].set_ylabel('Phase')
    for i in range(calib_mat.shape[1]):
        axes[3, 0].plot(np.angle(calib_mat[frame_i, i, :]).flatten(), label="ant %d" % bs_nodes[i])
    axes[3, 0].set_xlabel('Subcarrier')
    axes[3, 0].set_ylim(-np.pi, np.pi)
    axes[3, 0].legend(loc='lower right', frameon=False)

def plot_constellation_stats(evm, evm_snr, ul_data, txdata, frame_i, cell_i, ul_slot_i, data_str = "Uplink"):
    n_users = ul_data.shape[1]
    plt_x_len = int(np.ceil(np.sqrt(n_users)))
    plt_y_len = int(np.ceil(n_users / plt_x_len))
    fig5, axes5 = plt.subplots(nrows=plt_y_len, ncols=plt_x_len, squeeze=False, figsize=(10, 8))
    fig5.suptitle(data_str+" User Constellations (ZF) - Frame %d - Cell %d - UL SF %d" % (frame_i, cell_i, ul_slot_i))
    fig6, axes6 = plt.subplots(nrows=2, ncols=1, squeeze=False, figsize=(10, 8))
    fig6.suptitle('Uplink EVM/SNR - Cell %d - UL SF %d' % (cell_i, ul_slot_i))
    axes6[0, 0].set_ylabel('EVM (%)')
    axes6[1, 0].set_ylabel('EVM-SNR (dB)')
    axes6[0, 0].set_xlabel('Frame Number')
    for i in range(n_users):
        y_i = int(i // plt_x_len)
        x_i = i % plt_x_len
        axes5[y_i, x_i].set_title('User %d'%(i))
        axes5[y_i, x_i].scatter(np.real(ul_data[frame_i, i, ul_slot_i, :]), np.imag(ul_data[frame_i, i, ul_slot_i, :]))
        axes5[y_i, x_i].scatter(np.real(txdata[frame_i, i, ul_slot_i, :]), np.imag(txdata[frame_i, i, ul_slot_i, :]))

        axes6[0, 0].plot(range(ul_data.shape[0]), 100 * evm[:, i], label='User %d'%(i))
        axes6[1, 0].plot(range(ul_data.shape[0]), evm_snr[:, i], label='User %d'%(i))
    axes6[0, 0].legend(loc='upper right', frameon=False)

def show_plot(cmpx_pilots, lts_seq_orig, match_filt, ref_user, ref_ant, ref_frame, frm_st_idx):
    '''
    Plot channel analysis
    '''

    # WZC: fix the hardcode issue
    frame_to_plot = ref_frame
    frm_st_idx = frm_st_idx
    ref_ant = ref_ant
    ref_user = ref_user
    test_mf = False
    debug = False

    fig = plt.figure()
    ax1 = fig.add_subplot(3, 1, 1)
    ax1.grid(True)
    ax1.set_title(
        'channel_analysis:csi_from_pilots(): Re of Rx pilot - ref frame {} and ref ant. {} (UE {})'.format(
            frame_to_plot, ref_ant, ref_user))

    if debug:
        print("cmpx_pilots.shape = {}".format(cmpx_pilots.shape))

    ax1.plot(
        np.real(cmpx_pilots[frame_to_plot - frm_st_idx, 0, ref_user, ref_ant, :]))

    z_pre = np.zeros(82, dtype='complex64')
    z_post = np.zeros(68, dtype='complex64')
    lts_t_rep = lts_seq_orig

    if debug:

        lts_t_rep_tst = np.append(z_pre, lts_t_rep)
        lts_t_rep_tst = np.append(lts_t_rep_tst, z_post)

        if test_mf:
            w = np.random.normal(0, 0.1 / 2, len(lts_t_rep_tst)) + \
                1j * np.random.normal(0, 0.1 / 2, len(lts_t_rep_tst))
            lts_t_rep_tst = lts_t_rep_tst + w
            cmpx_pilots = np.tile(
                lts_t_rep_tst, (n_frame, cmpx_pilots.shape[1], cmpx_pilots.shape[2], cmpx_pilots.shape[3], 1))
            print("if test_mf: Shape of lts_t_rep_tst: {} , cmpx_pilots.shape = {}".format(
                lts_t_rep_tst.shape, cmpx_pilots.shape))

        loc_sec = lts_t_rep_tst
    else:
        loc_sec = np.append(z_pre, lts_t_rep)
        loc_sec = np.append(loc_sec, z_post)
    ax2 = fig.add_subplot(3, 1, 2)
    ax2.grid(True)
    ax2.set_title(
        'channel_analysis:csi_from_pilots(): Local LTS sequence zero padded')
    ax2.plot(loc_sec)

    ax3 = fig.add_subplot(3, 1, 3)
    ax3.grid(True)
    ax3.set_title(
        'channel_analysis:csi_from_pilots(): MF (uncleared peaks) - ref frame {} and ref ant. {} (UE {})'.format(
            frame_to_plot, ref_ant, ref_user))
    ax3.stem(match_filt[frame_to_plot - frm_st_idx, 0, ref_user, ref_ant, :])
    ax3.set_xlabel('Samples')

def plot_cfo(cfo, n_frm_st, ant_i = -1):
    n_cell = cfo.shape[1]
    n_ue = cfo.shape[2]
    n_ant = cfo.shape[3]
    fig, axes = plt.subplots(nrows=n_ue, ncols=n_cell, squeeze=False)
    fig.suptitle('Frames\' CFO dices per antenna') 
    for n_c in range(n_cell):
        for n_u in range(n_ue):
            cfo_u = cfo[:,n_c,n_u,:]
            x_pl = np.arange(cfo_u.shape[0]) + n_frm_st
            for j in range(n_ant):
                if ant_i == -1 or j == ant_i:
                    axes[n_u, n_c].plot(x_pl,cfo_u[:,j].flatten(), label = 'Antenna: {}'.format(j) )
            axes[n_u, n_c].legend(loc='lower right', ncol=2, frameon=False)
            axes[n_u, n_c].set_xlabel('Frame no.')
            axes[n_u, n_c].set_ylabel('CFO (Hz)')
            axes[n_u, n_c].grid(True)

def plot_start_frame(sub_fr_strt, n_frm_st):
    n_cell = sub_fr_strt.shape[1]
    n_ue = sub_fr_strt.shape[2]
    n_ant = sub_fr_strt.shape[3]
    fig, axes = plt.subplots(nrows=n_ue, ncols=n_cell, squeeze=False)
    fig.suptitle('Frames\' starting indices per antenna')
    for n_c in range(n_cell):
        for n_u in range(n_ue):
            sf_strts = sub_fr_strt[:,n_c,n_u,:]
            x_pl = np.arange(sf_strts.shape[0]) + n_frm_st
            for j in range(n_ant):
                axes[n_u, n_c].plot(x_pl,sf_strts[:,j].flatten(), label = 'Antenna: {}'.format(j) )
            axes[n_u, n_c].legend(loc='lower right', ncol=2, frameon=False)
            axes[n_u, n_c].set_xlabel('Frame no.')
            axes[n_u, n_c].set_ylabel('Starting index')
            axes[n_u, n_c].grid(True)

def plot_pilot_mat(seq_found, n_frm_st, n_frm_end):
    n_cell = seq_found.shape[1]
    n_ue = seq_found.shape[2]
    n_ant = seq_found.shape[3]
    fig, axes = plt.subplots(nrows=n_ue, ncols=n_cell, squeeze=False)
    c = []
    fig.suptitle('Pilot Map (Percentage of Detected Pilots Per Symbol) - NOTE: Might exceed 100% due to threshold')
    for n_c in range(n_cell):
        for n_u in range(n_ue):
            c.append(axes[n_u, n_c].imshow(seq_found[:, n_c, n_u, :].T, vmin=0, vmax=100, cmap='Blues',
                                           interpolation='nearest',
                                           extent=[n_frm_st, n_frm_end, n_ant, 0],
                                           aspect="auto"))
            axes[n_u, n_c].set_title('Cell {} UE {}'.format(n_c, n_u))
            axes[n_u, n_c].set_ylabel('Antenna #')
            axes[n_u, n_c].set_xlabel('Frame #')
            axes[n_u, n_c].set_xticks(np.arange(n_frm_st, n_frm_end, 1), minor=True)
            axes[n_u, n_c].set_yticks(np.arange(0, n_ant, 1), minor=True)
            axes[n_u, n_c].grid(which='minor', color='0.75', linestyle='-', linewidth=0.05)
    cbar = plt.colorbar(c[-1], ax=axes.ravel().tolist(), ticks=np.linspace(0, 100, 11), orientation='horizontal')
    cbar.ax.set_xticklabels(['0%', '10%', '20%', '30%', '40%', '50%', '60%', '70%', '80%', '90%', '100%'])

    ## For some reason, if one of the subplots has all of the frames in the same state (good/bad/partial)
    ## it chooses a random color to paint the whole subplot!
    ## Below is some sort of remedy (will fail if SISO!):
    #for n_c in range(frame_map.shape[1]):
    #    for n_u in range(frame_map.shape[2]):
    #        f_map = frame_map[:,n_c,n_u,:]
    #        n_gf = f_map[f_map == 1].size
    #        n_bf = f_map[f_map == -1].size
    #        n_pr = f_map[f_map == 0].size
    #        if n_gf == 0:
    #            frame_map[-1,n_c,n_u,-1] = 1
    #            print("No good frames! Colored the last frame of the last antenna Good for cell {} and UE {} to keep plotter happy!".format(n_c,n_u))

    #        if n_pr == 0:
    #            frame_map[0,n_c,n_u,-1] = 0
    #            print("No partial frames! Colored frame 0 of the last antenna for cell {} and UE {} Partial to keep plotter happy!".format(n_c,n_u))
    #        if n_bf == 0:
    #            frame_map[-1,n_c,n_u,0] = -1
    #            print("No bad frames! Colored the last frame of antenna 0 Bad for cell {} and UE {} to keep plotter happy!".format(n_c,n_u))

    #fig, axes = plt.subplots(nrows=n_ue, ncols=n_cell, squeeze=False)
    #c = []
    #fig.suptitle('Frame Map')
    #for n_c in range(n_cell):
    #    for n_u in range(n_ue):
    #        c.append( axes[n_u, n_c].imshow(frame_map[:,n_c,n_u,:].T, cmap=plt.cm.get_cmap('Blues', 3), interpolation='none',
    #              extent=[n_frm_st,n_frm_end, n_ant,0],  aspect="auto") )
    #        axes[n_u, n_c].set_title('Cell {} UE {}'.format(n_c, n_u))
    #        axes[n_u, n_c].set_ylabel('Antenna #')
    #        axes[n_u, n_c].set_xlabel('Frame #')
    #        # Minor ticks
    #        axes[n_u, n_c].set_xticks(np.arange(n_frm_st, n_frm_end, 1), minor=True)
    #        axes[n_u, n_c].set_yticks(np.arange(0, n_ant, 1), minor=True)
    #        # Gridlines based on minor ticks
    #        axes[n_u, n_c].grid(which='minor', color='0.75', linestyle='-', linewidth=0.1)

    #cbar = plt.colorbar(c[-1], ax=axes.ravel().tolist(), ticks=[-1, 0, 1], orientation = 'horizontal')
    #cbar.ax.set_xticklabels(['Bad Frame', 'Probably partial/corrupt', 'Good Frame'])
def plot_snr_map(snr, n_frm_st, n_frm_end, n_ant):
    n_cell = snr.shape[1]
    n_ue = snr.shape[2]
    fig, axes = plt.subplots(nrows=n_ue, ncols=n_cell, squeeze=False)
    c = []
    fig.suptitle('SNR Map')
    for n_c in range(n_cell):
        for n_u in range(n_ue):
            c.append(
                axes[n_u, n_c].imshow(snr[:, n_c, n_u, :].T, vmin=np.min(snr), vmax=np.max(snr), cmap='Blues',
                                      interpolation='nearest',
                                      extent=[n_frm_st, n_frm_end, n_ant, 0],
                                      aspect="auto"))
            axes[n_u, n_c].set_title('Cell {} UE {}'.format(n_c, n_u))
            axes[n_u, n_c].set_ylabel('Antenna #')
            axes[n_u, n_c].set_xlabel('Frame #')
            axes[n_u, n_c].set_xticks(np.arange(n_frm_st, n_frm_end, 1), minor=True)
            axes[n_u, n_c].set_yticks(np.arange(0, n_ant, 1), minor=True)
            axes[n_u, n_c].grid(which='minor', color='0.75', linestyle='-', linewidth=0.05)
    cbar = plt.colorbar(c[-1], ax=axes.ravel().tolist(), ticks=np.linspace(0, np.max(snr), 10),
                        orientation='horizontal')

def plot_match_filter(match_filt, ref_frame, n_frm_st, ant_i):
    n_cell = match_filt.shape[1]
    n_ue = match_filt.shape[2]

    # plot a frame:
    fig, axes = plt.subplots(nrows=n_cell, ncols=n_ue, squeeze=False)
    fig.suptitle('MF Frame # {} Antenna # {}'.format(ref_frame, ant_i))
    for n_c in range(n_cell):
        for n_u in range(n_ue):
            axes[n_c, n_u].stem(match_filt[ref_frame - n_frm_st, n_c, n_u, ant_i, :])
            axes[n_c, n_u].set_xlabel('Samples')
            axes[n_c, n_u].set_title('Cell {} UE {}'.format(n_c, n_u))
            axes[n_c, n_u].grid(True)

def plot_spectral_efficiency(subf_conj, subf_zf, mubf_conj, mubf_zf, timestamp, num_cl, n_ant, n_ue, frame_i):
    fig1, axes1 = plt.subplots(nrows=2, ncols=2, squeeze=False, figsize=(10, 8))
    axes1[0, 0].set_title('Subcarrier-Mean Spectral Efficiency Using Beamforming Weights at Frame %d'%frame_i)
    for j in range(num_cl):
        axes1[0, 0].plot(timestamp, mubf_conj[:,j], label = 'Conj User: {}'.format(j) )
    for j in range(num_cl):
        axes1[0, 1].plot(timestamp, mubf_zf[:,j], label = 'ZF User: {}'.format(j) )
    axes1[0,0].legend(loc='upper right', ncol=1, frameon=False)
    axes1[0,0].set_xlabel('Time (s)', fontsize=14)
    axes1[0,0].set_ylabel('MUBF %dx%d (bps/Hz)'%(n_ant, n_ue), fontsize=14)
    axes1[0,1].legend(loc='upper right', ncol=1, frameon=False)
    axes1[0,1].set_xlabel('Time (s)', fontsize=14)
    for j in range(num_cl):
        axes1[1, 0].plot(timestamp, subf_conj[:,j], label = 'Conj User: {}'.format(j) )
    for j in range(num_cl):
        axes1[1, 1].plot(timestamp, subf_zf[:,j], label = 'ZF User: {}'.format(j) )
    axes1[1,0].legend(loc='upper right', ncol=1, frameon=False)
    axes1[1,0].set_xlabel('Time (s)', fontsize=14)
    axes1[1,0].set_ylabel('SUBF %dx1 (bps/Hz)'%n_ant, fontsize=14)
    axes1[1,1].legend(loc='upper right', ncol=1, frameon=False)
    axes1[1,1].set_xlabel('Time (s)', fontsize=14)

def plot_demmel_snr(demmel, timestamp, subcarrier_i):
    plt.figure(pl+2, figsize=(10, 8))
    plt.plot(timestamp, demmel[:, subcarrier_i])
    plt.xlabel('Time (s)', fontsize=14)
    plt.ylabel('Condition Number', fontsize=14)
    plt.title('CSI Matrix Demmel condition number across time, Subcarrier %d'%subcarrier_i)
    #pl += 1

    # SNR
    #snr_linear = np.mean(zf[-1], axis = -1)
    #snr_dB = 10 * np.log10(snr_linear)
    #plt.figure(pl+2, figsize=(10, 8))
    #for i in range(num_cl_tmp):
    #    plt.plot(np.arange(0, csi.shape[0]*timestep, timestep)[:csi.shape[0]], snr_dB[:, i], label = 'User: {}'.format(i))
    ## plt.ylim([0,2])
    #plt.xlabel('Time (s)', fontsize=14)
    #plt.ylabel('ZF SNR (dB)', fontsize=14)
    #plt.title('ZF SNR Across Frames')
    #plt.legend()
