#pragma once

#include <string>

namespace vacdm {
namespace types {

#pragma warning(disable: 4091)
typedef struct Measure {
	std::string ident;  // measure id, same as in FlowMeasures -> remove?
	int value = -1;     // measure value in seconds, i.e. 5
	std::string type;	// MDI ("minimum_departure_interval")
};

typedef struct MeasureFilter {
	std::string type; // i.e. "ADEP", "ADES" or "level"
	std::vector<std::string> value; //i.e. "EGKK", "230", "EH**"
};
typedef struct FlowMeasures
{
		std::string id;
		std::string ident;
		std::string event_id;
		std::string reason;
		std::chrono::utc_clock::time_point starttime;
		std::chrono::utc_clock::time_point endtime;
		std::chrono::utc_clock::time_point withdrawn_at;
		std::vector<std::string> notified_flight_information_regions;
		Measure measure;
		std::vector<MeasureFilter> filters;
};
#pragma warning(default: 4091)
}
}