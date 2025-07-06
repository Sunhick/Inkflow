#include <Inkplate.h>
#include "ImageUpdater.h"
#include "Config.h"

Inkplate display(INKPLATE_3BIT);
ImageUpdater updater(display, ssid, password, imageUrl, refreshMs);

void setup() {
    delay(1000);
    updater.begin();
}

void loop() {
    updater.loop();
}
