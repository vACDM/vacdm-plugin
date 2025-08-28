#include <memory>
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

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
    std::shared_ptr<vacdm::log::ILogger> logger = std::make_shared<vacdm::log::ConsoleLogger>();
    auto server = std::make_shared<vacdm::com::Server>(logger);
    auto datamanager = std::make_shared<vacdm::core::DataManager>(server, logger);

    Plugin.reset(new vacdm::vACDM(server, datamanager, logger));
    *ppPlugInInstance = Plugin.get();
}

void __declspec(dllexport) EuroScopePlugInExit(void) {}
