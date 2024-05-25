#include <algorithm>
#include <string>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include "backend/Server.h"
#include "config/ConfigHandler.h"
#include "core/DataManager.h"
#include "log/Logger.h"
#include "utils/Number.h"
#include "utils/String.h"
#include "vACDM.h"


using namespace vacdm;
using namespace vacdm::logging;
using namespace vacdm::core;
using namespace vacdm::utils;

namespace vacdm {
bool vACDM::OnCompileCommand(const char *sCommandLine) {
    std::string command(sCommandLine);

#pragma warning(push)
#pragma warning(disable : 4244)
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
#pragma warning(pop)

    // only handle commands containing ".vacdm"
    if (0 != command.find(".VACDM")) return false;

    // master command
    if (std::string::npos != command.find("MASTER")) {
        bool userIsConnected = this->GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_NO;
        bool userIsInSweatbox = this->GetConnectionType() == EuroScopePlugIn::CONNECTION_TYPE_SWEATBOX;
        bool userIsObserver = std::string_view(this->ControllerMyself().GetCallsign()).ends_with("_OBS") == true ||
                              this->ControllerMyself().GetFacility() == 0;
        bool serverAllowsObsAsMaster = com::Server::instance().getServerConfig().allowMasterAsObserver;
        bool serverAllowsSweatboxAsMaster = com::Server::instance().getServerConfig().allowMasterInSweatbox;

        std::string userIsNotEligibleMessage;

        if (!userIsConnected) {
            userIsNotEligibleMessage = "You are not logged in to the VATSIM network";
        } else if (userIsObserver && !serverAllowsObsAsMaster) {
            userIsNotEligibleMessage = "You are logged in as Observer and Server does not allow Observers to be Master";
        } else if (userIsInSweatbox && !serverAllowsSweatboxAsMaster) {
            userIsNotEligibleMessage =
                "You are logged in on a Sweatbox Server and Server does not allow Sweatbox connections";
        } else {
            DisplayMessage("Executing vACDM as the MASTER");
            Logger::instance().log(Logger::LogSender::vACDM, "Switched to MASTER", Logger::LogLevel::Info);
            com::Server::instance().setMaster(true);

            return true;
        }

        DisplayMessage("Cannot upgrade to Master");
        DisplayMessage(userIsNotEligibleMessage);
        return true;
    } else if (std::string::npos != command.find("SLAVE")) {
        DisplayMessage("Executing vACDM as the SLAVE");
        Logger::instance().log(Logger::LogSender::vACDM, "Switched to SLAVE", Logger::LogLevel::Info);
        com::Server::instance().setMaster(false);
        return true;
    } else if (std::string::npos != command.find("RELOAD")) {
        ConfigHandler::instance().reloadConfig();
        return true;
    } else if (std::string::npos != command.find("LOG")) {
        if (std::string::npos != command.find("LOGLEVEL")) {
            DisplayMessage(Logger::instance().handleLogLevelCommand(command));
        } else {
            DisplayMessage(Logger::instance().handleLogCommand(command));
        }
        return true;
    } else if (std::string::npos != command.find("UPDATERATE")) {
        const auto elements = vacdm::utils::String::splitString(command, " ");
        if (elements.size() != 3) {
            DisplayMessage("Usage: .vacdm UPDATERATE value");
            return true;
        }
        if (false == isNumber(elements[2]) ||
            std::stoi(elements[2]) < minUpdateCycleSeconds && std::stoi(elements[2]) > maxUpdateCycleSeconds) {
            DisplayMessage("Usage: .vacdm UPDATERATE value");
            DisplayMessage("Value must be number between " + std::to_string(minUpdateCycleSeconds) + " and " +
                           std::to_string(maxUpdateCycleSeconds));
            return true;
        }

        ConfigHandler::instance().changeUpdateCycle(std::stoi(elements[2]));

        return true;
    }
    return false;
}
}  // namespace vacdm