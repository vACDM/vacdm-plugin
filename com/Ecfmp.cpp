#include "Ecfmp.h"
#include "logging/Logger.h"

using namespace vacdm;
using namespace vacdm::ecfmp;

static std::string __receivedGetData;

static std::size_t receiveCurlGet(void* ptr, std::size_t size, std::size_t nmemb, void* stream) {
	(void)stream;

	std::string serverResult = static_cast<char*>(ptr);
	__receivedGetData += serverResult;
	return size * nmemb;
}

Ecfmp::Ecfmp() :
	m_baseUrl("https://ecfmp.vatsim.net/api/v1"),
	m_errorCode(),
	curl(curl_easy_init()),
	res(CURLE_OK) {

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveCurlGet);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
	}
}

Ecfmp::~Ecfmp() {
	if (curl)
	{
		curl_easy_cleanup(curl);
	}
}

Ecfmp& Ecfmp::instance() {
	static Ecfmp __instance;
	return __instance;
}

std::chrono::utc_clock::time_point Ecfmp::isoStringToTimestamp(const std::string& timestamp) {
	std::chrono::utc_clock::time_point retval;
	std::stringstream stream;

	stream << timestamp.substr(0, timestamp.length() - 1);
	std::chrono::from_stream(stream, "%FT%T", retval);

	return retval;
}

std::list<types::FlowMeasures> vacdm::ecfmp::Ecfmp::allFlowMeasures()
{
	if (curl != nullptr) {
		__receivedGetData.clear();

		std::string url = m_baseUrl + "/flow-measure";

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		// get data
		CURLcode result = curl_easy_perform(curl);
		if (CURLE_OK == result) {
			Json::CharReaderBuilder builder{};
			auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
			std::string errors;
			Json::Value root;

			if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root, &errors) && root.isArray()) {
				std::list<types::FlowMeasures> flowMeasures;

				for (const auto& flowMeasure : std::as_const(root)) {
					flowMeasures.push_back(types::FlowMeasures());

					flowMeasures.back().id = flowMeasure["id"].asString();
					flowMeasures.back().ident = flowMeasure["ident"].asString();
					flowMeasures.back().event_id = flowMeasure["event_id"].asString();
					flowMeasures.back().reason = flowMeasure["reason"].asString();
					flowMeasures.back().starttime = Ecfmp::isoStringToTimestamp(flowMeasure["starttime"].asString());
					flowMeasures.back().endtime = Ecfmp::isoStringToTimestamp(flowMeasure["endtime"].asString());
					flowMeasures.back().withdrawn_at = Ecfmp::isoStringToTimestamp(flowMeasure["withdrawn_at"].asString());

					// notified FIRs
					Json::Value notifiedFirs = flowMeasure["notified_flight_information_regions"];
					try
					{	
						if (!notifiedFirs.empty()) {
							for (const auto& region : flowMeasure["notified_flight_information_regions"]) {
								flowMeasures.back().notified_flight_information_regions.push_back(region.asString());
							}
						}
					}
					catch (const std::exception& e) {
						logging::Logger::instance().log("ECFMP JSON ERROR", logging::Logger::Level::Error, "Error in notified FIR conversion");
						logging::Logger::instance().log("error", logging::Logger::Level::Error, e.what());
					}

					// measure
					flowMeasures.back().measure.type = flowMeasure["measure"]["type"].asString();
					flowMeasures.back().measure.value = flowMeasure["measure"]["value"].asInt();

					// filters
					Json::Value filters = flowMeasure["filters"];
					try
					{
						if (!filters.empty()) {
							for (const auto& filter : flowMeasure["filters"]) {
								vacdm::types::MeasureFilter measureFilter;

								measureFilter.type = filter["type"].asString();
								Json::Value value = filter["value"];
								if (!value.empty()) {
									for (const auto& value : filter["value"]) {
										measureFilter.value.push_back(value.asString());
									}
								}

								flowMeasures.back().filters.push_back(measureFilter);
							}
						}
					}
					catch (const std::exception& e)
					{
						logging::Logger::instance().log("ECFMP JSON ERROR", logging::Logger::Level::Error, "Error in Filter conversion");
						logging::Logger::instance().log("error", logging::Logger::Level::Error, e.what());
					}
				}
				return flowMeasures;
			}
		}
	}
	else {
		logging::Logger::instance().log("ECFMP", logging::Logger::Level::Debug, "curl == nullptr");
	}

	return {};
}
