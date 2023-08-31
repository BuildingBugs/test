

#include "capnotrainer_go.h"
#include <iomanip> // required for std::fixed and std::setprecision


CapnoTrainerGo::CapnoTrainerGo(uint8_t conn_handle, double current_time) :
    connection_handle(conn_handle),
    last_time(current_time)
{

}

void CapnoTrainerGo::handle_incoming_data(std::vector<uint8_t> &data, double current_time)
{

    current_buffer.clear();

    if (last_buffer == data) {
        // std::cout << "Found duplicate" << std::endl;
        return;
    }

    // current_buffer = data;
    current_buffer.insert(current_buffer.end(), last_buffer_remains.begin(), last_buffer_remains.end());
    current_buffer.insert(current_buffer.end(), data.begin(), data.end());

    for (size_t i = 0; i < current_buffer.size(); i++)
    {
        bool is_cmd_byte = current_buffer.at(i) >= 128;
        if (is_cmd_byte)
        {
            if (i < current_buffer.size() - 1)
            {
                uint8_t cmd_size = current_buffer.at(i + 1) + 2;
                size_t remaining_buffer_size = current_buffer.size() - i;

                if (cmd_size < remaining_buffer_size)
                {
                    std::vector<uint8_t> cmd_data = std::vector<uint8_t>(current_buffer.begin() + i, current_buffer.begin() + i + cmd_size);
                    if (handle_check_checksum(cmd_data) && handle_non_cmd_byte_check(data))
                    {
                        handle_cmd_byte_parsing(cmd_data);
                        
                        if (cmd_data.at(0) !=  0x80)
                        {
                            /*std::cout << "Reception (cmd): ";
                            for (const auto& c : cmd_data) {
                                std::cout << std::hex << (int)c << "  ";
                            }
                            std::cout << std::endl;*/
                        }
                    }
                }
                else
                {
                    last_buffer_remains.clear();
                    last_buffer_remains = std::vector<uint8_t>(current_buffer.begin() + i, current_buffer.end());
                }
            }
            else
            {
                last_buffer_remains.clear();
                last_buffer_remains = std::vector<uint8_t>(current_buffer.begin() + i, current_buffer.end());
            }
        }
    }

    last_buffer = data;
    last_time = current_time;
}

void CapnoTrainerGo::handle_cmd_byte_parsing(std::vector<uint8_t> &data)
{
    uint8_t cmd = data.at(0);

    switch (cmd)
    {
    case 0x80:
    {
        handle_set_start_stream_res(data);
    }break;

    case 0xC9:
    {

    }break;

    case 0x84:
    {
        uint8_t isb_type = data.at(2);
        switch (isb_type)
        {
        case 0x00:
        {

        } break;

        case 0x01:
        {
            handle_get_barometric_pressure_res(data);
        } break;

        case 0x04:
        {
            handle_get_gas_temperature_res(data);
        } break;

        case 0x05:
        {
            handle_set_etco2_time_period_res(data);
        } break;

        default:
        {

        }
        }
    }break;

    case 0xFB:
    {
        uint8_t info_type = data.at(2);
        switch (info_type)
        {
        case 0x07:
        {
            handle_get_battery_status_res(data);
        }break;

        default:
        {

        }
        }
    }

    case 0xC8:
    {

    } break;

    default:
    {
    }break;
    }
}

bool CapnoTrainerGo::handle_non_cmd_byte_check(std::vector<uint8_t> &buffer)
{
    bool cmd_check = true;
    for (size_t i = 1; i < buffer.size(); i++)
    {
        cmd_check = buffer.at(i) < 0x80;
        if (cmd_check) { break; }
    }
    return cmd_check;
}

bool CapnoTrainerGo::handle_check_checksum(std::vector<uint8_t> &buffer)
{

    uint8_t checksum = 0x00;

    for (size_t i = 0; i < buffer.size(); i++)
    {
        if (buffer.at(i) >= 0x80 && i != 0) {
            return false;
        }

        checksum = checksum + buffer.at(i);
    }

    checksum &= 0x7F;
    return checksum == 0x00;
}

std::vector<uint8_t> CapnoTrainerGo::handle_add_checksum(std::vector<uint8_t>& buffer)
{
    uint8_t checksum = 0x00;
    uint8_t readsize = buffer.at(1) + 1; 
    for (size_t i = 0; i < readsize; i++)
    {
        checksum += buffer.at(i);
    }

    checksum = (~checksum + 1) & 0x7F; 
    buffer.insert(buffer.begin() + readsize, checksum);
    return buffer;
}




std::vector<uint8_t> CapnoTrainerGo::handle_set_stop_stream_req()
{
    std::vector<uint8_t> data = {0xC9, 0x01, 0x00};
    data = this->handle_add_checksum(data);
    return data;
}

void CapnoTrainerGo::handle_set_stop_stream_res(std::vector<uint8_t> &buffer)
{
    this->_has_received_stop_cmd = true;
}

std::vector<uint8_t> CapnoTrainerGo::handle_set_start_stream_req()
{
    std::vector<uint8_t> data = { 0x80, 0x02, 0x00,0x00 };
    data = this->handle_add_checksum(data);
    return data;
}

void CapnoTrainerGo::handle_set_start_stream_res(std::vector<uint8_t> &buffer)
{

    this->_has_received_start_cmd = true;

    uint8_t nbf = buffer.at(1);
    uint8_t sync = buffer.at(2);
    uint8_t co2wb1 = buffer.at(3);
    uint8_t co2wb2 = buffer.at(4);

    float co2 = static_cast<float> ((128 * static_cast<float> (co2wb1) + static_cast<float>(co2wb2) ) - 1000 ) / 100;
    if (co2 < 0.0) { co2 = 0; }

    // std::cout << "Sync | CO2: " << (int) sync << "  " << co2 << std::endl;

    co2_array.push_back(co2);

    if (nbf > 6)
    {
        /*for (const auto& c : buffer) {
            std::cout << std::hex << (int)c << "  ";
        }
        std::cout << std::endl; */

        uint8_t dpi = buffer.at(5);
        switch (dpi)
        {
            case 2: // ETCO2  
            {
                uint16_t etco2 = (128 * static_cast<uint16_t> (buffer.at(6)) + static_cast<uint16_t>(buffer.at(7)));
                etco2_array.push_back((float)etco2 / 10.0);
            } break;
            
            case 3: // RESP Rate
            {
                uint16_t bpm = (128 * static_cast<uint16_t> (buffer.at(6)) + static_cast<uint16_t>(buffer.at(7)));
                bpm_array.push_back((float)bpm);
            } break;

            case 4: // Inspiratory CO2
            {
                uint16_t inspco2 = (128 * static_cast<uint16_t> (buffer.at(6)) + static_cast<uint16_t>(buffer.at(7)));
                inspco2_array.push_back((float)inspco2/10.0);
            } break;

            default:
            {
            }break;
        }
    }
}

std::vector<uint8_t> CapnoTrainerGo::handle_get_barometric_pressure_req()
{
    std::vector<uint8_t> data = { 0x84, 0x02, 0x01,0x00 };
    data = this->handle_add_checksum(data);
    return data;
}

void CapnoTrainerGo::handle_get_barometric_pressure_res(std::vector<uint8_t> &buffer)
{
    float barometric_pressure = (float) buffer.at(3) * 128 + (float) buffer.at(4); 
    std::cout << "Barometric Pressure: " << barometric_pressure << std::endl;
    this->_has_received_pressure = true;

}

std::vector<uint8_t> CapnoTrainerGo::handle_get_gas_temperature_req()
{
    std::vector<uint8_t> data = { 0x84, 0x02, 0x04,0x00 };
    data = this->handle_add_checksum(data);
    return data;
}

void CapnoTrainerGo::handle_get_gas_temperature_res(std::vector<uint8_t> &buffer)
{
    float gas_temperature = ((float)buffer.at(3) * 128 + (float)buffer.at(4)) / 2;
    std::cout << "Gas Temperature: " << gas_temperature << std::endl;
    this->_has_received_temperature = true;
}

std::vector<uint8_t> CapnoTrainerGo::handle_set_etco2_time_period_req()
{
    // setting ETCO2 time period to 20 seconds 
    std::vector<uint8_t> data = { 0x84, 0x03, 0x05, 0x14, 0x00 };
    data = this->handle_add_checksum(data);
    return data;
}

void CapnoTrainerGo::handle_set_etco2_time_period_res(std::vector<uint8_t>& buffer)
{
    float etco2_time_period = (float)buffer.at(3);
    std::cout << "ETCO2 Time Period: " << etco2_time_period << std::endl;
}

std::vector<uint8_t> CapnoTrainerGo::handle_get_battery_status_req()
{
    std::vector<uint8_t> data = { 0xFB, 0x02, 0x07, 0x00 };
    data = this->handle_add_checksum(data);
    return data;
}

void CapnoTrainerGo::handle_get_battery_status_res(std::vector<uint8_t> &buffer)
{
    std::string battery_status = buffer.at(3) == 0x00 ? "Normal Status" : "Abnormal Status";
    std::string battery_charging = buffer.at(7) == 0x01 ? "Charging" : "Not Charging";
    float battery_volts = (float)((buffer.at(4) & 0x7F) << 0x07) + ((float)buffer.at(5));
    float battery_percent = (float)(buffer.at(6));

    this->_has_received_battery = true;

    battery_array.push_back(battery_percent);
    std::cout << "Battery Status: " << battery_status << "\tBattery Charging: "  << battery_charging << "\tBattery Percentage: " << battery_percent << std::endl;
}
