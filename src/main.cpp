#include <memory>
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include "config/ConfigHandler.h"
#include "config/IConfigHandler.h"
#include "core/DataManager.h"
#include "core/Server.h"
#include "log/ConsoleLogger.h"
#include "log/ILogger.h"
#include "log/SqlLiteLogger.h"
#include "main.h"
#include "utils/File.h"
#include "vACDM.h"

std::unique_ptr<vacdm::vACDM> Plugin;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance) {
    std::shared_ptr<vacdm::log::ILogger> logger =
        std::make_shared<vacdm::log::SqlLiteLogger>(::utils::file::GetDllDirectoryPathFs());
    auto server = std::make_shared<vacdm::com::Server>(logger);
    auto datamanager = std::make_shared<vacdm::core::DataManager>(server, logger);
    std::shared_ptr<interfaces::IConfigHandler> confighandler =
        std::make_shared<config::ConfigHandler>(::utils::file::GetDllDirectoryPathFs() / "vacdm.txt");

    Plugin.reset(new vacdm::vACDM(server, datamanager, logger, confighandler));
    *ppPlugInInstance = Plugin.get();
}

void __declspec(dllexport) EuroScopePlugInExit(void) {}
