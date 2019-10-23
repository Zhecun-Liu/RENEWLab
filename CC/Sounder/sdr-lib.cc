/*
 Copyright (c) 2018-2019, Rice University 
 RENEW OPEN SOURCE LICENSE: http://renew-wireless.org/license
 Author(s): Rahman Doost-Mohamamdy: doost@rice.edu
 
---------------------------------------------------------------------
 Initializes and Configures Radios in the massive-MIMO base station 
---------------------------------------------------------------------
*/

#include "include/sdr-lib.h"
#include "include/Radio.h"
#include "include/comms-lib.h"
#include "include/macros.h"
#include <SoapySDR/Errors.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Time.hpp>

ClientRadioSet::ClientRadioSet(Config* cfg)
    : _cfg(cfg)
{
    //load channels
    std::vector<size_t> channels;
    if (_cfg->bsChannel == "A")
        channels = { 0 };
    else if (cfg->bsChannel == "B")
        channels = { 1 };
    else
        channels = { 0, 1 };
    radios.reserve(_cfg->nClSdrs);
    for (size_t i = 0; i < _cfg->nClSdrs; i++) {
        SoapySDR::Kwargs args;
        args["timeout"] = "1000000";
        args["serial"] = _cfg->cl_sdr_ids.at(i);
        try {
            radios.push_back(new Radio(args, SOAPY_SDR_CF32, channels, _cfg->rate));
        } catch (std::runtime_error) {
            std::cerr << "Ignoring serial " << _cfg->cl_sdr_ids.at(i) << std::endl;
            continue;
        }
        auto dev = radios.back()->dev;
        SoapySDR::Kwargs info = dev->getHardwareInfo();

        for (auto ch : { 0, 1 }) //channels)
        {
            double rxgain = _cfg->clRxgain_vec[ch][i]; //[0,30]
            double txgain = _cfg->clTxgain_vec[ch][i]; //[0,52]
            // No client calibration for now!
            radios.back()->dev_init(_cfg, ch, rxgain, txgain, info["frontend"]);
        }

        initAGC(dev);
    }
    radios.shrink_to_fit();

    int ueTrigOffset = 505; //_cfg->prefix + 256 + _cfg->postfix + 17 + _cfg->prefix;
    int sf_start = ueTrigOffset / _cfg->sampsPerSymbol;
    int sp_start = ueTrigOffset % _cfg->sampsPerSymbol;

    for (size_t i = 0; i < radios.size(); i++) {
        auto dev = radios[i]->dev;
        dev->writeRegister("IRIS30", CORR_CONF, 0x1);
        for (int k = 0; k < 128; k++)
            dev->writeRegister("ARGCOE", k * 4, 0);
        usleep(100000);

        dev->writeRegister("IRIS30", 64, 1); // reset faros_corr
        dev->writeRegister("IRIS30", 64, 0); // unreset faros_corr
        dev->writeRegister("IRIS30", 92, 1); // threshold is left-shifted by this many bits

        for (int k = 0; k < 128; k++)
            dev->writeRegister("ARGCOE", k * 4, _cfg->coeffs[k]);

        std::string tddSched = _cfg->clFrames[i];
        for (size_t s = 0; s < _cfg->clFrames[i].size(); s++) {
            char c = _cfg->clFrames[i].at(s);
            if (c == 'B')
                tddSched.replace(s, 1, "G");
            else if (c == 'U')
                tddSched.replace(s, 1, "T");
            else if (c == 'D')
                tddSched.replace(s, 1, "R");
        }
        std::cout << "Client " << i << " schedule: " << tddSched << std::endl;
#ifdef JSON
        json conf;
        conf["tdd_enabled"] = true;
        conf["frame_mode"] = _cfg->frame_mode;
        int max_frame_ = (int)(2.0 / ((_cfg->sampsPerSymbol * _cfg->symbolsPerFrame) / _cfg->rate));
        conf["max_frame"] = max_frame_;
        if (_cfg->clSdrCh == 2)
            conf["dual_pilot"] = true;
        //std::cout << "max_frames for client " << i << " is " << max_frame_ << std::endl;

        conf["frames"] = json::array();
        conf["frames"].push_back(tddSched[i]);
        conf["symbol_size"] = _cfg->sampsPerSymbol;
        std::string confString = conf.dump();
#else
        std::string confString = "{\"tdd_enabled\":true,\"frame_mode\":" + _cfg->frame_mode + ",";
        confString += "\"symbol_size\":" + std::to_string(_cfg->sampsPerSymbol);
        if (_cfg->clSdrCh == 2)
            confString += "\"dual_pilot\":true,";
        confString += ",\"frames\":[\"" + tddSched[i] + "\"]}";
        std::cout << confString << std::endl;
#endif
        dev->writeSetting("TDD_CONFIG", confString);

        dev->setHardwareTime(SoapySDR::ticksToTimeNs((sf_start << 16) | sp_start, _cfg->rate), "TRIGGER");
        dev->writeSetting("TX_SW_DELAY", "30"); // experimentally good value for dev front-end
        dev->writeSetting("TDD_MODE", "true");
        // write beacons to FPGA buffers
        int val = 0;
        for (char const& c : _cfg->bsChannel) {
            std::string tx_ram = "TX_RAM_";
            dev->writeRegisters(tx_ram + c, val, _cfg->beacon);
            val += 2048;
        }
        radios[i]->activateRecv();
        radios[i]->activateXmit();

        if (_cfg->bsChannel != "B") // A or AB
            dev->writeRegister("IRIS30", CORR_CONF, 0x11);
        else
            dev->writeRegister("IRIS30", CORR_CONF, 0x31);
    }
    std::cout << __func__ << " done!" << std::endl;
}

ClientRadioSet::~ClientRadioSet(void)
{
    for (size_t i = 0; i < radios.size(); i++)
        delete radios[i];
}

void ClientRadioSet::radioStop(void)
{
    for (size_t i = 0; i < radios.size(); i++) {
        auto dev = radios[i]->dev;
        dev->writeRegister("IRIS30", CORR_CONF, 0);
        std::cout << "device " << i << " T=" << std::hex << SoapySDR::timeNsToTicks(dev->getHardwareTime(""), _cfg->rate) << std::dec << std::endl;
        for (int i = 0; i < _cfg->symbolsPerFrame; i++) {
            dev->writeRegister("RFCORE", SCH_ADDR_REG, i);
            dev->writeRegister("RFCORE", SCH_MODE_REG, 0);
        }
        dev->writeSetting("TDD_MODE", "false");
        radios[i]->reset_DATA_clk_domain();
        //SoapySDR::Device::unmake(dev);
    }
}

int ClientRadioSet::triggers(int i)
{
    return (radios[i]->getTriggers());
}

int ClientRadioSet::radioRx(size_t radio_id, void* const* buffs, int numSamps, long long& frameTime)
{
    if (radio_id < radios.size()) {
        long long frameTimeNs = 0;
        int ret = radios[radio_id]->recv(buffs, numSamps, frameTimeNs);
        frameTime = frameTimeNs; //SoapySDR::timeNsToTicks(frameTimeNs, _rate);
        return ret;
    }
    std::cout << "invalid radio id " << radio_id << std::endl;
    return 0;
}

int ClientRadioSet::radioTx(size_t radio_id, const void* const* buffs, int numSamps, int flags, long long& frameTime)
{
    return radios[radio_id]->xmit(buffs, numSamps, flags, frameTime);
}

void ClientRadioSet::initAGC(SoapySDR::Device* iclSdr)
{
    /*
     * Initialize AGC parameters
     */
    // OBCH
    int en = (int)_cfg->clAgcEn;
    // AGC Core
    // Enable AGC Flag (set to 0 initially)
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_ENABLE_FLAG, 0);
    // Reset AGC Flag
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_RESET_FLAG, 1);
    // Saturation Threshold: 10300 about -6dBm
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_IQ_THRESH, 8000);
    // Number of samples needed to claim sat.
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_NUM_SAMPS_SAT, 3);
    // Threshold at which AGC stops
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_MAX_NUM_SAMPS_AGC, 10);
    // Gain settle takes about 20 samps(value=20)
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_WAIT_COUNT_THRESH, 20);
    // Drop gain at initial saturation detection
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_BIG_JUMP, 30);
    // Drop gain at subsequent sat. detections
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_SMALL_JUMP, 3);
    // RSSI Target for AGC: ideally around 14 (3.6GHz) or 27 (2.5GHz)
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_RSSI_TARGET, 14);
    // Disable
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_TEST_GAIN_SETTINGS, 0);
    // Clear AGC reset flag
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_RESET_FLAG, 0);
    // Enable AGC
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_ENABLE_FLAG, en);
    // Initialize gains to this value
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_AGC_GAIN_INIT,
        _cfg->clAgcGainInit);

    // Packet Detect Core
    // RSSI value at which Pkt is detected
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_PKT_DET_THRESH, 500);
    // Number of samples needed to detect frame
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_PKT_DET_NUM_SAMPS, 5);
    // Enable packet detection flag
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_PKT_DET_ENABLE, 1);
    // trigger first one if enabled
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_PKT_DET_NEW_FRAME, en);
    // clear
    iclSdr->writeRegister("IRIS30", FPGA_IRIS030_WR_PKT_DET_NEW_FRAME, 0);
}

BaseRadioSet::BaseRadioSet(Config* cfg)
    : _cfg(cfg)
{
    std::vector<int> nBsAntennas(_cfg->nCells);
    bsRadios.resize(_cfg->nCells);

    for (size_t c = 0; c < _cfg->nCells; c++) {
        int radioNum = _cfg->nBsSdrs[c];
        nBsAntennas[c] = radioNum * _cfg->bsChannel.length();
        std::cout << radioNum << " radios in cell " << c << std::endl;
        if (!_cfg->hub_ids.empty()) {
            SoapySDR::Kwargs args;
            args["driver"] = "remote";
            args["timeout"] = "1000000";
            args["serial"] = _cfg->hub_ids.at(c);
            hubs.push_back(SoapySDR::Device::make(args));
        }

        bsRadios[c].resize(radioNum);
        std::atomic_int threadCount = ATOMIC_VAR_INIT(radioNum);
        for (int i = 0; i < radioNum; i++) {
            //args["serial"] = _cfg->bs_sdr_ids[c][i];
            //args["timeout"] = "1000000";
            //bsSdrs[c].push_back(SoapySDR::Device::make(args));
            BaseRadioContext* context = new BaseRadioContext;
            context->brs = this;
            context->threadCount = &threadCount;
            context->tid = i;
            context->cell = c;
#ifdef THREADED_INIT
            pthread_t init_thread_;
            if (pthread_create(&init_thread_, NULL, BaseRadioSet::init_launch, context) != 0) {
                perror("init thread create failed");
                exit(0);
            }
#else
            init(context);
#endif
        }

        while (threadCount > 0)
            ;

        // Strip out broken radios.
        for (int i = 0; i < radioNum; i++) {
            if (bsRadios[c][i] == NULL) {
                while (radioNum != 0 && bsRadios[c][radioNum - 1] == NULL) {
                    --radioNum;
                    bsRadios[c].pop_back();
                }
                if (i < radioNum) {
                    bsRadios[c][i] = bsRadios[c][--radioNum];
                    bsRadios[c].pop_back();
                }
            }
        }
        bsRadios[c].shrink_to_fit();
        _cfg->nBsSdrs[c] = radioNum;

        // Perform DC Offset & IQ Imbalance Calibration
        if (_cfg->imbalanceCalEn) {
            if (_cfg->bsChannel.find('A') != std::string::npos)
                dciqCalibrationProc(0);
            if (_cfg->bsChannel.find('B') != std::string::npos)
                dciqCalibrationProc(1);
        }

        threadCount = radioNum;
        for (int i = 0; i < radioNum; i++) {
            BaseRadioContext* context = new BaseRadioContext;
            context->brs = this;
            context->threadCount = &threadCount;
            context->tid = i;
            context->cell = c;
#ifdef THREADED_INIT
            pthread_t configure_thread_;
            if (pthread_create(&configure_thread_, NULL, BaseRadioSet::configure_launch, context) != 0) {
                perror("configure thread create failed");
                exit(0);
            }
#else
            configure(context);
#endif
        }

        while (threadCount > 0)
            ;
        // Measure Sync Delays now!
        sync_delays(c);
    }

    if (_cfg->sampleCalEn) {
        bool adjust = false;
        int cal_cnt = 0;
        while (!adjust) {
            if (++cal_cnt > 10) {
                std::cout << "10 attemps of sample offset calibration, stopping..." << std::endl;
                break;
            }
            adjust = true;
            collectCSI(adjust); // run 1: find offsets and adjust
        }
        collectCSI(adjust); // run 2: verify adjustments
        usleep(100000);
    }

    std::vector<std::string> _tddSched;
    _tddSched.resize(_cfg->frames.size());
    for (unsigned int f = 0; f < _cfg->frames.size(); f++) {
        _tddSched[f] = _cfg->frames[f];
        for (size_t s = 0; s < _cfg->frames[f].size(); s++) {
            char c = _cfg->frames[f].at(s);
            if (c == 'B')
                _tddSched[f].replace(s, 1, "P");
            else if (c == 'P')
                _tddSched[f].replace(s, 1, "R");
            else if (c == 'U')
                _tddSched[f].replace(s, 1, "R");
            else if (c == 'D')
                _tddSched[f].replace(s, 1, "T");
        }
        std::cout << _tddSched[f] << std::endl;
    }

#ifdef JSON
    json conf;
    conf["tdd_enabled"] = true;
    conf["frame_mode"] = "free_running";
    conf["max_frame"] = _cfg->max_frame;
    conf["frames"] = _tddSched;
    conf["symbol_size"] = _cfg->sampsPerSymbol;
    std::string confString = conf.dump();
#else
    std::string confString = "{\"tdd_enabled\":true,\"frame_mode\":\"free_running\",";
    confString += "\"symbol_size\":" + std::to_string(_cfg->sampsPerSymbol);
    confString += ",\"frames\":[";
    for (int f = 0; f < _cfg->frames.size(); f++)
        confString += (f == _cfg->frames.size() - 1) ? "\"" + _tddSched[f] + "\"" : "\"" + _tddSched[f] + "\",";
    confString += "]}";
    std::cout << confString << std::endl;
#endif
    for (size_t i = 0; i < bsRadios[0].size(); i++) {
        SoapySDR::Device* dev = bsRadios[0][i]->dev;
        dev->writeSetting("TX_SW_DELAY", "30"); // experimentally good value for dev front-end
        dev->writeSetting("TDD_MODE", "true");
        dev->writeSetting("TDD_CONFIG", confString);
    }

    // write beacons to FPGA buffers
    if (!_cfg->beamsweep or nBsAntennas[0] == 1) {
        std::vector<unsigned> zeros(_cfg->sampsPerSymbol, 0);
        size_t ndx = 0;
        for (size_t i = 0; i < bsRadios[0].size(); i++) {
            SoapySDR::Device* dev = bsRadios[0][i]->dev;
            for (char const& c : _cfg->bsChannel) {
                std::string tx_ram = "TX_RAM_";
                std::vector<unsigned>& msg = ndx == _cfg->beacon_ant ? _cfg->beacon : zeros;
                dev->writeRegisters(tx_ram + c, 0, msg);
                ++ndx;
            }
            dev->writeRegister("RFCORE", 156, 0);
        }
    } else { // beamsweep
        std::vector<unsigned> beacon_weights(nBsAntennas[0]);
        size_t ndx = 0;
        for (size_t i = 0; i < bsRadios[0].size(); i++) {
            SoapySDR::Device* dev = bsRadios[0][i]->dev;
            for (char const& c : _cfg->bsChannel) {
                std::string tx_ram = "TX_RAM_";
                dev->writeRegisters(tx_ram + c, 0, _cfg->beacon);
                std::string tx_ram_wgt = "TX_RAM_WGT_";
                for (int j = 0; j < nBsAntennas[0]; j++)
                    beacon_weights[j] = CommsLib::hadamard2(ndx, j);
                dev->writeRegisters(tx_ram_wgt + c, 0, beacon_weights);
                ++ndx;
            }
            dev->writeRegister("RFCORE", 156, bsRadios[0].size());
            dev->writeRegister("RFCORE", 160, 1);
        }
    }

    for (size_t i = 0; i < bsRadios[0].size(); i++) {
        SoapySDR::Device* dev = bsRadios[0][i]->dev;
        bsRadios[0][i]->activateRecv();
        bsRadios[0][i]->activateXmit();
        dev->setHardwareTime(0, "TRIGGER");
    }
    std::cout << __func__ << " done!" << std::endl;
}

BaseRadioSet::~BaseRadioSet(void)
{
    if (!_cfg->hub_ids.empty()) {
        for (unsigned int i = 0; i < hubs.size(); i++)
            SoapySDR::Device::unmake(hubs[i]);
    }
    for (unsigned int c = 0; c < _cfg->nCells; c++)
        for (size_t i = 0; i < _cfg->nBsSdrs[c]; i++)
            delete bsRadios[c][i];
}

void* BaseRadioSet::init_launch(void* in_context)
{
    BaseRadioContext* context = (BaseRadioContext*)in_context;
    BaseRadioSet* brs = context->brs;
    brs->init(context);
    return 0;
}

void BaseRadioSet::init(BaseRadioContext* context)
{
    int i = context->tid;
    int c = context->cell;
    std::atomic_int* threadCount = context->threadCount;
    delete context;

    std::vector<size_t> channels;
    if (_cfg->bsChannel == "A")
        channels = { 0 };
    else if (_cfg->bsChannel == "B")
        channels = { 1 };
    else
        channels = { 0, 1 };

    SoapySDR::Kwargs args;
    args["driver"] = "iris";
    args["timeout"] = "1000000";
    args["serial"] = _cfg->bs_sdr_ids[c][i];
    try {
        bsRadios[c][i] = new Radio(args, SOAPY_SDR_CS16, channels, _cfg->rate);
    } catch (std::runtime_error) {
        std::cerr << "Ignoring iris " << _cfg->bs_sdr_ids[c][i] << std::endl;
        bsRadios[c][i] = NULL;
    }

    (*threadCount)--;
}

void* BaseRadioSet::configure_launch(void* in_context)
{
    BaseRadioContext* context = (BaseRadioContext*)in_context;
    BaseRadioSet* brs = context->brs;
    brs->configure(context);
    return 0;
}

void BaseRadioSet::configure(BaseRadioContext* context)
{
    int i = context->tid;
    int c = context->cell;
    std::atomic_int* threadCount = context->threadCount;
    delete context;

    //load channels
    std::vector<size_t> channels;
    if (_cfg->bsChannel == "A")
        channels = { 0 };
    else if (_cfg->bsChannel == "B")
        channels = { 1 };
    else
        channels = { 0, 1 };

    Radio* bsRadio = bsRadios[c][i];
    SoapySDR::Device* dev = bsRadio->dev;
    SoapySDR::Kwargs info = dev->getHardwareInfo();
    for (auto ch : channels) {
        double rxgain = _cfg->rxgain[ch];
        double txgain = _cfg->txgain[ch];
        bsRadios[c][i]->dev_init(_cfg, ch, rxgain, txgain, info["frontend"]);
    }

    (*threadCount)--;
}

SoapySDR::Device* BaseRadioSet::baseRadio(size_t cellId)
{
    if (cellId < hubs.size())
        return (hubs[cellId]);
    if (cellId < bsRadios.size() && bsRadios[cellId].size() > 0)
        return bsRadios[cellId][0]->dev;
    return NULL;
}

void BaseRadioSet::sync_delays(int cellIdx)
{
    /*
     * Compute Sync Delays
     */
    SoapySDR::Device* base = baseRadio(cellIdx);
    if (base != NULL)
        base->writeSetting("SYNC_DELAYS", "");
}

void BaseRadioSet::radioTrigger(void)
{
    for (size_t c = 0; c < _cfg->nCells; c++) {
        SoapySDR::Device* base = baseRadio(c);
        if (base != NULL)
            base->writeSetting("TRIGGER_GEN", "");
    }
}

void BaseRadioSet::radioStart()
{
    radioTrigger();
}

void BaseRadioSet::readSensors()
{
    for (size_t i = 0; i < bsRadios[0].size(); i++) {
        SoapySDR::Device* dev = bsRadios[0][i]->dev;
        std::cout << "TEMPs on Iris " << i << std::endl;
        std::cout << "ZYNQ_TEMP: " << dev->readSensor("ZYNQ_TEMP") << std::endl;
        std::cout << "LMS7_TEMP  : " << dev->readSensor("LMS7_TEMP") << std::endl;
        std::cout << "FE_TEMP  : " << dev->readSensor("FE_TEMP") << std::endl;
        std::cout << "TX0 TEMP  : " << dev->readSensor(SOAPY_SDR_TX, 0, "TEMP") << std::endl;
        std::cout << "TX1 TEMP  : " << dev->readSensor(SOAPY_SDR_TX, 1, "TEMP") << std::endl;
        std::cout << "RX0 TEMP  : " << dev->readSensor(SOAPY_SDR_RX, 0, "TEMP") << std::endl;
        std::cout << "RX1 TEMP  : " << dev->readSensor(SOAPY_SDR_RX, 1, "TEMP") << std::endl;
        std::cout << std::endl;
    }
}

void BaseRadioSet::radioStop(void)
{
    for (size_t i = 0; i < bsRadios[0].size(); i++) {
        SoapySDR::Device* dev = bsRadios[0][i]->dev;
        // write schedule
        for (unsigned int j = 0; j < _cfg->frames.size(); j++) {
            for (int k = 0; k < _cfg->symbolsPerFrame; k++) // symnum <= 256
            {
                dev->writeRegister("RFCORE", SCH_ADDR_REG, j * 256 + k);
                dev->writeRegister("RFCORE", SCH_MODE_REG, 0);
            }
        }
        dev->writeSetting("TDD_MODE", "false");
        bsRadios[0][i]->reset_DATA_clk_domain();
    }
}

void BaseRadioSet::radioTx(const void* const* buffs)
{
    long long frameTime(0);
    for (size_t i = 0; i < bsRadios[0].size(); i++) {
        bsRadios[0][i]->xmit(buffs, _cfg->sampsPerSymbol, 0, frameTime);
    }
}

int BaseRadioSet::radioTx(size_t radio_id, const void* const* buffs, int flags, long long& frameTime)
{
    int w = bsRadios[0][radio_id]->xmit(buffs, _cfg->sampsPerSymbol, flags, frameTime);
#if DEBUG_RADIO
    size_t chanMask;
    long timeoutUs(0);
    int s = dev->readStreamStatus(bsRadio->txs, chanMask, flags, frameTime, timeoutUs);
    std::cout << "radio " << radio_id << " tx returned " << w << " and status " << s << std::endl;
#endif
    return w;
}

void BaseRadioSet::radioRx(void* const* buffs)
{
    long long frameTime(0);
    for (size_t i = 0; i < bsRadios[0].size(); i++) {
        void* const* buff = buffs + (i * 2);
        bsRadios[0][i]->recv(buff, _cfg->sampsPerSymbol, frameTime);
    }
}

int BaseRadioSet::radioRx(size_t radio_id, void* const* buffs, long long& frameTime)
{
    if (radio_id < bsRadios[0].size()) {
        long long frameTimeNs = 0;

        int ret = bsRadios[0][radio_id]->recv(buffs, _cfg->sampsPerSymbol, frameTimeNs);
        frameTime = frameTimeNs; //SoapySDR::timeNsToTicks(frameTimeNs, _rate);
#if DEBUG_RADIO
        if (ret != _cfg->sampsPerSymbol)
            std::cout << "recv returned " << ret << " from radio " << radio_id << ", Expected "
                      << _cfg->sampsPerSymbol << std::endl;
        else
            std::cout << "radio " << radio_id << "received " << ret << std::endl;
#endif
        return ret;
    }
    std::cout << "invalid radio id " << radio_id << std::endl;
    return 0;
}
