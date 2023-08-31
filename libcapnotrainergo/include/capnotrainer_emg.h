#pragma once

#include "commons.h"


class CapnoTrainerEmg
{

public:
	CapnoTrainerEmg(uint8_t conn_handle, double current_time);

	uint8_t connection_handle;
	std::string device_name;
	double last_time; 

	void handle_incoming_data(std::vector<uint8_t> &data, double current_time);
	std::vector<float> emg_array;
};