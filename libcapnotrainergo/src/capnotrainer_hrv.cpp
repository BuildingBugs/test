#include "capnotrainer_hrv.h"


CapnoTrainerHrv::CapnoTrainerHrv(uint8_t conn_handle, double current_time) : 
	connection_handle(conn_handle),
	last_time(current_time)
{

}

void CapnoTrainerHrv::handle_incoming_data(std::vector<uint8_t>& data, double current_time )
{

	uint16_t heart_rate = static_cast<uint16_t> ( data.at(1) << 8 ) | ( data.at(2) ) ;
	uint16_t rr_interval_1 = static_cast<uint16_t> (data.at(3) << 8) | (data.at(4));
	uint16_t rr_interval_2 = static_cast<uint16_t> (data.at(5) << 8) | (data.at(6));


	if (heart_rate > 0 && heart_rate < 65535) {
		hr_array.push_back(heart_rate);
	}

	if (rr_interval_1 > 0 && rr_interval_1 < 65535)
	{
		if (rr_interval_2 > 0 && rr_interval_2 < 65535)
		{
			float combine = ( static_cast<float> (rr_interval_1) + static_cast<float> (rr_interval_2) ) / 2;
			rr_array.push_back(combine);
		}
		else
		{
			rr_array.push_back(static_cast<float> (rr_interval_1) );
		}
	}

	last_time = current_time;
}