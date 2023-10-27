#include "color.h"

using namespace std::chrono_literals;
using namespace vacdm;

Color::Color() :
    m_pluginConfig() {
    this->m_worker = std::thread(&Color::run, this);
}

void Color::run() {
    std::size_t counter = 1;
    while (true) {
        if (m_stop == true)
            return;
        std::this_thread::sleep_for(1s);
        // run every two seconds
        if (counter++ % 2 != 0)
            continue;

        // toggle show highlight color
        this->m_flash = !(this->m_flash);
    }
}

Color::~Color() {
    this->m_stop = true;
    this->m_worker.join();
}

// Times:

COLORREF Color::colorizeEobtAndTobt(const types::Flight_t& flight) const
{
    // return highlight color
    if (this->m_flash == true) {
        return this->m_pluginConfig.debug;
    }

    const auto now = std::chrono::utc_clock::now();
    const auto timeSinceTobt = std::chrono::duration_cast<std::chrono::seconds>(now - flight.tobt).count();
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(now - flight.tsat).count();
    const auto diffTsatTobt = std::chrono::duration_cast<std::chrono::seconds>(flight.tsat - flight.tobt).count();

    if (flight.tsat == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }
    // ASAT exists
    if (flight.asat.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    // TOBT in past && TSAT expired, i.e. 5min past TSAT || TOBT >= +1h || TSAT does not exist && TOBT in past
    // -> TOBT in past && (TSAT expired || TSAT does not exist) || TOBT >= now + 1h
    if (timeSinceTobt > 0 && (timeSinceTsat >= 5 * 60 || flight.tsat == types::defaultTime) || flight.tobt >= now + std::chrono::hours(1)) //last statement could cause problems
    {
        return this->m_pluginConfig.orange;
    }
    // Diff TOBT TSAT >= 5min && unconfirmed
    if (diffTsatTobt >= 5 * 60 && (flight.tobt_state == "GUESS" || flight.tobt_state == "FLIGHTPLAN"))
    {
        return this->m_pluginConfig.lightyellow;
    }
    // Diff TOBT TSAT >= 5min && confirmed
    if (diffTsatTobt >= 5 * 60 && flight.tobt_state == "CONFIRMED")
    {
        return this->m_pluginConfig.yellow;
    }
    // Diff TOBT TSAT < 5min
    if (diffTsatTobt < 5 * 60 && flight.tobt_state == "CONFIRMED")
    {
        return this->m_pluginConfig.green;
    }
    // tobt is not confirmed
    if (flight.tobt_state != "CONFIRMED")
    {
        return this->m_pluginConfig.lightgreen;
    }
    return this->m_pluginConfig.debug;
}

COLORREF Color::colorizeTsat(const types::Flight_t& flight) const {
    if (flight.asat != types::defaultTime || flight.tsat == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.tsat).count();
    if (timeSinceTsat <= 5 * 60 && timeSinceTsat >= -5 * 60)
    {
        if (flight.ctot.time_since_epoch().count() > 0)
        {
            // CTOT exists
            return this->m_pluginConfig.blue;
        }
        return this->m_pluginConfig.green;
    }
    // TSAT earlier than 5+ min
    if (timeSinceTsat < -5 * 60)
    {
        if (flight.ctot.time_since_epoch().count() > 0)
        {
            // CTOT exists
            return this->m_pluginConfig.lightblue;
        }
        return this->m_pluginConfig.lightgreen;
    }
    // TSAT passed by 5+ min
    if (timeSinceTsat > 5 * 60)
    {
        if (flight.ctot.time_since_epoch().count() > 0)
        {
            // CTOT exists
            return this->m_pluginConfig.red;
        }
        return this->m_pluginConfig.orange;
    }
    return this->m_pluginConfig.debug;
}

COLORREF Color::colorizeTtot(const types::Flight_t& flight) const {
    if (flight.ttot == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }

    auto now = std::chrono::utc_clock::now();

    // Round up to the next 10, 20, 30, 40, 50, or 00 minute interval
    auto timeSinceEpoch = flight.ttot.time_since_epoch();
    auto minutesSinceEpoch = std::chrono::duration_cast<std::chrono::minutes>(timeSinceEpoch);
    std::chrono::time_point<std::chrono::utc_clock> rounded;

    // Compute the number of minutes remaining to the next highest ten
    auto remainingMinutes = 10 - minutesSinceEpoch.count() % 10;

    // If the time point is already at a multiple of ten minutes, no rounding is needed
    if (remainingMinutes == 10) {
        rounded = std::chrono::time_point_cast<std::chrono::minutes>(flight.ttot);
    }
    else {
        // Add the remaining minutes to the time point
        auto roundedUpMinutes = minutesSinceEpoch + std::chrono::minutes(remainingMinutes);

        // Convert back to a time_point object and return
        rounded = std::chrono::time_point_cast<std::chrono::minutes>(std::chrono::utc_clock::time_point(roundedUpMinutes));
        rounded += std::chrono::seconds(30);
    }

    // Check if the current time has passed the ttot time point
    if (flight.atot.time_since_epoch().count() > 0)
    {
        // ATOT exists
        return this->m_pluginConfig.grey;
    }
    if (now < rounded)
    {
        // time before TTOT and during TTOT block
        return this->m_pluginConfig.green;
    }
    else if (now >= rounded)
    {
        // time past TTOT / TTOT block
        return this->m_pluginConfig.orange;
    }
    return this->m_pluginConfig.debug;
}

COLORREF Color::colorizeAort(const types::Flight_t& flight) const {
    if (flight.aort == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }
    if (flight.aobt.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    const auto timeSinceAort = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.aort).count();

    if (timeSinceAort <= 5 * 60 && timeSinceAort >= 0)
    {
        return this->m_pluginConfig.green;
    }
    if (timeSinceAort > 5 * 60 && timeSinceAort <= 10 * 60)
    {
        return this->m_pluginConfig.yellow;
    }
    if (timeSinceAort > 10 * 60 && timeSinceAort <= 15 * 60)
    {
        return this->m_pluginConfig.orange;
    }
    if (timeSinceAort > 15 * 60)
    {
        return this->m_pluginConfig.red;
    }

    return this->m_pluginConfig.debug;
}

COLORREF Color::colorizeAsrt(const types::Flight_t& flight) const {

    if (flight.asat.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    const auto timeSinceAsrt = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.asrt).count();
    if (timeSinceAsrt <= 5 * 60 && timeSinceAsrt >= 0)
    {
        return this->m_pluginConfig.green;
    }
    if (timeSinceAsrt > 5 * 60 && timeSinceAsrt <= 10 * 60)
    {
        return this->m_pluginConfig.yellow;
    }
    if (timeSinceAsrt > 10 * 60 && timeSinceAsrt <= 15 * 60)
    {
        return this->m_pluginConfig.orange;
    }
    if (timeSinceAsrt > 15 * 60)
    {
        return this->m_pluginConfig.red;
    }

    return this->m_pluginConfig.debug;
}

COLORREF Color::colorizeAsat(const types::Flight_t& flight) const {
    if (flight.asat == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }

    if (flight.aobt.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }

    const auto timeSinceAsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.asat).count();
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.tsat).count();
    if (flight.taxizoneIsTaxiout == false)
    {

        if (/* Datalink clearance == true &&*/ timeSinceTsat >= -5 * 60 && timeSinceTsat <= 5 * 60)
        {
            return this->m_pluginConfig.green;
        }
        if (timeSinceAsat < 5 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    if (flight.taxizoneIsTaxiout == true)
    {
        if (timeSinceTsat >= -5 * 60 && timeSinceTsat <= 10 * 60 /* && Datalink clearance == true*/)
        {
            return this->m_pluginConfig.green;
        }
        if (timeSinceAsat < 10 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    return this->m_pluginConfig.orange;
}

// Timers:

COLORREF Color::colorizeAsatTimer(const types::Flight_t& flight) const {
    // aort set
    if (flight.aort.time_since_epoch().count() > 0)
    {
        return this->m_pluginConfig.grey;
    }
    const auto timeSinceAobt = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.aobt).count();
    if (timeSinceAobt >= 0)
    {
        // hide Timer
    }
    const auto timeSinceAsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.asat).count();
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.tsat).count();
    // Pushback required
    if (flight.taxizoneIsTaxiout != false)
    {
        /*
        if (hasdatalinkclearance == true && timesincetsat >=5*60 && timesincetsat <=5*60)
        {
            return this->m_pluginConfig.green
        } */
        if (timeSinceAsat < 5 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    if (flight.taxizoneIsTaxiout == true)
    {
        if (timeSinceTsat >= -5 * 60 && timeSinceTsat <= 10 * 60)
        {
            return this->m_pluginConfig.green;
        }
        if (timeSinceAsat <= 10 * 60)
        {
            return this->m_pluginConfig.green;
        }
    }
    return this->m_pluginConfig.orange;
}

COLORREF Color::colorizeCtotandCtottimer(const types::Flight_t& flight) const {
    if (flight.ctot == types::defaultTime)
    {
        return this->m_pluginConfig.grey;
    }

    const auto timetoctot = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - flight.ctot).count();
    if (timetoctot >= 5 * 60)
    {
        return this->m_pluginConfig.lightgreen;
    }
    if (timetoctot <= 5 * 60 && timetoctot >= -10 * 60)
    {
        return this->m_pluginConfig.green;
    }
    if (timetoctot < -10 * 60)
    {
        return this->m_pluginConfig.orange;
    }

    return this->m_pluginConfig.grey;
}

void Color::changePluginConfig(const SystemConfig newPluginConfig) {
    this->m_pluginConfig = newPluginConfig;
}

Color& vacdm::Color::instance()
{
    static Color __instance;
    return __instance;
}