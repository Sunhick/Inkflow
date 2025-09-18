# Display Compositor Migration Guide

This guide explains how to migrate existing widgets to use the new Display Compositor system for improved performance and rendering efficiency.

## Overview

The Display Compositor introduces a virtual drawing surface that widgets render to, allowing for optimized single-pass rendering to the physical display. This eliminates flickering, reduces power consumption, and enables efficient partial updates.

## Migration Steps

### 1. Update Widget Base Class

**Before (Direct Display Widget):**
```cpp
class MyWidget : public Widget {
public:
    MyWidget(Inkplate& display) : Widget(display) {}

    void render(const LayoutRegion& region) override {
        // Direct drawing to display
        display.setCursor(region.x + 10, region.y + 10);
        display.setTextSize(2);
        display.print("Hello World");
    }
};
```

**After (Compositor Widget):**
```cpp
class MyWidget : public CompositorWidget {
public:
    MyWidget(DisplayCompositor& compositor) : CompositorWidget(compositor) {}

protected:
    void renderToSurface(VirtualSurface* surface, const LayoutRegion& region) override {
        // Drawing to virtual surface
        surface->setCursor(region.x + 10, region.y + 10);
        surface->setTextSize(2);
        surface->print("Hello World");
    }
};
```

### 2. Update Drawing Calls

Replace all `display.*` calls with `surface->*` calls:

| Old Display Call                          | New Surface Call                           |
| ----------------------------------------- | ------------------------------------------ |
| `display.setCursor(x, y)`                 | `surface->setCursor(x, y)`                 |
| `display.setTextSize(size)`               | `surface->setTextSize(size)`               |
| `display.setTextColor(color)`             | `surface->setTextColor(color)`             |
| `display.print(text)`                     | `surface->print(text)`                     |
| `display.fillRect(x, y, w, h, color)`     | `surface->fillRect(x, y, w, h, color)`     |
| `display.drawLine(x0, y0, x1, y1, color)` | `surface->drawLine(x0, y0, x1, y1, color)` |
| `display.drawPixel(x, y, color)`          | `surface->drawPixel(x, y, color)`          |

### 3. Update Widget Instantiation

**Before:**
```cpp
MyWidget* widget = new MyWidget(display);
```

**After:**
```cpp
MyWidget* widget = new MyWidget(compositor);
```

### 4. Update Layout Manager Integration

**Before (Direct Rendering):**
```cpp
void LayoutManager::renderAllWidgets() {
    widget1->render(region1);
    widget2->render(region2);
    display.display(); // Direct display update
}
```

**After (Compositor Rendering):**
```cpp
void LayoutManager::renderAllWidgets() {
    compositor->clear();
    widget1->render(region1);
    widget2->render(region2);
    compositor->compose();
    compositor->render(); // Single optimized render
}
```

## Complete Example Migration

### Original TimeWidget (Simplified)

```cpp
class TimeWidget : public Widget {
public:
    TimeWidget(Inkplate& display) : Widget(display) {}

    void render(const LayoutRegion& region) override {
        clearRegion(region);

        display.setCursor(region.x + 10, region.y + 10);
        display.setTextSize(2);
        display.setTextColor(0);
        display.print("TIME");

        display.setCursor(region.x + 10, region.y + 40);
        display.setTextSize(3);
        display.print(getCurrentTime());
    }
};
```

### Migrated CompositorTimeWidget

```cpp
class CompositorTimeWidget : public CompositorWidget {
public:
    CompositorTimeWidget(DisplayCompositor& compositor) : CompositorWidget(compositor) {}

protected:
    void renderToSurface(VirtualSurface* surface, const LayoutRegion& region) override {
        surface->clearRegion(region);

        surface->setCursor(region.x + 10, region.y + 10);
        surface->setTextSize(2);
        surface->setTextColor(0);
        surface->print("TIME");

        surface->setCursor(region.x + 10, region.y + 40);
        surface->setTextSize(3);
        surface->print(getCurrentTime());
    }
};
```

## Benefits After Migration

1. **Reduced Flickering**: All drawing happens off-screen before being rendered
2. **Partial Updates**: Only changed regions are updated on the display
3. **Better Performance**: Single render operation instead of multiple display updates
4. **Power Efficiency**: Fewer display refreshes mean lower power consumption
5. **Automatic Optimization**: Compositor handles display mode switching and region merging

## Compatibility Notes

- The `Widget` base class is still supported for backward compatibility
- Existing widgets will continue to work but won't benefit from compositor optimizations
- Mixed usage (some widgets using compositor, others direct rendering) is supported but not recommended
- The compositor automatically handles display mode switching for partial updates

## Testing Your Migration

1. Verify widgets render correctly in their assigned regions
2. Test partial updates by changing individual widgets
3. Monitor serial output for compositor debug messages
4. Check that display updates are visually smooth without flickering

## Common Issues and Solutions

### Issue: Widget not rendering
**Solution**: Ensure `renderToSurface()` is implemented instead of `render()`

### Issue: Text appears garbled
**Solution**: Check that text drawing calls use `surface->` prefix instead of `display.`

### Issue: Partial updates not working
**Solution**: Verify that `markRegionDirty()` is being called for changed regions

### Issue: Performance worse than before
**Solution**: Ensure you're using `renderPartial()` for small updates and `render()` for full screen updates
