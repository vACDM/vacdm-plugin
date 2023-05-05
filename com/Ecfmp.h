#pragma once

#include <curl/curl.h>
#include <json/json.h>
#include <chrono>
#include <types/Measure.h>
#include <list>

namespace vacdm {
namespace ecfmp {

/**
* @brief Defines the communication with the ECFMP API
*/
class Ecfmp {
	public: 
		~Ecfmp();

		Ecfmp(const Ecfmp&) = delete;
		Ecfmp(Ecfmp&&) = delete;

		Ecfmp& operator=(const Ecfmp&) = delete;
		Ecfmp& operator=(Ecfmp&&) = delete;

		static Ecfmp& instance();

		std::list<types::FlowMeasures> allFlowMeasures();

	private:
		Ecfmp();

		std::string m_baseUrl;
		std::string m_errorCode;

		CURL* curl;
		CURLcode res;

		static std::chrono::utc_clock::time_point isoStringToTimestamp(const std::string& timestamp);
	};
}
}