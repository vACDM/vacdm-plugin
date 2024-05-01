#include <memory>
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include "main.h"
#include "vACDM.h"

std::unique_ptr<vacdm::vACDM> Plugin;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance) {
    Plugin.reset(new vacdm::vACDM());
    *ppPlugInInstance = Plugin.get();
}

void __declspec(dllexport) EuroScopePlugInExit(void) {}
