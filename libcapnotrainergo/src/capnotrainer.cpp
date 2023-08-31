

#include "capnotrainer.h"
#include "capnotrainer_go.h"

CapnoTrainer::CapnoTrainer(user_cb_t user_cb, bool debug) :
    io(),
    work_guard(asio::make_work_guard(io)),
    user_cb(user_cb),
    debug(debug),
    serial_port_1(io),
    serial_port_2(io)
{
    io_thread = std::thread([this]() { io.run(); });
}


CapnoTrainer::~CapnoTrainer()
{
    Disconnect();
    work_guard.reset(); // Reset the work guard to allow io.run() to exit
    io_thread.join();
    //io.stop();
}

void CapnoTrainer::Connect(const char* port1, const char* port2)
{
    if (!serial_port_1.is_open()) {
        serial_port_1 = asio::serial_port(io, port1);
        serial_port_1.set_option(asio::serial_port_base::baud_rate(115200));
        serial_port_1.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    }

    if (!serial_port_2.is_open()) {
        serial_port_2 = asio::serial_port(io, port2);
        serial_port_2.set_option(asio::serial_port_base::baud_rate(115200));
        serial_port_2.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    }

    is_connected = true;
    Read();
}

void CapnoTrainer::Disconnect()
{
    is_connected = false;
    
    if (serial_port_1.is_open()) {
        serial_port_1.cancel();
        serial_port_1.close();
        if (debug) { std::cout << "Closing port 1" << std::endl; }
    }

    if (serial_port_2.is_open()) {
        serial_port_2.cancel();
        serial_port_2.close();
        if (debug) { std::cout << "Closing port 2" << std::endl; }
    }

    HandleCleanup();
}

bool CapnoTrainer::isConnected()
{
    return is_connected;
}


void CapnoTrainer::Initialize()
{
 /*   if (serial_port_1.is_open() && serial_port_2.is_open())
    {
        Read();
    }*/
}

void CapnoTrainer::HandleCleanup()
{
    device_handles.clear();
    go_devices.clear();
    hrv_devices.clear();
    emg_devices.clear();
}

void CapnoTrainer::Read()
{   
    if (is_connected)
    {
        ReadFromPort1();
        ReadFromPort2();
    }
    else
    { 
        std::chrono::milliseconds delay(100);
        io.post([this, delay]() {
            std::this_thread::sleep_for(delay);
            Read();
        });
    }
}

void CapnoTrainer::ReadFromPort1()
{
    if (serial_port_1.is_open())
    {
        serial_port_1.async_read_some(
            asio::buffer(read_buffer_1, CAPNOTRAINER_SERIAL_READ_SIZE),
            [this](const asio::error_code& ec, std::size_t bytes_transfered)
        {
            if (!ec) {
                if (bytes_transfered > 0)
                {
                    HandleParser(read_buffer_1);
                }
                ReadFromPort1();
            }
        });
    }
}

void CapnoTrainer::ReadFromPort2()
{
    if (serial_port_2.is_open())
    {
        serial_port_2.async_read_some(
            asio::buffer(read_buffer_2, CAPNOTRAINER_SERIAL_READ_SIZE),
            [this](const asio::error_code& ec, std::size_t bytes_transfered)
        {
            if (!ec) {
                if (bytes_transfered > 0)
                {
                    HandleParser(read_buffer_2);
                }
                ReadFromPort2();
            }
        });
    }
}

void CapnoTrainer::Write(std::vector<uint8_t>& buffer, uint8_t cmd_type)
{
    if (serial_port_1.is_open() && serial_port_2.is_open() )
    {
        std::vector<uint8_t> write_buffer;
        //(CAPNOTRAINER_SERIAL_WRITE_SIZE, 0x00);

        write_buffer.push_back(cmd_type);
        //write_buffer.insert(write_buffer.begin(), DONGLE_DEVTYPE_CAPNO_GO);

        for (size_t i = 0; i < buffer.size(); i++)
        {
            write_buffer.push_back(buffer.at(i));
            //write_buffer.insert(write_buffer.begin() + i + 1, buffer.at(i) );
        }

        if (write_buffer.size() != CAPNOTRAINER_SERIAL_WRITE_SIZE)
        {
            for (size_t i = write_buffer.size(); i < (CAPNOTRAINER_SERIAL_WRITE_SIZE - write_buffer.size()); i++)
            {
                write_buffer.push_back(0x00);
            }
        }

       asio::error_code ec; 
       int status = serial_port_1.write_some( asio::buffer(write_buffer), ec);
       // std::cout << "Write status: " << ec.value() << "\tec: " << ec.message().c_str() << std::endl;
    }
    return;
}


bool CapnoTrainer::CheckGoDevice(std::vector<CapnoTrainerGo>::iterator &go_device)
{
    if (!go_device->_has_received_pressure)
    {
        std::vector<uint8_t> write_data = go_device->handle_get_barometric_pressure_req();
        Write(write_data, DONGLE_DEVTYPE_CAPNO_GO);
    }

    if (!go_device->_has_received_temperature)
    {
        std::vector<uint8_t> write_data = go_device->handle_get_gas_temperature_req();
        Write(write_data, DONGLE_DEVTYPE_CAPNO_GO);
    }

    if (!go_device->_has_received_battery)
    {
        std::vector<uint8_t> write_data_r = go_device->handle_set_etco2_time_period_req();
        Write(write_data_r, DONGLE_DEVTYPE_CAPNO_GO);

        std::vector<uint8_t> write_data = go_device->handle_get_battery_status_req();
        Write(write_data, DONGLE_DEVTYPE_CAPNO_GO);
    }

    if (!go_device->_has_received_start_cmd)
    {
        std::vector<uint8_t> write_data = go_device->handle_set_start_stream_req();
        Write(write_data, DONGLE_DEVTYPE_CAPNO_GO);
    }

    if (go_device->device_name.size() == 0)
    {
        // send device name request.
        std::vector<uint8_t> write_name_data = { 0xdd, go_device->connection_handle };
        Write(write_name_data, 0xff);
    }
        
    return (go_device->_has_received_pressure && go_device->_has_received_temperature &&
        go_device->_has_received_start_cmd && go_device->_has_received_battery );
            
}

void CapnoTrainer::HandleParser(uint8_t * read_buffer)
{
    uint8_t device_type = read_buffer[0];
    uint8_t connection_handle = read_buffer[1];

    switch (device_type)
    {
        case DONGLE_DEVTYPE_CAPNO_6:
        {

        } break;

        case DONGLE_DEVTYPE_CAPNO_GO:
        {
            HandleGoParser(read_buffer);
        } break;

        case DONGLE_DEVTYPE_HRV:
        {
            std::vector<uint8_t> data(&read_buffer[0], &read_buffer[CAPNOTRAINER_SERIAL_READ_SIZE]);
            HandleHrvParser(data);
            HandleEmgParser(data);
        } break;

        case DONGLE_DEVTYPE_EMG:
        {
            std::vector<uint8_t> data(&read_buffer[0], &read_buffer[CAPNOTRAINER_SERIAL_READ_SIZE]);
            HandleHrvParser(data);
            HandleEmgParser(data);
        } break;

        case DONGLE_INVALID_CONN_HANDLE:
        {
            HandleNameParser(read_buffer);
        } break;

    default:
    {

    }
    }
}


void CapnoTrainer::HandleGoParser(uint8_t *read_buffer)
{
    uint8_t device_type = read_buffer[0];
    uint8_t connection_handle = read_buffer[1];
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    double duration = (double) value.count();

    // read the data part
    std::vector<uint8_t> data(&read_buffer[4], &read_buffer[CAPNOTRAINER_SERIAL_READ_SIZE]);

    // find the go device then
    std::vector<CapnoTrainerGo>::iterator go_device = std::find_if(
        go_devices.begin(), go_devices.end(),
        [&connection_handle](const auto& go) {
        return go.connection_handle == connection_handle && go.connection_handle != DONGLE_INVALID_CONN_HANDLE;
    });
    
    if (go_device != go_devices.end())
    {
        if ((duration - go_device->last_time) < device_max_connection_time)
        {
            go_device->handle_incoming_data(data, duration);
            CheckGoDevice(go_device);
            /*if (CheckGoDevice(go_device))
            {*/
            if (go_device->co2_array.size() > 0)
            {
                user_cb(go_device->co2_array, DONGLE_DEVTYPE_CAPNO_GO, connection_handle, DATA_CO2);
                go_device->co2_array.clear();
            }
            if (go_device->battery_array.size() > 0)
            {
                user_cb(go_device->battery_array, DONGLE_DEVTYPE_CAPNO_GO, connection_handle, DATA_CAPNO_BATTERY);
                go_device->battery_array.clear();
            }
            if (go_device->bpm_array.size() > 0)
            {
                user_cb(go_device->bpm_array, DONGLE_DEVTYPE_CAPNO_GO, connection_handle, DATA_BPM_AVERAGE);
                go_device->bpm_array.clear();
            }
            if (go_device->etco2_array.size() > 0)
            {
                user_cb(go_device->etco2_array, DONGLE_DEVTYPE_CAPNO_GO, connection_handle, DATA_ETCO2_AVERAGE);
                go_device->etco2_array.clear();
            }
            if (go_device->inspco2_array.size() > 0)
            {
                user_cb(go_device->inspco2_array, DONGLE_DEVTYPE_CAPNO_GO, connection_handle, DATA_INSP_CO2_AVERAGE);
                go_device->inspco2_array.clear();
            }
            //}
        }
        else 
        {
            if (debug) std::cout << "Removing "  << go_device->device_name << "  from the device list and reinitializing it again" << std::endl;
            device_handles.erase(std::remove(device_handles.begin(), device_handles.end(), go_device->connection_handle), device_handles.end());
            go_devices.erase(go_device);
        }
            
    } 
    else
    {
        CapnoTrainerGo go_device(connection_handle, duration);
        go_devices.push_back(go_device);
        device_handles.push_back(connection_handle);
        std::vector<CapnoTrainerGo>::iterator last_go_device = go_devices.end() - 1;
        CheckGoDevice(last_go_device);
    }
}

void CapnoTrainer::HandleHrvParser(std::vector<uint8_t> &data)
{
    uint8_t hrv_offset = 43;
    uint8_t hrv_container_length = 7;
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    double duration = (double) value.count();

    for (size_t i = 1; i < hrv_offset; i += hrv_container_length)
    {
        if (i + hrv_container_length >= data.size())
        {
            break;
        }

        std::vector<uint8_t> sub_data(data.begin() + i, data.begin() + i + hrv_container_length);
        if (sub_data.size() == hrv_container_length)
        {
            uint8_t hrv_conn_handle = sub_data.at(0);
            if (hrv_conn_handle != DONGLE_INVALID_CONN_HANDLE)
            {
                // find the hrv device then
                std::vector<CapnoTrainerHrv>::iterator hrv_device = std::find_if(
                    hrv_devices.begin(), hrv_devices.end(),
                    [&hrv_conn_handle](const auto& hrv) {
                    return hrv.connection_handle == hrv_conn_handle;
                });

                if (hrv_device != hrv_devices.end())
                {
                    if ( (duration - hrv_device->last_time) < device_max_connection_time)
                    {
                        hrv_device->handle_incoming_data(sub_data, duration);
                        
                        if (hrv_device->rr_array.size() > 0)
                        {
                            user_cb(hrv_device->rr_array, DONGLE_DEVTYPE_HRV, hrv_device->connection_handle, DATA_RR_INTERVALS);
                            hrv_device->rr_array.clear();
                        }

                        if (hrv_device->hr_array.size() > 0)
                        {
                            user_cb(hrv_device->hr_array, DONGLE_DEVTYPE_HRV, hrv_device->connection_handle, DATA_HEART_RATE);
                            hrv_device->hr_array.clear();
                        }

                        if (hrv_device->device_name.size() == 0)
                        {
                            // send device name request.
                            std::vector<uint8_t> write_name_data = { 0xdd, hrv_device->connection_handle };
                            Write(write_name_data, 0xff);
                        }
                    }
                    else
                    {
                        if (debug) std::cout << "Removing " << hrv_device->device_name << "  from the device list and reinitializing it again" << std::endl;
                        device_handles.erase(std::remove(device_handles.begin(), device_handles.end(), hrv_device->connection_handle), device_handles.end());
                        hrv_devices.erase(hrv_device);
                    }
                }
                else 
                {
                    CapnoTrainerHrv hrv_device(hrv_conn_handle, duration);
                    hrv_device.handle_incoming_data(sub_data, duration);
                    
                    // send device name request.
                    std::vector<uint8_t> write_name_data = { 0xdd, hrv_device.connection_handle };
                    Write(write_name_data, 0xff);

                    hrv_devices.push_back(hrv_device);

                    if (hrv_device.rr_array.size() > 0)
                    {
                        user_cb(hrv_device.rr_array, DONGLE_DEVTYPE_HRV, hrv_conn_handle, DATA_RR_INTERVALS);
                        hrv_device.rr_array.clear();
                    }
                }
            }
            
        }
    }

}

void CapnoTrainer::HandleEmgParser(std::vector<uint8_t> &data)
{
    uint8_t hrv_offset = 42;
    uint8_t emg_data_size = 3;
    std::vector<uint8_t> emg_data(data.begin() + hrv_offset, data.begin() + hrv_offset + 16);
    
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    double duration = (double) value.count();

    for (size_t i = 1; i < 14; i+= emg_data_size)
    {
        std::vector<uint8_t> emg_data_slice(emg_data.begin() + i, emg_data.begin() + i + emg_data_size);
        uint8_t emg_conn_handle = emg_data_slice.at(0);

        if (emg_conn_handle != DONGLE_INVALID_CONN_HANDLE)
        {
            // find the hrv device then
            std::vector<CapnoTrainerEmg>::iterator emg_device = std::find_if(
                emg_devices.begin(), emg_devices.end(),
                [&emg_conn_handle](const auto& emg) {
                return emg.connection_handle == emg_conn_handle;
            });

            if (emg_device != emg_devices.end())
            {
                if ( (duration - emg_device->last_time) < device_max_connection_time)
                {
                    emg_device->handle_incoming_data(emg_data_slice, duration);
                    if (emg_device->emg_array.size() > 0)
                    {
                        user_cb(emg_device->emg_array, DONGLE_DEVTYPE_EMG, emg_device->connection_handle, DATA_EMG);
                        emg_device->emg_array.clear();
                    }

                    if (emg_device->device_name.size() == 0)
                    {
                        // send device name request.
                        std::vector<uint8_t> write_name_data = { 0xdd, emg_device->connection_handle };
                        Write(write_name_data, 0xff);
                    }
                }
                else
                {
                    if (debug) std::cout << "Removing " << emg_device->device_name << "  from the device list and reinitializing it again" << std::endl;
                    device_handles.erase(std::remove(device_handles.begin(), device_handles.end(), emg_device->connection_handle), device_handles.end());
                    emg_devices.erase(emg_device);
                }

            }
            else 
            {
                CapnoTrainerEmg emg_device(emg_conn_handle, duration);
                emg_device.handle_incoming_data(emg_data_slice, duration);
                emg_devices.push_back(emg_device);
                
                // send device name request.
                std::vector<uint8_t> write_name_data = { 0xdd, emg_device.connection_handle };
                Write(write_name_data, 0xff);

                if (emg_device.emg_array.size() > 0)
                {
                    user_cb(emg_device.emg_array, DONGLE_DEVTYPE_EMG, emg_device.connection_handle, DATA_EMG);
                    emg_device.emg_array.clear();
                }
            }
        }


    }

}

void CapnoTrainer::HandleNameParser(uint8_t *read_buffer)
{
    uint8_t device_type = read_buffer[0];
    uint8_t connection_handle = read_buffer[1];

    if (connection_handle == 0xdd)
    {
        uint8_t device_name_conn_handle = read_buffer[2];
        uint8_t device_name_len = read_buffer[3];
        std::vector<uint8_t> device_name_vector;
        for (uint8_t i = 4; i < device_name_len + 4; i++)
        {
            device_name_vector.push_back(read_buffer[i]);
        }

        std::string device_name(device_name_vector.begin(), device_name_vector.end());

        if (device_name.find("GO") != std::string::npos || device_name.find("CAPT") != std::string::npos)
        {
            for (auto& capno : go_devices)
            {
                if (capno.connection_handle == device_name_conn_handle)
                {
                    if (debug) std::cout << "A GO device was connected with name: " << device_name.c_str() << std::endl;
                    capno.device_name = device_name;
                }
            }
        }
        else if (device_name.find("CAPNO-") != std::string::npos)
        {
            // capno 6 devices name
        }
        else if (device_name.find("ANR") != std::string::npos)
        {
            for (auto& emg : emg_devices)
            {
                if (emg.connection_handle == device_name_conn_handle)
                {
                    if (debug) std::cout << "A EMG device was connected with name: " << device_name.c_str() << std::endl;
                    emg.device_name = device_name;
                    break;
                }
            }
        }
        else
        {
            for (auto& hrv : hrv_devices)
            {
                if (hrv.connection_handle == device_name_conn_handle)
                {
                    if (debug) std::cout << "A HRV device was connected with name: " << device_name.c_str() << std::endl;
                    hrv.device_name = device_name;
                    break;
                }
            }
        }

    }
}


void CapnoTrainer::HandleDeviceConnections(uint8_t conn_handle)
{
    if (std::find(device_handles.begin(), device_handles.end(), conn_handle) != device_handles.end()) {
    }
    else {
        device_handles.push_back(conn_handle);
    }
}