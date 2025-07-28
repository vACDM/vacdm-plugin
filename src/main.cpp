#include <memory>
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include "core/DataManager.h"
#include "core/Server.h"
#include "main.h"
#include "vACDM.h"

std::unique_ptr<vacdm::vACDM> Plugin;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance) {
    auto server = std::make_shared<vacdm::com::Server>();
    auto datamanager = std::make_shared<vacdm::core::DataManager>(server);

    Plugin.reset(new vacdm::vACDM(server, datamanager));
    *ppPlugInInstance = Plugin.get();
}

void __declspec(dllexport) EuroScopePlugInExit(void) {}
