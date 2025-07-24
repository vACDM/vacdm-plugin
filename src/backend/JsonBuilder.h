#pragma once

#include <json/json.h>

#include "types/Pilot.h"
#include "utils/Date.h"

namespace vacdm::backend {
/// @brief Utility class for constructing JSON messages for communication between plugin and backend.
///
/// This class encapsulates the construction of structured JSON messages according to the
/// specification agreed upon by the vACDM plugin and the backend server. Each static method
/// generates a specific message type based on provided data.
class JsonBuilder {
   public:
    static Json::Value buildInitialPilotData(const types::Pilot& data) {
        Json::Value json = Json::Value();

        json["callsign"] = data.callsign;
        json["inactive"] = data.inactive;

        json["position"] = Json::Value();
        json["position"]["lat"] = data.latitude;
        json["position"]["lon"] = data.longitude;

        json["flightplan"] = Json::Value();
        json["flightplan"]["adep"] = data.origin;
        json["flightplan"]["ades"] = data.destination;

        json["vacdm"] = Json::Value();
        json["vacdm"]["eobt"] = utils::Date::timestampToIsoString(data.eobt);
        json["vacdm"]["tobt"] = utils::Date::timestampToIsoString(data.tobt);

        json["clearance"] = Json::Value();
        json["clearance"]["dep_rwy"] = data.runway;
        json["clearance"]["sid"] = data.sid;

        return json;
    }

    static Json::Value buildTargetDpiNow(const types::Pilot& data) {
        Json::Value json = Json::Value();

        json["callsign"] = data.callsign;
        json["messageType"] = "T-DPI-n";
        json["tobtState"] = "NOW";

        return json;
    }

    static Json::Value buildTargetDpiTarget(const types::Pilot& data) {
        Json::Value json = Json::Value();

        json["callsign"] = data.callsign;
        json["messageType"] = "T-DPI-t";
        json["tobtState"] = "CONFIRMED";
        json["tobt"] = utils::Date::timestampToIsoString(data.tobt);

        return json;
    }

    static Json::Value buildTargetDpiSequenced(const types::Pilot& data) {
        Json::Value json = Json::Value();

        json["callsign"] = data.callsign;
        json["messageType"] = "T-DPI-s";
        json["asat"] = utils::Date::timestampToIsoString(data.asat);

        return json;
    }

    static Json::Value buildAtcDpi(const types::Pilot& data) {
        Json::Value json = Json::Value();

        json["callsign"] = data.callsign;
        json["messageType"] = "A-DPI";
        json["aobt"] = utils::Date::timestampToIsoString(data.aobt);

        return json;
    }

    static Json::Value buildCustomDpiTaxioutTime(const types::Pilot& data) {
        Json::Value json = Json::Value();

        json["callsign"] = data.callsign;
        json["messageType"] = "X-DPI-taxi";
        json["exot"] = utils::Date::timestampToIsoString(data.exot);

        return json;
    }

    static Json::Value buildCustomDpiRequest(const types::Pilot& data, const bool isAsrtUpdate) {
        Json::Value json = Json::Value();

        json["callsign"] = data.callsign;
        json["messageType"] = "X-DPI-req";
        if (isAsrtUpdate) {
            json["asrt"] = utils::Date::timestampToIsoString(data.asrt);
        } else {
            json["aort"] = utils::Date::timestampToIsoString(data.aort);
        }

        return json;
    }

    static Json::Value buildPilotDisconnect(const std::string& callsign) {
        Json::Value json = Json::Value();

        json["callsign"] = callsign;
        json["disconnected"] = true;

        return json;
    }
};
}  // namespace vacdm::backend
