#include <memory>
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include "vACDM.h"

std::unique_ptr<EuroScopePlugIn::CPlugIn> Plugin;

void    __declspec (dllexport)    EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance)
{
    Plugin.reset(new vacdm::vACDM());
    *ppPlugInInstance = Plugin.get();
}

void    __declspec (dllexport)    EuroScopePlugInExit(void)
{
}
