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
	m_firstCall(true),
	m_validWebApi(false),
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

/**
* @brief Checks if the ECFMP API is availabe and get/sets the FIR data
* @return Returns true if check was successful 
*/
bool Ecfmp::checkEcfmpApi()
{
	/* API is already checked */
	if (false == m_firstCall)
		return this->m_validWebApi;

	m_validWebApi = false;

	if (curl) {
		__receivedGetData.clear();

		std::string url = m_baseUrl + "/flight-information-region";
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		/* send the command */
		CURLcode result = curl_easy_perform(curl);
		if (CURLE_OK == result) {
			Json::CharReaderBuilder builder{};
			auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
			std::string errors;
			Json::Value root;

			if (reader->parse(__receivedGetData.c_str(), __receivedGetData.c_str() + __receivedGetData.length(), &root, &errors) && root.isArray()) {
				this->m_validWebApi = true;

				/* get FIR data: */
				for (const auto& firEntry : std::as_const(root)) {
					FlightInformationRegion fir;

					fir.id			= firEntry["id"].asInt();
					fir.identifier	= firEntry["identifier"].asString();
					fir.name		= firEntry["name"].asString();

					this->flightInformationRegions.push_back(fir);
				}
			}
			else {
				this->m_errorCode = "Invalid ECFMP backend response: " + __receivedGetData;
				this->m_validWebApi = false;
			}
		}
		else {
			this->m_errorCode = "Connection to the ECFMP failed";
		}
	}

	m_firstCall = false;
	return this->m_validWebApi;
}

std::list<types::FlowMeasures> Ecfmp::allFlowMeasures()
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

					flowMeasures.back().id				= flowMeasure["id"].asString();
					flowMeasures.back().ident			= flowMeasure["ident"].asString();
					flowMeasures.back().event_id		= flowMeasure["event_id"].asString();
					flowMeasures.back().reason			= flowMeasure["reason"].asString();
					flowMeasures.back().starttime		= Ecfmp::isoStringToTimestamp(flowMeasure["starttime"].asString());
					flowMeasures.back().endtime			= Ecfmp::isoStringToTimestamp(flowMeasure["endtime"].asString());
					flowMeasures.back().withdrawn_at	= Ecfmp::isoStringToTimestamp(flowMeasure["withdrawn_at"].asString());

					// handle notified FIRs array
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
					flowMeasures.back().measure.type	= flowMeasure["measure"]["type"].asString();
					flowMeasures.back().measure.value	= flowMeasure["measure"]["value"].asInt();

					// handle filters array
					Json::Value filters = flowMeasure["filters"];
					try
					{
						if (!filters.empty()) {
							for (const auto& filter : flowMeasure["filters"]) {
								vacdm::types::MeasureFilter measureFilter;

								measureFilter.type = filter["type"].asString();
								Json::Value value = filter["value"];
								if (!value.empty()) {
									for (const auto& filtervalue : filter["value"]) {
										measureFilter.value.push_back(filtervalue.asString());
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

const std::string& Ecfmp::errorMessage() const {
	return this->m_errorCode;
}
