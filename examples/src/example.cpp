#include <iostream> 
#include <thread>
#include <functional>
#include <chrono>
#include "capnotrainer.h"


CapnoTrainer* capno = nullptr;

void user_data_callback(std::vector<float> data, DeviceType device_type, uint8_t conn_handle, DataType data_type)
{
    // this callback function is called every time a 
    // new data point is received on the dongle. 

    // the data point is parsed based on its device type and 
    // passed to this function. 

    // Users are instructed to deep copy paste before the 
    // buffer goes out of scope.
    
    switch (device_type)
    {
    case DONGLE_DEVTYPE_CAPNO_GO:
    {
        if (data_type == DATA_CO2)
        {
           //std::cout << "Received CO2 data with length: " << data.size() << "  with handle: " << (int)conn_handle << std::endl;
        }
        if (data_type == DATA_CAPNO_BATTERY)
        {
            std::cout << "Received Battery data with length: " << data.at(0) << "  with handle: " << (int)conn_handle << std::endl;
        }
        if (data_type == DATA_ETCO2_AVERAGE)
        {
            std::cout << "Received ETCO2 average data with length: " << data.at(0) << "  with handle: " << (int)conn_handle << std::endl;
        }
        if (data_type == DATA_BPM_AVERAGE)
        {
            std::cout << "Received BPM average data with length: " << data.at(0) << "  with handle: " << (int)conn_handle << std::endl;
        }
        if (data_type == DATA_INSP_CO2_AVERAGE)
        {
            std::cout << "Received Insp. CO2 average data with length: " << data.at(0) << "  with handle: " << (int)conn_handle << std::endl;
        }
        if (data_type == DATA_CAPNO_STATUS)
        {
            // capno status (future implementation)
        }
    }
    break;

    case DONGLE_DEVTYPE_EMG:
    {
        if (data_type == DATA_EMG)
        {
            std::cout << "Received EMG data with length: " << data.size() << "  with handle: " << (int)conn_handle << std::endl;
        }
    }
    break;

    case DONGLE_DEVTYPE_HRV:
    {
        if (data_type == DATA_RR_INTERVALS)
        {
            std::cout << "Receuved RR-interval data with length: " << data.at(0) << "  with handle: " << (int)conn_handle << std::endl;
        }
        if (data_type == DATA_HEART_RATE)
        {
            // Some HRV devices outputs heart rate 
            // but we have not implemented it as it is 
            // an average heart rate instead of an instanteneous one. 
        }
    }
    break;

    case DONGLE_DEVTYPE_CAPNO_6:
    {
        // NOT IMPLEMENTED
    }
    break;

    default:
        break;
    }


}


void counter() {

    int count = 0;
    while (count >= 0) {
        std::cout << "Please enter an integer: ";
        std::cin >> count;
        std::cout << "You entered: " << count << std::endl;

        if (capno != nullptr) {
            if (count == 1) {
                capno->Disconnect();
            } 
            if (count == 2) {
                capno->Connect("COM15", "COM16");
            }
        }
    }

    return;
}

int main(int argc, char* argv[]) {

    /*if (argc != 3)
    {
        std::cout << "Please run as following: \n main.exe COMx COMy" << std::endl;
        return 0;
    }*/

    // port names can be found based on 
    // port enumeration and pid/vid values. 
    const char* port1 = "COM15"; //argv[1]; //  "/dev/ttyACM0";
    const char* port2 = "COM16"; //argv[2]; //  "/dev/ttyACM1";

    std::cout << "CapnoTrainer: " << CapnoTrainer::GetVersion() << std::endl;

    try {
        capno = new CapnoTrainer(user_data_callback, true);
        capno->Connect(port1, port2);
        // capno->Initialize();
        //std::thread t1(std::thread([]() { capno->Initialize(); }));
        std::thread t2(std::thread([]() { counter();  }));
        //t1.join();
        t2.join();
    }
    catch (asio::system_error& e) {
        std::cout << e.what() << std::endl;
    }

    while (true) {
    }

    return 0;
}