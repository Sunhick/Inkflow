#ifndef NAME_WIDGET_H
#define NAME_WIDGET_H

#include "../../core/Widget.h"

// Forward declaration
class Compositor;

class NameWidget : public Widget {
public:
    NameWidget(Inkplate& display);
    NameWidget(Inkplate& display, const String& familyName);

    // Widget interface implementation
    void render(const LayoutRegion& region) override;
    void renderToCompositor(Compositor& compositor, const LayoutRegion& region) override;
    bool shouldUpdate() override;
    void begin() override;
    WidgetType getWidgetType() const override;

    // Name-specific methods
    void setFamilyName(const String& name);
    String getFamilyName() const;

private:
    String familyName;
    bool hasRendered;

    void drawNameDisplay(const LayoutRegion& region);
    void drawNameDisplayToCompositor(Compositor& compositor, const LayoutRegion& region);
};

#endif
