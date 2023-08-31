#pragma once

#ifdef WIN32
    #define _WIN32_WINNT 0x0601
#endif


#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>

//#include <asio/impl/src.hpp>

#include "asio.hpp"
#include "asio/buffer.hpp"


enum DeviceType : uint32_t
{
    DONGLE_DEVTYPE_CAPNO_GO = 0x01,
    DONGLE_DEVTYPE_EMG = 0x02,
    DONGLE_DEVTYPE_HRV = 0x03,
    DONGLE_DEVTYPE_CAPNO_6 = 0x04,

    DONGLE_INVALID_CONN_HANDLE = 0xFF,
};

enum DataType : uint8_t
{
    DATA_CO2,
    DATA_ETCO2_AVERAGE,
    DATA_INSP_CO2_AVERAGE,
    DATA_BPM_AVERAGE,

    DATA_RR_INTERVALS,
    DATA_HEART_RATE,
    DATA_EMG,

    DATA_CAPNO_BATTERY,
    DATA_BAROMETRIC_PRESSURE,
    DATA_GAS_TEMPERATURE,
    DATA_CAPNO_STATUS,
};