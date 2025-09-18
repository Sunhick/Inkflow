#ifndef NAME_WIDGET_H
#define NAME_WIDGET_H

#include "../core/Widget.h"

class NameWidget : public Widget {
public:
    NameWidget(Inkplate& display);
    NameWidget(Inkplate& display, const String& familyName);

    // Widget interface implementation
    void render(const LayoutRegion& region) override;
    bool shouldUpdate() override;
    void begin() override;

    // Name-specific methods
    void setFamilyName(const String& name);
    String getFamilyName() const;

private:
    String familyName;
    bool hasRendered;

    void drawNameDisplay(const LayoutRegion& region);
};

#endif
