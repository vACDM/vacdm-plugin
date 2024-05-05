#include "TagItemsColor.h"

using namespace vacdm;
using namespace vacdm::tagitems;

Color::Color() { m_pluginConfig = ConfigHandler::instance().subscribe(this); }

Color::~Color() { ConfigHandler::instance().unsubscribe(this); }

Color &Color::instance() {
    static Color __instance;
    return __instance;
}

void Color::onConfigUpdate(PluginConfig pluginConfig) { m_pluginConfig = m_pluginConfig; }

COLORREF Color::colorizeEobt(const types::Pilot &pilot) { return colorizeEobtAndTobt(pilot); }

COLORREF Color::colorizeTobt(const types::Pilot &pilot) { return colorizeEobtAndTobt(pilot); }

COLORREF Color::colorizeTsat(const types::Pilot &pilot) {
    if (pilot.asat != types::defaultTime || pilot.tsat == types::defaultTime) {
        return m_pluginConfig.grey;
    }
    const auto timeSinceTsat =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.tsat).count();
    if (timeSinceTsat <= 5 * 60 && timeSinceTsat >= -5 * 60) {
        // CTOT exists
        if (pilot.ctot.time_since_epoch().count() > 0) {
            return m_pluginConfig.blue;
        }
        return m_pluginConfig.green;
    }
    // TSAT earlier than 5+ min
    if (timeSinceTsat < -5 * 60) {
        // CTOT exists
        if (pilot.ctot.time_since_epoch().count() > 0) {
            return m_pluginConfig.lightblue;
        }
        return m_pluginConfig.lightgreen;
    }
    // TSAT passed by 5+ min
    if (timeSinceTsat > 5 * 60) {
        // CTOT exists
        if (pilot.ctot.time_since_epoch().count() > 0) {
            return m_pluginConfig.red;
        }
        return m_pluginConfig.orange;
    }
    return m_pluginConfig.debug;
}

COLORREF Color::colorizeTtot(const types::Pilot &pilot) {
    if (pilot.ttot == types::defaultTime) {
        return m_pluginConfig.grey;
    }

    auto now = std::chrono::utc_clock::now();

    // Round up to the next 10, 20, 30, 40, 50, or 00 minute interval
    auto timeSinceEpoch = pilot.ttot.time_since_epoch();
    auto minutesSinceEpoch = std::chrono::duration_cast<std::chrono::minutes>(timeSinceEpoch);
    std::chrono::time_point<std::chrono::utc_clock> rounded;

    // Compute the number of minutes remaining to the next highest ten
    auto remainingMinutes = 10 - minutesSinceEpoch.count() % 10;

    // If the time point is already at a multiple of ten minutes, no rounding is needed
    if (remainingMinutes == 10) {
        rounded = std::chrono::time_point_cast<std::chrono::minutes>(pilot.ttot);
    } else {
        // Add the remaining minutes to the time point
        auto roundedUpMinutes = minutesSinceEpoch + std::chrono::minutes(remainingMinutes);

        // Convert back to a time_point object and return
        rounded =
            std::chrono::time_point_cast<std::chrono::minutes>(std::chrono::utc_clock::time_point(roundedUpMinutes));
        rounded += std::chrono::seconds(30);
    }

    // Check if the current time has passed the ttot time point
    if (pilot.atot.time_since_epoch().count() > 0) {
        // ATOT exists
        return m_pluginConfig.grey;
    }
    if (now < rounded) {
        // time before TTOT and during TTOT block
        return m_pluginConfig.green;
    } else if (now >= rounded) {
        // time past TTOT / TTOT block
        return m_pluginConfig.orange;
    }
    return m_pluginConfig.debug;
}

int Color::colorizeExot(const types::Pilot &pilot) {
    std::ignore = pilot;
    return EuroScopePlugIn::TAG_COLOR_DEFAULT;
}

COLORREF Color::colorizeAsat(const types::Pilot &pilot) {
    if (pilot.asat == types::defaultTime) {
        return m_pluginConfig.grey;
    }

    if (pilot.aobt.time_since_epoch().count() > 0) {
        return m_pluginConfig.grey;
    }

    const auto timeSinceAsat =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.asat).count();
    const auto timeSinceTsat =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.tsat).count();
    if (pilot.taxizoneIsTaxiout == false) {
        if (/* Datalink clearance == true &&*/ timeSinceTsat >= -5 * 60 && timeSinceTsat <= 5 * 60) {
            return m_pluginConfig.green;
        }
        if (timeSinceAsat < 5 * 60) {
            return m_pluginConfig.green;
        }
    }
    if (pilot.taxizoneIsTaxiout == true) {
        if (timeSinceTsat >= -5 * 60 && timeSinceTsat <= 10 * 60 /* && Datalink clearance == true*/) {
            return m_pluginConfig.green;
        }
        if (timeSinceAsat < 10 * 60) {
            return m_pluginConfig.green;
        }
    }
    return m_pluginConfig.orange;
}

COLORREF Color::colorizeAobt(const types::Pilot &pilot) {
    std::ignore = pilot;
    return m_pluginConfig.grey;
}

COLORREF Color::colorizeAtot(const types::Pilot &pilot) {
    std::ignore = pilot;
    return m_pluginConfig.grey;
}

COLORREF Color::colorizeAsrt(const types::Pilot &pilot) {
    if (pilot.asat.time_since_epoch().count() > 0) {
        return m_pluginConfig.grey;
    }
    const auto timeSinceAsrt =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.asrt).count();
    if (timeSinceAsrt <= 5 * 60 && timeSinceAsrt >= 0) {
        return m_pluginConfig.green;
    }
    if (timeSinceAsrt > 5 * 60 && timeSinceAsrt <= 10 * 60) {
        return m_pluginConfig.yellow;
    }
    if (timeSinceAsrt > 10 * 60 && timeSinceAsrt <= 15 * 60) {
        return m_pluginConfig.orange;
    }
    if (timeSinceAsrt > 15 * 60) {
        return m_pluginConfig.red;
    }

    return m_pluginConfig.debug;
}

COLORREF Color::colorizeAort(const types::Pilot &pilot) {
    if (pilot.aort == types::defaultTime) {
        return m_pluginConfig.grey;
    }
    if (pilot.aobt.time_since_epoch().count() > 0) {
        return m_pluginConfig.grey;
    }
    const auto timeSinceAort =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.aort).count();

    if (timeSinceAort <= 5 * 60 && timeSinceAort >= 0) {
        return m_pluginConfig.green;
    }
    if (timeSinceAort > 5 * 60 && timeSinceAort <= 10 * 60) {
        return m_pluginConfig.yellow;
    }
    if (timeSinceAort > 10 * 60 && timeSinceAort <= 15 * 60) {
        return m_pluginConfig.orange;
    }
    if (timeSinceAort > 15 * 60) {
        return m_pluginConfig.red;
    }

    return m_pluginConfig.debug;
}

COLORREF Color::colorizeCtot(const types::Pilot &pilot) { return colorizeCtotandCtottimer(pilot); }

COLORREF Color::colorizeCtotTimer(const types::Pilot &pilot) { return colorizeCtotandCtottimer(pilot); }

COLORREF Color::colorizeAsatTimer(const types::Pilot &pilot) {
    // aort set
    if (pilot.aort.time_since_epoch().count() > 0) {
        return m_pluginConfig.grey;
    }
    const auto timeSinceAobt =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.aobt).count();
    if (timeSinceAobt >= 0) {
        // hide Timer
    }
    const auto timeSinceAsat =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.asat).count();
    const auto timeSinceTsat =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.tsat).count();
    // Pushback required
    if (pilot.taxizoneIsTaxiout != false) {
        /*
        if (hasdatalinkclearance == true && timesincetsat >=5*60 && timesincetsat <=5*60)
        {
            return m_pluginConfig.green
        } */
        if (timeSinceAsat < 5 * 60) {
            return m_pluginConfig.green;
        }
    }
    if (pilot.taxizoneIsTaxiout == true) {
        if (timeSinceTsat >= -5 * 60 && timeSinceTsat <= 10 * 60) {
            return m_pluginConfig.green;
        }
        if (timeSinceAsat <= 10 * 60) {
            return m_pluginConfig.green;
        }
    }
    return m_pluginConfig.orange;
}

// other:

COLORREF Color::colorizeEcfmpMeasure(const types::Pilot &pilot) {
    return pilot.measures.empty() ? m_pluginConfig.grey : m_pluginConfig.green;
}

COLORREF Color::colorizeEventBooking(const types::Pilot &pilot) {
    return pilot.hasBooking ? m_pluginConfig.green : m_pluginConfig.grey;
}

COLORREF Color::colorizeEobtAndTobt(const types::Pilot &pilot) {
    const auto now = std::chrono::utc_clock::now();
    const auto timeSinceTobt = std::chrono::duration_cast<std::chrono::seconds>(now - pilot.tobt).count();
    const auto timeSinceTsat = std::chrono::duration_cast<std::chrono::seconds>(now - pilot.tsat).count();
    const auto diffTsatTobt = std::chrono::duration_cast<std::chrono::seconds>(pilot.tsat - pilot.tobt).count();

    if (pilot.tsat == types::defaultTime) {
        return m_pluginConfig.grey;
    }
    // ASAT exists
    if (pilot.asat.time_since_epoch().count() > 0) {
        return m_pluginConfig.grey;
    }
    // TOBT in past && TSAT expired, i.e. 5min past TSAT || TOBT >= +1h || TSAT does not exist && TOBT in past
    // -> TOBT in past && (TSAT expired || TSAT does not exist) || TOBT >= now + 1h
    if (timeSinceTobt > 0 && (timeSinceTsat >= 5 * 60 || pilot.tsat == types::defaultTime) ||
        pilot.tobt >= now + std::chrono::hours(1))  // last statement could cause problems
    {
        return m_pluginConfig.orange;
    }
    // Diff TOBT TSAT >= 5min && unconfirmed
    if (diffTsatTobt >= 5 * 60 && (pilot.tobt_state == "GUESS" || pilot.tobt_state == "FLIGHTPLAN")) {
        return m_pluginConfig.lightyellow;
    }
    // Diff TOBT TSAT >= 5min && confirmed
    if (diffTsatTobt >= 5 * 60 && pilot.tobt_state == "CONFIRMED") {
        return m_pluginConfig.yellow;
    }
    // Diff TOBT TSAT < 5min
    if (diffTsatTobt < 5 * 60 && pilot.tobt_state == "CONFIRMED") {
        return m_pluginConfig.green;
    }
    // tobt is not confirmed
    if (pilot.tobt_state != "CONFIRMED") {
        return m_pluginConfig.lightgreen;
    }
    return m_pluginConfig.debug;
}

COLORREF Color::colorizeCtotandCtottimer(const types::Pilot &pilot) {
    if (pilot.ctot == types::defaultTime) {
        return m_pluginConfig.grey;
    }

    const auto timetoctot =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now() - pilot.ctot).count();
    if (timetoctot >= 5 * 60) {
        return m_pluginConfig.lightgreen;
    }
    if (timetoctot <= 5 * 60 && timetoctot >= -10 * 60) {
        return m_pluginConfig.green;
    }
    if (timetoctot < -10 * 60) {
        return m_pluginConfig.orange;
    }

    return m_pluginConfig.grey;
}