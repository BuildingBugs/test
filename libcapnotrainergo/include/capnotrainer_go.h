#pragma once


#include "commons.h"

class CapnoTrainerGo
{
public:
    CapnoTrainerGo(uint8_t conn_handle, double current_time);

    std::string device_name = "";
    uint8_t connection_handle = DONGLE_INVALID_CONN_HANDLE;

    std::vector<float> co2_array;
    std::vector<float> battery_array;
    std::vector<float> etco2_array;
    std::vector<float> inspco2_array;
    std::vector<float> bpm_array;
    double last_time;

    bool _has_received_pressure = false;
    bool _has_received_temperature = false;
    bool _has_received_battery = false;
    bool _has_received_stop_cmd = false;
    bool _has_received_start_cmd = false;


    std::vector<uint8_t> current_buffer;
    std::vector<uint8_t> last_buffer;
    std::vector<uint8_t> last_buffer_remains;

    void handle_incoming_data(std::vector<uint8_t>& data, double current_time);
    void handle_cmd_byte_parsing(std::vector<uint8_t>& data);


    bool handle_non_cmd_byte_check(std::vector<uint8_t> &buffer);
    bool handle_check_checksum(std::vector<uint8_t>& buffer);
    std::vector<uint8_t> handle_add_checksum(std::vector<uint8_t>& buffer);


    std::vector<uint8_t> handle_set_stop_stream_req();
    void handle_set_stop_stream_res(std::vector<uint8_t>& buffer);

    std::vector<uint8_t> handle_set_start_stream_req();
    void handle_set_start_stream_res(std::vector<uint8_t>& buffer);

    std::vector<uint8_t> handle_get_barometric_pressure_req();
    void handle_get_barometric_pressure_res(std::vector<uint8_t>& buffer);

    std::vector<uint8_t> handle_get_gas_temperature_req();
    void handle_get_gas_temperature_res(std::vector<uint8_t>& buffer);

    std::vector<uint8_t> handle_set_etco2_time_period_req();
    void handle_set_etco2_time_period_res(std::vector<uint8_t>& buffer);

    std::vector<uint8_t> handle_get_battery_status_req();
    void handle_get_battery_status_res(std::vector<uint8_t>& buffer);

};