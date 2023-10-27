#include <types/Flight.h>
#include <types/SystemConfig.h>

namespace vacdm {

class Color {
private:
    std::thread m_worker;
    bool m_flash;
    bool m_stop;
    void run();
public:
    Color();
    ~Color();

    // Times:
    COLORREF colorizeEobtAndTobt(const types::Flight_t& flight) const;
    COLORREF colorizeTsat(const types::Flight_t& flight) const;
    COLORREF colorizeTtot(const types::Flight_t& flight) const;
    COLORREF colorizeAort(const types::Flight_t& flight) const;
    COLORREF colorizeAsrt(const types::Flight_t& flight) const;
    COLORREF colorizeAsat(const types::Flight_t& flight) const;

    // Timers:
    COLORREF colorizeAsatTimer(const types::Flight_t& flight) const;
    COLORREF colorizeCtotandCtottimer(const types::Flight_t& flight) const;

    SystemConfig m_pluginConfig;
    void changePluginConfig(const SystemConfig newPluginConfig);

    Color(const Color&) = delete;
    Color(Color&&) = delete;

    Color& operator=(const Color&) = delete;
    Color& operator=(Color&&) = delete;

    static Color& instance();
};

}