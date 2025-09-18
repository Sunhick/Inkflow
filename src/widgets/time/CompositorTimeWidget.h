#ifndef COMPOSITOR_TIME_WIDGET_H
#define COMPOSITOR_TIME_WIDGET_H

#include "../../core/CompositorWidget.h"
#include <WiFi.h>
#include <time.h>

class CompositorTimeWidget : public CompositorWidget {
public:
    CompositorTimeWidget(DisplayCompositor& compositor);
    CompositorTimeWidget(DisplayCompositor& compositor, unsigned long updateInterval);

    // Widget interface
    void begin() override;
    bool shouldUpdate() override;

    // Time management
    void syncTimeWithNTP();
    void forceTimeSync();
    void forceUpdate();
    bool isTimeInitialized() const;

    // Time formatting
    String getFormattedDate();
    String getFormattedTime();
    String getDayOfWeek();

protected:
    // CompositorWidget interface
    void renderToSurface(VirtualSurface* surface, const LayoutRegion& region) override;

private:
    static const char* NTP_SERVER;
    static const unsigned long DEFAULT_TIME_UPDATE_INTERVAL = 60000; // 1 minute
    static const long GMT_OFFSET_SEC = -8 * 3600; // PST
    static const int DAYLIGHT_OFFSET_SEC = 3600;

    unsigned long lastTimeUpdate;
    bool timeInitialized;
    unsigned long timeUpdateInterval;

    void drawTimeDisplay(VirtualSurface* surface, const LayoutRegion& region);
};

#endif
