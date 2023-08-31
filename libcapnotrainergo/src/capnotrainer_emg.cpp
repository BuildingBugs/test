#include "capnotrainer_emg.h"


CapnoTrainerEmg::CapnoTrainerEmg(uint8_t conn_handle, double current_time) 
	: connection_handle(conn_handle),
	last_time(current_time)
{

}

void CapnoTrainerEmg::handle_incoming_data(std::vector<uint8_t> &data, double current_time)
{
	uint16_t emg_data = (uint16_t)data.at(1) | ((uint16_t)data.at(2) << 8);
	emg_array.push_back(emg_data);
	last_time = current_time;
}