#include <Inkplate.h>
#include "ImageUpdater.h"
#include "Config.h"

Inkplate display(INKPLATE_3BIT);
ImageUpdater updater(display, WIFI_SSID, WIFI_PASSWORD, SERVER_URL, REFRESH_MS);

void setup() {
    delay(1000);
    updater.begin();
}

void loop() {
    updater.loop();
}
