#include <list>

#include "Airport.h"
#include "logging/Logger.h"
#include "Server.h"

using namespace std::chrono_literals;
using namespace vacdm;
using namespace vacdm::com;

static constexpr std::size_t FlightConsolidated = 0;
static constexpr std::size_t FlightEuroscope = 1;
static constexpr std::size_t FlightServer = 2;

Airport::Airport() :
        m_airport(),
        m_worker(),
        m_pause(false),
        m_lock("EMPTY"),
        m_flights(),
        m_stop(false),
        m_manualUpdatePerformance("EMPTY"),
        m_workerAllFlightsPerformance("EMPTY"),
        m_workerUpdateFlightsPerformance("EMPTY"),
        m_asynchronousMessageLock("EMPTY"),
        m_asynchronousMessages() { }

Airport::Airport(const std::string& airport) :
        m_airport(airport),
        m_worker(),
        m_pause(false),
        m_lock(airport),
        m_flights(),
        m_stop(false),
        m_manualUpdatePerformance("ManualUpdates" + airport),
        m_workerAllFlightsPerformance("AllFlightsRequest" + airport),
        m_workerUpdateFlightsPerformance("UpdateFlights" + airport),
        m_asynchronousMessageLock("AsyncMessages" + airport),
        m_asynchronousMessages() {
    const auto flights = Server::instance().allFlights(this->m_airport);
    for (const auto& flight : std::as_const(flights)) {
        this->m_flights.insert({ flight.callsign, { flight, types::Flight_t(), flight } });
    }

    this->m_worker = std::thread(&Airport::run, this);
}

Airport::~Airport() {
    this->m_stop = true;
    this->m_worker.join();
}

void Airport::pause() {
    this->m_pause = true;
}

void Airport::resume() {
    this->m_pause = false;
}

void Airport::resetData() {
    std::lock_guard guard(this->m_lock);
    this->m_flights.clear();
}

void Airport::updateFromEuroscope(types::Flight_t& flight) {
    if (true == this->m_pause)
        return;

    std::lock_guard guard(this->m_lock);

    bool found = false;
    for (auto& pair : this->m_flights) {
        if (flight.callsign == pair.first) {
            const auto oldUpdateTime = pair.second[FlightEuroscope].lastUpdate;
            pair.second[FlightEuroscope] = flight;
            pair.second[FlightEuroscope].lastUpdate = oldUpdateTime;
            found = true;
            break;
        }
    }

    if (found == false) {
        flight.lastUpdate = std::chrono::utc_clock::now();
        this->m_flights.insert({ flight.callsign, { flight, flight, types::Flight_t() } });
    }
}

const std::string& Airport::airport() const {
    return this->m_airport;
}

void Airport::flightDisconnected(const std::string& callsign) {
    if (true == this->m_pause)
        return;

    std::lock_guard guard(this->m_lock);

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        Json::Value root;

        root["callsign"] = callsign;
        root["inactive"] = true;
        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Callsign disconnected: " + callsign);
        Server::instance().patchFlight(callsign, root);

        it->second[FlightEuroscope].inactive = true;
    }
    this->m_manualUpdatePerformance.stop();
}

std::string Airport::timestampToIsoString(const std::chrono::utc_clock::time_point& timepoint) {
    if (timepoint.time_since_epoch().count() >= 0) {
        std::stringstream stream;
        stream << std::format("{0:%FT%T}", timepoint);
        auto timestamp = stream.str();
        timestamp = timestamp.substr(0, timestamp.length() - 4) + "Z";
        return timestamp;
    }
    else {
        return "1969-12-31T23:59:59.999Z";
    }
}

void Airport::updateExot(const std::string& callsign, const std::chrono::utc_clock::time_point& exot) {
    if (true == this->m_pause)
        return;

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        Json::Value root;

        root["callsign"] = callsign;
        root["vacdm"] = Json::Value();
        root["vacdm"]["exot"] = std::chrono::duration_cast<std::chrono::minutes>(exot.time_since_epoch()).count();
        root["vacdm"]["tsat"] = Airport::timestampToIsoString(types::defaultTime);
        root["vacdm"]["ttot"] = Airport::timestampToIsoString(types::defaultTime);
        root["vacdm"]["asat"] = Airport::timestampToIsoString(types::defaultTime);
        root["vacdm"]["aobt"] = Airport::timestampToIsoString(types::defaultTime);
        root["vacdm"]["atot"] = Airport::timestampToIsoString(types::defaultTime);

        it->second[FlightEuroscope].lastUpdate = std::chrono::utc_clock::now();
        it->second[FlightConsolidated].tsat = types::defaultTime;
        it->second[FlightConsolidated].ttot = types::defaultTime;
        it->second[FlightConsolidated].exot = types::defaultTime;
        it->second[FlightConsolidated].asat = types::defaultTime;
        it->second[FlightConsolidated].aobt = types::defaultTime;
        it->second[FlightConsolidated].atot = types::defaultTime;

        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Updating EXOT: " + callsign + ", " + root["vacdm"]["exot"].asString());

        std::lock_guard asyncGuard(this->m_asynchronousMessageLock);
        this->m_asynchronousMessages.push_back({
            SendType::Patch,
            callsign,
            root,
        });
    }

    this->m_manualUpdatePerformance.stop();
}

void Airport::updateTobt(const std::string& callsign, const std::chrono::utc_clock::time_point& tobt, bool manualTobt) {
    if (true == this->m_pause)
        return;

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        bool resetTsat = (tobt == types::defaultTime && true == manualTobt) || tobt >= it->second[FlightConsolidated].tsat;
        Json::Value root;

        root["callsign"] = callsign;
        root["vacdm"] = Json::Value();
        root["vacdm"]["tobt"] = Airport::timestampToIsoString(tobt);
        if (true == resetTsat)
            root["vacdm"]["tsat"] = Airport::timestampToIsoString(types::defaultTime);
        if (false == manualTobt)
            root["vacdm"]["tobt_state"] = "CONFIRMED";
        root["vacdm"]["ttot"] = Airport::timestampToIsoString(types::defaultTime);
        root["vacdm"]["asat"] = Airport::timestampToIsoString(types::defaultTime);
        root["vacdm"]["aobt"] = Airport::timestampToIsoString(types::defaultTime);
        root["vacdm"]["atot"] = Airport::timestampToIsoString(types::defaultTime);

        it->second[FlightEuroscope].lastUpdate = std::chrono::utc_clock::now();
        it->second[FlightConsolidated].tobt = tobt;
        if (true == resetTsat)
            it->second[FlightConsolidated].tsat = types::defaultTime;
        it->second[FlightConsolidated].ttot = types::defaultTime;
        it->second[FlightConsolidated].exot = types::defaultTime;
        it->second[FlightConsolidated].asat = types::defaultTime;
        it->second[FlightConsolidated].aobt = types::defaultTime;
        it->second[FlightConsolidated].atot = types::defaultTime;

        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Updating TOBT: " + callsign + ", " + root["vacdm"]["tobt"].asString());

        std::lock_guard asyncGuard(this->m_asynchronousMessageLock);
        this->m_asynchronousMessages.push_back({
            SendType::Patch,
            callsign,
            root,
        });
    }
    this->m_manualUpdatePerformance.stop();
}

void Airport::updateAsat(const std::string& callsign, const std::chrono::utc_clock::time_point& asat) {
    if (true == this->m_pause)
        return;

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        Json::Value root;

        root["callsign"] = callsign;
        root["vacdm"] = Json::Value();
        root["vacdm"]["asat"] = Airport::timestampToIsoString(asat);

        it->second[FlightEuroscope].lastUpdate = std::chrono::utc_clock::now();
        it->second[FlightConsolidated].asat = asat;

        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Updating ASAT: " + callsign + ", " + root["vacdm"]["asat"].asString());
        Server::instance().patchFlight(callsign, root);
    }
    this->m_manualUpdatePerformance.stop();
}

void Airport::updateAobt(const std::string& callsign, const std::chrono::utc_clock::time_point& aobt) {
    if (true == this->m_pause)
        return;

    std::lock_guard guard(this->m_lock);

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        Json::Value root;

        root["callsign"] = callsign;
        root["vacdm"] = Json::Value();
        root["vacdm"]["aobt"] = Airport::timestampToIsoString(aobt);

        it->second[FlightEuroscope].lastUpdate = std::chrono::utc_clock::now();
        it->second[FlightConsolidated].aobt = aobt;

        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Updating AOBT: " + callsign + ", " + root["vacdm"]["aobt"].asString());

        std::lock_guard asyncGuard(this->m_asynchronousMessageLock);
        this->m_asynchronousMessages.push_back({
            SendType::Patch,
            callsign,
            root,
        });
    }
    this->m_manualUpdatePerformance.stop();
}

void Airport::updateAtot(const std::string& callsign, const std::chrono::utc_clock::time_point& atot) {
    if (true == this->m_pause)
        return;

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        Json::Value root;

        root["callsign"] = callsign;
        root["vacdm"] = Json::Value();
        root["vacdm"]["atot"] = Airport::timestampToIsoString(atot);

        it->second[FlightEuroscope].lastUpdate = std::chrono::utc_clock::now();
        it->second[FlightConsolidated].atot = atot;

        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Updating ATOT: " + callsign + ", " + root["vacdm"]["atot"].asString());

        std::lock_guard asyncGuard(this->m_asynchronousMessageLock);
        this->m_asynchronousMessages.push_back({
            SendType::Patch,
            callsign,
            root,
        });
    }
    this->m_manualUpdatePerformance.stop();
}

void Airport::updateAsrt(const std::string& callsign, const std::chrono::utc_clock::time_point& asrt) {
    if (true == this->m_pause)
        return;

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        Json::Value root;

        root["callsign"] = callsign;
        root["vacdm"] = Json::Value();
        root["vacdm"]["asrt"] = Airport::timestampToIsoString(asrt);

        it->second[FlightEuroscope].lastUpdate = std::chrono::utc_clock::now();
        it->second[FlightConsolidated].asrt = asrt;

        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Updating ASRT: " + callsign + ", " + root["vacdm"]["asrt"].asString());

        std::lock_guard asyncGuard(this->m_asynchronousMessageLock);
        this->m_asynchronousMessages.push_back({
            SendType::Patch,
            callsign,
            root,
        });
    }
    this->m_manualUpdatePerformance.stop();
}

void Airport::updateAort(const std::string& callsign, const std::chrono::utc_clock::time_point& aort) {
    if (true == this->m_pause)
        return;

    this->m_manualUpdatePerformance.start();
    auto it = this->m_flights.find(callsign);
    if (it != this->m_flights.end() && it->second[FlightServer].callsign == callsign) {
        Json::Value root;

        root["callsign"] = callsign;
        root["vacdm"] = Json::Value();
        root["vacdm"]["aort"] = Airport::timestampToIsoString(aort);

        it->second[FlightEuroscope].lastUpdate = std::chrono::utc_clock::now();
        it->second[FlightConsolidated].aort = aort;

        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "Updating AORT: " + callsign + ", " + root["vacdm"]["aort"].asString());

        std::lock_guard asyncGuard(this->m_asynchronousMessageLock);
        this->m_asynchronousMessages.push_back({
            SendType::Patch,
            callsign,
            root,
        });
    }
    this->m_manualUpdatePerformance.stop();
}

Airport::SendType Airport::deltaEuroscopeToBackend(const std::array<types::Flight_t, 3>& data, Json::Value& root) {
    root.clear();

    root["callsign"] = data[FlightEuroscope].callsign;

    if (data[FlightServer].callsign == "" && data[FlightEuroscope].callsign != "") {
        root["inactive"] = false;

        root["position"] = Json::Value();
        root["position"]["lat"] = data[FlightEuroscope].latitude;
        root["position"]["lon"] = data[FlightEuroscope].longitude;

        root["flightplan"] = Json::Value();
        root["flightplan"]["flight_rules"] = data[FlightEuroscope].rule;
        root["flightplan"]["departure"] = data[FlightEuroscope].origin;
        root["flightplan"]["arrival"] = data[FlightEuroscope].destination;

        root["vacdm"] = Json::Value();
        root["vacdm"]["eobt"] = Airport::timestampToIsoString(data[FlightEuroscope].eobt);
        root["vacdm"]["tobt"] = Airport::timestampToIsoString(data[FlightEuroscope].tobt);

        root["clearance"] = Json::Value();
        root["clearance"]["dep_rwy"] = data[FlightEuroscope].runway;
        root["clearance"]["sid"] = data[FlightEuroscope].sid;
        root["clearance"]["initial_climb"] = data[FlightEuroscope].initialClimb;
        root["clearance"]["assigned_squawk"] = data[FlightEuroscope].assignedSquawk;
        root["clearance"]["current_squawk"] = data[FlightEuroscope].currentSquawk;

        return Airport::SendType::Post;
    }
    else {
        int deltaCount = 0;

        if (data[FlightEuroscope].inactive != data[FlightServer].inactive) {
            root["inactive"] = data[FlightEuroscope].inactive;
            deltaCount += 1;
        }

        auto lastDelta = deltaCount;
        root["position"] = Json::Value();
        if (data[FlightEuroscope].latitude != data[FlightServer].latitude) {
            root["position"]["lat"] = data[FlightEuroscope].latitude;
            deltaCount += 1;
        }
        if (data[FlightEuroscope].longitude != data[FlightServer].longitude) {
            root["position"]["lon"] = data[FlightEuroscope].longitude;
            deltaCount += 1;
        }
        if (deltaCount == lastDelta)
            root.removeMember("position");

        lastDelta = deltaCount;
        root["flightplan"] = Json::Value();
        if (data[FlightEuroscope].origin != data[FlightServer].origin) {
            root["flightplan"]["departure"] = data[FlightEuroscope].origin;
            deltaCount += 1;
        }
        if (data[FlightEuroscope].destination != data[FlightServer].destination) {
            root["flightplan"]["arrival"] = data[FlightEuroscope].destination;
            deltaCount += 1;
        }
        if (lastDelta == deltaCount)
            root.removeMember("flightplan");

        lastDelta = deltaCount;
        root["clearance"] = Json::Value();
        if (data[FlightEuroscope].runway != data[FlightServer].runway) {
            root["clearance"]["dep_rwy"] = data[FlightEuroscope].runway;
            deltaCount += 1;
        }
        if (data[FlightEuroscope].sid != data[FlightServer].sid) {
            root["clearance"]["sid"] = data[FlightEuroscope].sid;
            deltaCount += 1;
        }
        if (data[FlightEuroscope].initialClimb != data[FlightServer].initialClimb) {
            root["clearance"]["initial_climb"] = data[FlightEuroscope].initialClimb;
            deltaCount += 1;
        }
        if (data[FlightEuroscope].assignedSquawk != data[FlightServer].assignedSquawk) {
            root["clearance"]["assigned_squawk"] = data[FlightEuroscope].assignedSquawk;
            deltaCount += 1;
        }
        if (data[FlightEuroscope].currentSquawk != data[FlightServer].currentSquawk) {
            root["clearance"]["current_squawk"] = data[FlightEuroscope].currentSquawk;
            deltaCount += 1;
        }
        if (lastDelta == deltaCount)
            root.removeMember("clearance");

        return deltaCount != 0 ? Airport::SendType::Patch : Airport::SendType::None;
    }
}

void Airport::consolidateData(std::array<types::Flight_t, 3>& data) {
    if (data[FlightEuroscope].callsign == data[FlightServer].callsign) {
        data[FlightConsolidated].inactive = data[FlightServer].inactive;

        data[FlightConsolidated].latitude = data[FlightEuroscope].latitude;
        data[FlightConsolidated].longitude = data[FlightEuroscope].longitude;

        data[FlightConsolidated].origin = data[FlightEuroscope].origin;
        data[FlightConsolidated].destination = data[FlightEuroscope].destination;
        data[FlightConsolidated].rule = data[FlightEuroscope].rule;

        data[FlightConsolidated].lastUpdate = data[FlightServer].lastUpdate;
        data[FlightConsolidated].eobt = data[FlightServer].eobt;
        data[FlightConsolidated].tobt = data[FlightServer].tobt;
        data[FlightConsolidated].ctot = data[FlightServer].ctot;
        data[FlightConsolidated].ttot = data[FlightServer].ttot;
        data[FlightConsolidated].tsat = data[FlightServer].tsat;
        data[FlightConsolidated].exot = data[FlightServer].exot;
        data[FlightConsolidated].asat = data[FlightServer].asat;
        data[FlightConsolidated].aobt = data[FlightServer].aobt;
        data[FlightConsolidated].atot = data[FlightServer].atot;
        data[FlightConsolidated].aort = data[FlightServer].aort;
        data[FlightConsolidated].asrt = data[FlightServer].asrt;
        data[FlightConsolidated].tobt_state = data[FlightServer].tobt_state;
        
        data[FlightConsolidated].hasBooking = data[FlightServer].hasBooking;

        data[FlightConsolidated].runway = data[FlightEuroscope].runway;
        data[FlightConsolidated].sid = data[FlightEuroscope].sid;
        data[FlightConsolidated].assignedSquawk = data[FlightEuroscope].assignedSquawk;
        data[FlightConsolidated].currentSquawk = data[FlightEuroscope].currentSquawk;
        data[FlightConsolidated].initialClimb = data[FlightEuroscope].initialClimb;

        data[FlightConsolidated].departed = data[FlightEuroscope].departed;
        logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug, "Consolidated " + data[FlightServer].callsign);
    }
    else {
        logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug,
            "Invalid callsign match in consolidation: " + data[FlightEuroscope].callsign + ", " + data[FlightServer].callsign);
    }
}

void Airport::processAsynchronousMessages() {
    // get a snapshot of the message count to avoid endless loops during long processing times
    this->m_asynchronousMessageLock.lock();
    std::size_t messageCount = this->m_asynchronousMessages.size();
    this->m_asynchronousMessageLock.unlock();

    for (std::size_t i = 0; i < messageCount; ++i) {
        // get the first message
        this->m_asynchronousMessageLock.lock();
        // double check for empty queues
        if (this->m_asynchronousMessages.size() == 0) {
            this->m_asynchronousMessageLock.unlock();
            break;
        }

        AsynchronousMessage message = this->m_asynchronousMessages.front();
        this->m_asynchronousMessages.pop_front();
        this->m_asynchronousMessageLock.unlock();

        if (message.type == SendType::Patch) {
            Server::instance().patchFlight(message.callsign, message.content);
        }
        else if (message.type == SendType::Post) {
            Server::instance().postFlight(message.content);
        }
    }
}

void Airport::run() {
    std::size_t counter = 1;
    while (true) {
        std::this_thread::sleep_for(1s);
        if (true == this->m_stop)
            return;
        // run every five seconds
        if (counter++ % 5 != 0)
            continue;
        // pause until master resumed
        if (true == this->m_pause)
            continue;

        // process the backlog queue of asynchronous messages
        this->processAsynchronousMessages();

        // get the newest updates
        this->m_workerAllFlightsPerformance.start();
        logging::Logger::instance().log("Airport", logging::Logger::Level::Debug, "New server update cycle");
        auto flights = com::Server::instance().allFlights(this->m_airport);
        std::list<std::tuple<std::string, Airport::SendType, Json::Value>> transmissionBuffer;
        this->m_workerAllFlightsPerformance.stop();

        this->m_lock.lock();
        // check which updates are needed and update consolidated views based on the server
        for (auto it = this->m_flights.begin(); this->m_flights.end() != it;) {
            if (it->second[FlightEuroscope].callsign.length() == 0 || true == it->second[FlightConsolidated].departed) {
                logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug,
                    "Skipping flight\nCallsigns: " +
                    it->first + ", " + it->second[FlightEuroscope].callsign + "\nDeparted: " +
                    std::to_string(it->second[FlightConsolidated].departed)
                );
                ++it;
                continue;
            }

            if (false == it->second[FlightEuroscope].departed && it->second[FlightConsolidated].atot.time_since_epoch().count() > 0) {
                logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug,
                    "ATOT of " + it->first + " indicates departure: " +
                    std::to_string(it->second[FlightConsolidated].atot.time_since_epoch().count())
                );
                it->second[FlightEuroscope].departed = true;
            }

            bool removeFlight = it->second[FlightServer].inactive == true;
            for (auto rit = flights.begin(); flights.end() != rit; ++rit) {
                if (rit->callsign == it->second[FlightConsolidated].callsign) {
                    it->second[FlightServer] = *rit;
                    Airport::consolidateData(it->second);
                    removeFlight = false;
                    break;
                }
            }

            Json::Value root;
            const auto sendType = Airport::deltaEuroscopeToBackend(it->second, root);
            if (Airport::SendType::None != sendType)
                transmissionBuffer.push_back({ it->first, sendType, root });

            if (true == removeFlight) {
                logging::Logger::instance().log("vACDM", logging::Logger::Level::Debug, "Deleting " + it->first);
                it = this->m_flights.erase(it);
            }
            else {
                ++it;
            }
        }

        this->m_lock.unlock();

        // send the deltas
        this->m_workerUpdateFlightsPerformance.start();
        for (const auto& transmission : std::as_const(transmissionBuffer)) {
            if (std::get<1>(transmission) == Airport::SendType::Post)
                Server::instance().postFlight(std::get<2>(transmission));
            else
                Server::instance().patchFlight(std::get<0>(transmission), std::get<2>(transmission));
        }
        this->m_workerUpdateFlightsPerformance.stop();
    }
}

bool Airport::flightExists(const std::string& callsign) {
    if (true == this->m_pause)
        return false;

    std::lock_guard guard(this->m_lock);
    return this->m_flights.cend() != this->m_flights.find(callsign);
}

const types::Flight_t& Airport::flight(const std::string& callsign) {
    std::lock_guard guard(this->m_lock);
    return this->m_flights.find(callsign)->second[FlightConsolidated];
}
