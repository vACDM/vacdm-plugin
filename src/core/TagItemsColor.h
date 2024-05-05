#pragma once

#include "config/ConfigHandler.h"
#include "config/PluginConfig.h"
#include "types/Pilot.h"

using namespace vacdm;

namespace vacdm::tagitems {

class Color : ConfigObserver {
   private:
    Color();
    ~Color();
    Color(const Color &) = delete;
    Color(Color &&) = delete;
    Color &operator=(const Color &) = delete;
    Color &operator=(Color &&) = delete;

    static inline PluginConfig m_pluginConfig;

   public:
    static Color &instance();

    void onConfigUpdate(PluginConfig newPluginConfig);

    static COLORREF colorizeEobt(const types::Pilot &pilot);
    static COLORREF colorizeTobt(const types::Pilot &pilot);
    static COLORREF colorizeTsat(const types::Pilot &pilot);
    static COLORREF colorizeTtot(const types::Pilot &pilot);
    static int colorizeExot(const types::Pilot &pilot);
    static COLORREF colorizeAsat(const types::Pilot &pilot);
    static COLORREF colorizeAobt(const types::Pilot &pilot);
    static COLORREF colorizeAtot(const types::Pilot &pilot);
    static COLORREF colorizeAsrt(const types::Pilot &pilot);
    static COLORREF colorizeAort(const types::Pilot &pilot);
    static COLORREF colorizeCtot(const types::Pilot &pilot);
    static COLORREF colorizeCtotTimer(const types::Pilot &pilot);
    static COLORREF colorizeAsatTimer(const types::Pilot &pilot);
    static COLORREF colorizeEcfmpMeasure(const types::Pilot &pilot);
    static COLORREF colorizeEventBooking(const types::Pilot &pilot);

   private:
    static COLORREF colorizeEobtAndTobt(const types::Pilot &pilot);
    static COLORREF colorizeCtotandCtottimer(const types::Pilot &pilot);
};
}  // namespace vacdm::tagitems