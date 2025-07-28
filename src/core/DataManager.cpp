#include "DataManager.h"

#include "core/Server.h"
#include "log/Logger.h"
#include "main.h"
#include "utils/Date.h"

using namespace vacdm::com;
using namespace vacdm::core;
using namespace vacdm::logging;
using namespace std::chrono_literals;

static constexpr std::size_t ConsolidatedData = 0;
static constexpr std::size_t EuroscopeData = 1;
static constexpr std::size_t ServerData = 2;

DataManager::DataManager(std::shared_ptr<vacdm::com::Server> server, std::shared_ptr<vacdm::log::ILogger> logger)
    : m_server(server), m_logger(logger), m_pause(false), m_stop(false) {
    this->m_worker = std::thread(&DataManager::run, this);
}

DataManager::~DataManager() {
    this->m_stop = true;
    this->m_worker.join();
}

bool DataManager::checkPilotExists(const std::string& callsign) {
    if (true == this->m_pause) return false;

    std::lock_guard guard(this->m_pilotLock);
    return this->m_pilots.cend() != this->m_pilots.find(callsign);
}

const types::Pilot DataManager::getPilot(const std::string& callsign) {
    std::lock_guard guard(this->m_pilotLock);
    return this->m_pilots.find(callsign)->second[ConsolidatedData];
}

void DataManager::pause() { this->m_pause = true; }

void DataManager::resume() { this->m_pause = false; }

std::string DataManager::setUpdateCycleSeconds(const int newUpdateCycleSeconds) {
    if (newUpdateCycleSeconds < minUpdateCycleSeconds || newUpdateCycleSeconds > maxUpdateCycleSeconds)
        return "Could not set update rate";

    this->updateCycleSeconds = newUpdateCycleSeconds;

    return "vACDM updating every " +
           (newUpdateCycleSeconds == 1 ? "second" : std::to_string(newUpdateCycleSeconds) + " seconds");
}

void DataManager::run() {
    std::size_t counter = 1;
    while (true) {
        std::this_thread::sleep_for(1s);
        if (true == this->m_stop) return;
        if (true == this->m_pause) continue;

        // run every updateCycleSeconds seconds
        if (counter++ % updateCycleSeconds != 0) continue;

        // obtain a copy of the pilot data, work with the copy to minimize lock time
        std::map<std::string, std::array<vacdm::types::Pilot, 3U>> pilots;
        {
            std::lock_guard guard(this->m_pilotLock);
            pilots = this->m_pilots;
        }

        this->processAsynchronousMessages(pilots);

        this->processEuroScopeUpdates(pilots);

        this->consolidateWithBackend(pilots);

        if (true == m_server->getMaster()) {
            std::list<std::tuple<types::Pilot, DataManager::MessageType, Json::Value>> transmissionBuffer;
            for (auto& pilot : pilots) {
                Json::Value message;
                const auto sendType = DataManager::deltaEuroscopeToBackend(pilot.second, message);
                if (MessageType::None != sendType)
                    transmissionBuffer.push_back({pilot.second[ConsolidatedData], sendType, message});
            }

            for (const auto& transmission : std::as_const(transmissionBuffer)) {
                if (std::get<1>(transmission) == MessageType::Post)
                    m_server->postPilot(std::get<0>(transmission));
                else if (std::get<1>(transmission) == MessageType::Patch)
                    m_server->sendPatchMessage("/api/v1/pilots/" + std::get<0>(transmission).callsign,
                                               std::get<2>(transmission));
            }
        }

        {
            // replace the pilot data with the updated copy
            std::lock_guard guard(this->m_pilotLock);
            this->m_pilots = pilots;
        }
    }
}

void DataManager::processAsynchronousMessages(std::map<std::string, std::array<types::Pilot, 3U>>& pilots) {
    std::list<AsynchronousMessage> messages;
    {
        std::lock_guard guard(this->m_asyncMessagesLock);
        messages = std::move(this->m_asynchronousMessages);
    }

    for (auto& message : messages) {
        auto pilot = pilots.find(message.callsign);
        if (pilot == pilots.end()) continue;

        auto& [callsign, data] = *pilot;

        std::string messageType;

        switch (message.type) {
            case MessageType::UpdateEXOT:
                m_server->updateExot(message.callsign, message.value);
                messageType = "EXOT";
                break;
            case MessageType::UpdateTOBT:
                m_server->updateTobt(data[ConsolidatedData], message.value, false);
                messageType = "TOBT";
                break;
            case MessageType::UpdateTOBTConfirmed:
                m_server->updateTobt(data[ConsolidatedData], message.value, true);
                messageType = "TOBT Confirmed Status";
                break;
            case MessageType::UpdateASAT:
                m_server->updateAsat(message.callsign, message.value);
                messageType = "ASAT";
                break;
            case MessageType::UpdateASRT:
                m_server->updateAsrt(message.callsign, message.value);
                messageType = "ASRT";
                break;
            case MessageType::UpdateAOBT:
                m_server->updateAobt(message.callsign, message.value);
                messageType = "AOBT";
                break;
            case MessageType::UpdateAORT:
                m_server->updateAort(message.callsign, message.value);
                messageType = "AORT";
                break;
            case MessageType::ResetTOBT:
                m_server->resetTobt(message.callsign, types::defaultTime, data[ConsolidatedData].tobt_state);
                messageType = "TOBT reset";
                break;
            case MessageType::ResetASAT:
                m_server->updateAsat(message.callsign, message.value);
                messageType = "ASAT reset";
                break;
            case MessageType::ResetASRT:
                m_server->updateAsrt(message.callsign, message.value);
                messageType = "ASRT reset";
                break;
            case MessageType::ResetTOBTConfirmed:
                m_server->resetTobt(message.callsign, data[ConsolidatedData].tobt, "GUESS");
                messageType = "TOBT confirmed reset";
                break;
            case MessageType::ResetAORT:
                m_server->updateAort(message.callsign, message.value);
                messageType = "AORT reset";
                break;
            case MessageType::ResetAOBT:
                m_server->updateAobt(message.callsign, message.value);
                messageType = "AOBT reset";
                break;
            case MessageType::ResetPilot:
                m_server->deletePilot(message.callsign);
                pilots.erase(message.callsign);
                messageType = "Pilot reset";
                break;

            default:
                break;
        }

        m_logger->info("Sending " + messageType + " update: " + message.callsign + " - " +
                       utils::Date::timestampToIsoString(message.value));
    }
}

void DataManager::handleTagFunction(MessageType type, const std::string callsign,
                                    const std::chrono::utc_clock::time_point value) {
    // do not handle the tag function if the aircraft does not exist or the client is not master
    if (false == this->checkPilotExists(callsign) || false == m_server->getMaster()) return;

    // queue the update message which will be sent to the backend
    {
        std::lock_guard guard(this->m_asyncMessagesLock);
        this->m_asynchronousMessages.push_back({type, callsign, value});
    }

    // set the data locally, gives feedback to user that the action was handled, might get overwritten again in the
    // update cycle if the backend does not accept the message
    std::lock_guard guard(this->m_pilotLock);
    auto it = this->m_pilots.find(callsign);
    auto& pilot = it->second[ConsolidatedData];

    pilot.lastUpdate = std::chrono::utc_clock::now();

    switch (type) {
        case MessageType::UpdateEXOT:
            pilot.exot = value;
            pilot.tsat = types::defaultTime;
            pilot.ttot = types::defaultTime;
            pilot.asat = types::defaultTime;
            pilot.aobt = types::defaultTime;
            pilot.atot = types::defaultTime;
            break;
        case MessageType::UpdateTOBT: {
            bool resetTsat = value >= pilot.tsat;

            pilot.tobt = value;
            if (true == resetTsat) pilot.tsat = types::defaultTime;
            pilot.ttot = types::defaultTime;
            pilot.exot = types::defaultTime;
            pilot.asat = types::defaultTime;
            pilot.aobt = types::defaultTime;
            pilot.atot = types::defaultTime;

            break;
        }
        case MessageType::UpdateTOBTConfirmed: {
            bool resetTsat = value == types::defaultTime || value >= pilot.tsat;

            pilot.tobt = value;
            if (true == resetTsat) pilot.tsat = types::defaultTime;
            pilot.ttot = types::defaultTime;
            pilot.exot = types::defaultTime;
            pilot.asat = types::defaultTime;
            pilot.aobt = types::defaultTime;
            pilot.atot = types::defaultTime;

            break;
        }
        case MessageType::UpdateASAT:
            pilot.asat = value;
            break;
        case MessageType::UpdateASRT:
            pilot.asrt = value;
            break;
        case MessageType::UpdateAOBT:
            pilot.aobt = value;
            break;
        case MessageType::UpdateAORT:
            pilot.aort = value;
            break;
        case MessageType::ResetTOBT:
            pilot.tobt = types::defaultTime;
            pilot.tsat = types::defaultTime;
            pilot.ttot = types::defaultTime;
            pilot.exot = types::defaultTime;
            pilot.asat = types::defaultTime;
            pilot.asrt = types::defaultTime;
            pilot.aobt = types::defaultTime;
            pilot.aort = types::defaultTime;
            pilot.atot = types::defaultTime;
            break;
        case MessageType::ResetASAT:
            pilot.asat = types::defaultTime;
            break;
        case MessageType::ResetASRT:
            pilot.asrt = types::defaultTime;
            break;
        case MessageType::ResetTOBTConfirmed:
            pilot.tobt_state = "GUESS";
            break;
        case MessageType::ResetAORT:
            pilot.aort = types::defaultTime;
            break;
        case MessageType::ResetAOBT:
            pilot.aobt = types::defaultTime;
            break;
        case MessageType::ResetPilot:
            pilot.eobt = types::defaultTime;
            pilot.tobt = types::defaultTime;
            pilot.ctot = types::defaultTime;
            pilot.ttot = types::defaultTime;
            pilot.tsat = types::defaultTime;
            pilot.exot = types::defaultTime;
            pilot.asat = types::defaultTime;
            pilot.aobt = types::defaultTime;
            pilot.atot = types::defaultTime;
            pilot.atot = types::defaultTime;
            pilot.asrt = types::defaultTime;
            pilot.aort = types::defaultTime;
            break;
        default:
            break;
    }
}

DataManager::MessageType DataManager::deltaEuroscopeToBackend(const std::array<types::Pilot, 3>& data,
                                                              Json::Value& message) {
    message.clear();

    if (data[ServerData].callsign == "" && data[EuroscopeData].callsign != "") {
        return DataManager::MessageType::Post;
    } else {
        message["callsign"] = data[EuroscopeData].callsign;

        int deltaCount = 0;

        if (data[EuroscopeData].inactive != data[ServerData].inactive) {
            message["inactive"] = data[EuroscopeData].inactive;
            deltaCount += 1;
        }

        auto lastDelta = deltaCount;
        message["position"] = Json::Value();
        if (data[EuroscopeData].latitude != data[ServerData].latitude) {
            message["position"]["lat"] = data[EuroscopeData].latitude;
            deltaCount += 1;
        }
        if (data[EuroscopeData].longitude != data[ServerData].longitude) {
            message["position"]["lon"] = data[EuroscopeData].longitude;
            deltaCount += 1;
        }
        if (deltaCount == lastDelta) message.removeMember("position");

        // patch flightplan data
        lastDelta = deltaCount;
        message["flightplan"] = Json::Value();
        if (data[EuroscopeData].origin != data[ServerData].origin) {
            deltaCount += 1;
            message["flightplan"]["departure"] = data[EuroscopeData].origin;
        }
        if (data[EuroscopeData].destination != data[ServerData].destination) {
            deltaCount += 1;
            message["flightplan"]["arrival"] = data[EuroscopeData].destination;
        }
        if (deltaCount == lastDelta) message.removeMember("flightplan");

        // patch clearance data
        lastDelta = deltaCount;
        message["clearance"] = Json::Value();
        if (data[EuroscopeData].runway != data[ServerData].runway) {
            deltaCount += 1;
            message["clearance"]["dep_rwy"] = data[EuroscopeData].runway;
        }
        if (data[EuroscopeData].sid != data[ServerData].sid) {
            deltaCount += 1;
            message["clearance"]["sid"] = data[EuroscopeData].sid;
        }
        if (deltaCount == lastDelta) message.removeMember("clearance");

        return deltaCount != 0 ? DataManager::MessageType::Patch : DataManager::MessageType::None;
    }
}

void DataManager::setActiveAirports(const std::list<std::string> activeAirports) {
    std::lock_guard guard(this->m_airportLock);
    this->m_activeAirports = activeAirports;
}

void DataManager::queueFlightplanUpdate(EuroScopePlugIn::CFlightPlan flightplan) {
    // skip the update if:
    // - the flightplan or its data is invalid
    // - or the aircraft is out of range therefore GetSimulated() is true
    if (false == flightplan.IsValid() || nullptr == flightplan.GetFlightPlanData().GetPlanType() ||
        nullptr == flightplan.GetFlightPlanData().GetOrigin() || flightplan.GetSimulated())
        return;

    auto pilot = this->CFlightPlanToPilot(flightplan);

    std::lock_guard guard(this->m_euroscopeUpdatesLock);
    this->m_euroscopeFlightplanUpdates.push_back({std::chrono::utc_clock::now(), pilot});
}

void DataManager::consolidateWithBackend(std::map<std::string, std::array<types::Pilot, 3U>>& pilots) {
    // retrieving backend data
    auto backendPilots = m_server->getPilots(this->m_activeAirports);

    for (auto pilot = pilots.begin(); pilots.end() != pilot;) {
        // update backend data & consolidate
        bool removeFlight = pilot->second[ServerData].inactive == true;
        for (auto updateIt = backendPilots.begin(); updateIt != backendPilots.end(); ++updateIt) {
            if (updateIt->callsign == pilot->second[EuroscopeData].callsign) {
                m_logger->info("Updating " + pilot->second[EuroscopeData].callsign + " with" + updateIt->callsign);
                pilot->second[ServerData] = *updateIt;
                DataManager::consolidateData(pilot->second);
                removeFlight = false;
                updateIt = backendPilots.erase(updateIt);
                break;
            }
        }

        // remove pilot if he has been flagged as inactive from the backend
        if (true == removeFlight) {
            pilot = pilots.erase(pilot);
        } else {
            ++pilot;
        }
    }
}

void DataManager::consolidateData(std::array<types::Pilot, 3>& pilot) {
    if (pilot[EuroscopeData].callsign == pilot[ServerData].callsign) {
        // backend data
        pilot[ConsolidatedData].inactive = pilot[ServerData].inactive;
        pilot[ConsolidatedData].lastUpdate = pilot[ServerData].lastUpdate;

        pilot[ConsolidatedData].eobt = pilot[ServerData].eobt;
        pilot[ConsolidatedData].tobt = pilot[ServerData].tobt;
        pilot[ConsolidatedData].tobt_state = pilot[ServerData].tobt_state;
        pilot[ConsolidatedData].ctot = pilot[ServerData].ctot;
        pilot[ConsolidatedData].ttot = pilot[ServerData].ttot;
        pilot[ConsolidatedData].tsat = pilot[ServerData].tsat;
        pilot[ConsolidatedData].exot = pilot[ServerData].exot;
        pilot[ConsolidatedData].asat = pilot[ServerData].asat;
        pilot[ConsolidatedData].aobt = pilot[ServerData].aobt;
        pilot[ConsolidatedData].atot = pilot[ServerData].atot;
        pilot[ConsolidatedData].asrt = pilot[ServerData].asrt;
        pilot[ConsolidatedData].aort = pilot[ServerData].aort;

        pilot[ConsolidatedData].measures = pilot[ServerData].measures;
        pilot[ConsolidatedData].hasBooking = pilot[ServerData].hasBooking;
        pilot[ConsolidatedData].taxizoneIsTaxiout = pilot[ServerData].taxizoneIsTaxiout;

        // EuroScope data
        pilot[ConsolidatedData].latitude = pilot[EuroscopeData].latitude;
        pilot[ConsolidatedData].longitude = pilot[EuroscopeData].longitude;

        pilot[ConsolidatedData].origin = pilot[EuroscopeData].origin;
        pilot[ConsolidatedData].destination = pilot[EuroscopeData].destination;
        pilot[ConsolidatedData].runway = pilot[EuroscopeData].runway;
        pilot[ConsolidatedData].sid = pilot[EuroscopeData].sid;

        m_logger->info("Consolidated " + pilot[ServerData].callsign);
    } else {
        m_logger->info("Callsign mismatch during consolidation: " + pilot[EuroscopeData].callsign + ", " +
                       pilot[ServerData].callsign);
    }
}

void DataManager::processEuroScopeUpdates(std::map<std::string, std::array<types::Pilot, 3U>>& pilots) {
    // obtain a copy of the flightplan updates, clear the update list, consolidate flightplan updates
    std::list<EuroscopeFlightplanUpdate> flightplanUpdates;
    {
        std::lock_guard guard(this->m_euroscopeUpdatesLock);
        flightplanUpdates.swap(this->m_euroscopeFlightplanUpdates);
    }

    this->consolidateFlightplanUpdates(flightplanUpdates);

    for (auto& update : flightplanUpdates) {
        const auto& pilot = update.data;

        auto it = pilots.find(pilot.callsign);

        if (it != pilots.end()) {
            // Pilot found, update the corresponding data
            m_logger->info("Updated data of " + pilot.callsign);
            it->second[EuroscopeData] = pilot;
        } else {
            // Pilot not found, add a new entry
            m_logger->info("Added new pilot entry for callsign: " + pilot.callsign);
            pilots.insert({pilot.callsign, std::array<types::Pilot, 3U>{pilot, pilot, types::Pilot()}});
        }
    }
}

void DataManager::consolidateFlightplanUpdates(std::list<EuroscopeFlightplanUpdate>& inputList) {
    std::list<DataManager::EuroscopeFlightplanUpdate> resultList;

    for (const auto& currentUpdate : inputList) {
        auto pilot = currentUpdate.data;

        // only handle updates for active airports
        {
            std::lock_guard guard(this->m_airportLock);
            bool flightDepartsFromActiveAirport = std::find(m_activeAirports.begin(), m_activeAirports.end(),
                                                            std::string(pilot.origin)) != m_activeAirports.end();
            if (false == flightDepartsFromActiveAirport) continue;
        }

        // Check if the flight plan already exists in the result list
        auto it = std::find_if(resultList.begin(), resultList.end(),
                               [&currentUpdate](const EuroscopeFlightplanUpdate& existingUpdate) {
                                   return existingUpdate.data.callsign == currentUpdate.data.callsign;
                               });

        if (it != resultList.end()) {
            // Flight plan with the same callsign exists
            // Check if the timeIssued is newer
            if (currentUpdate.timeIssued > it->timeIssued) {
                // Update with the newer data
                *it = currentUpdate;
                m_logger->info("Updated: " + std::string(currentUpdate.data.callsign));
            } else {
                // Existing data is already newer, no update needed
                m_logger->info("Skipped old update for: " + std::string(currentUpdate.data.callsign));
            }
        } else {
            // Flight plan with the callsign doesn't exist, add it to the result list
            resultList.push_back(currentUpdate);
            m_logger->info("Update added: " + std::string(currentUpdate.data.callsign));
        }
    }

    inputList = resultList;
}

types::Pilot DataManager::CFlightPlanToPilot(const EuroScopePlugIn::CFlightPlan flightplan) {
    types::Pilot pilot;

    pilot.callsign = flightplan.GetCallsign();
    pilot.lastUpdate = std::chrono::utc_clock::now();

    // position data
    if (Plugin->RadarTargetSelect(pilot.callsign.c_str()).IsValid()) {
        // get the position of the flight using its radar target, it's more precise
        pilot.latitude = Plugin->RadarTargetSelect(pilot.callsign.c_str()).GetPosition().GetPosition().m_Latitude;
        pilot.longitude = Plugin->RadarTargetSelect(pilot.callsign.c_str()).GetPosition().GetPosition().m_Longitude;
    } else {
        // if we have no radar target we will use the fptrackposition,
        // not sufficient precision to determine the taxizone
        pilot.latitude = flightplan.GetFPTrackPosition().GetPosition().m_Latitude;
        pilot.longitude = flightplan.GetFPTrackPosition().GetPosition().m_Longitude;
    }

    // flightplan & clearance data
    pilot.origin = flightplan.GetFlightPlanData().GetOrigin();
    pilot.destination = flightplan.GetFlightPlanData().GetDestination();
    pilot.runway = flightplan.GetFlightPlanData().GetDepartureRwy();
    pilot.sid = flightplan.GetFlightPlanData().GetSidName();

    // acdm data
    pilot.eobt = utils::Date::convertEuroscopeDepartureTime(flightplan);
    pilot.tobt = pilot.eobt;

    return pilot;
}