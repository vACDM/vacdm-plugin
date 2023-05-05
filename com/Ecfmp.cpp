#include "Ecfmp.h"

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
	m_baseUrl("https://ecfmp.vatsim.net/api/v1/"),
	m_errorCode() {

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveCurlGet);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
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
					std::vector<std::string> parsedNotifiedFirs;
					if (!notifiedFirs.empty()) {
						for (int i = 0; i < notifiedFirs.size(); i++) {
							parsedNotifiedFirs.push_back(notifiedFirs[i]["notified_flight_information_regions"].asString());
						}
						flowMeasures.back().notified_flight_information_regions = parsedNotifiedFirs;
					}

					// measure
					flowMeasures.back().measure.type = flowMeasure["measure"]["type"].asString();
					flowMeasures.back().measure.value = flowMeasure["measure"]["value"].asInt();

					// filters
					Json::Value filters = flowMeasure["filters"];
					std::vector<types::MeasureFilter> parsedFilters;
					if (!filters.empty() && filters.isArray()) {
						for (int i = 0; i < notifiedFirs.size(); i++) {
							types::MeasureFilter measureFilter;

							measureFilter.type = filters["type"].asString();
							std::vector<std::string> values;
							if (filters["value"].isArray()) {

							}
							measureFilter.value = values;

							parsedFilters.push_back(measureFilter);
						}
						flowMeasures.back().filters = parsedFilters;
					}
				}
			}
		}
	}

	return std::list<types::FlowMeasures>();
}
