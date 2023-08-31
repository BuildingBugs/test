#pragma once

#include "commons.h"


class CapnoTrainerHrv
{
    
public:
    CapnoTrainerHrv(uint8_t conn_handle, double current_time);

    std::string device_name = "";
    uint8_t connection_handle = DONGLE_INVALID_CONN_HANDLE;
    double last_time;
    void handle_incoming_data(std::vector<uint8_t>& data, double current_time);
    std::vector<float> rr_array;
    std::vector<float> hr_array;

private:

};

