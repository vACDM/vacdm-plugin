#include "vACDM.h"

#include <Windows.h>
#include <shlwapi.h>

#include "Version.h"
#include "log/Logger.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace vacdm::logging;

namespace vacdm {
vACDM::vACDM()
    : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE) {
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    Logger::instance().log(Logger::LogSender::vACDM, "Version " + std::string(PLUGIN_VERSION) + " loaded",
                           Logger::LogLevel::System);

    // get the dll-path
    char path[MAX_PATH + 1] = {0};
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    this->m_settingsPath = std::string(path) + "\\vacdm.txt";

}

vACDM::~vACDM() {}

void vACDM::DisplayMessage(const std::string &message, const std::string &sender) {
    DisplayUserMessage("vACDM", sender.c_str(), message.c_str(), true, false, false, false, false);
}

}  // namespace vacdm