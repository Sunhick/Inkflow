# Display Compositor Architecture

## Overview

The Display Compositor is a rendering system designed specifically for e-paper displays like the Inkplate10. It provides efficient, flicker-free rendering by using a virtual drawing surface and intelligent update strategies.

## Core Components

### 1. DisplayCompositor

The central rendering engine that manages the entire display pipeline.

**Key Responsibilities:**
- Manages a virtual drawing surface (VirtualSurface)
- Tracks dirty regions that need updating
- Handles display mode switching (1-bit vs 3-bit)
- Optimizes partial updates
- Performs final rendering to the physical display

**Key Methods:**
- `begin()` - Initialize the compositor and virtual surface
- `beginWidgetRender(region)` - Start widget rendering, returns VirtualSurface
- `endWidgetRender(region)` - Complete widget rendering, marks region dirty
- `compose()` - Prepare virtual surface for rendering
- `render()` - Full screen render to physical display
- `renderPartial()` - Optimized partial update of dirty regions only

### 2. VirtualSurface

An off-screen drawing buffer that widgets render to before final display.

**Features:**
- 3-bit grayscale support (8 color levels: 0-7)
- Full drawing API compatible with Inkplate
- Automatic bounds checking
- Memory-efficient pixel storage

**Drawing Methods:**
- `fillRect()`, `drawLine()`, `drawPixel()` - Basic drawing primitives
- `setCursor()`, `setTextSize()`, `print()` - Text rendering
- `clear()`, `clearRegion()` - Surface management

### 3. CompositorWidget

Base class for widgets that work with the compositor system.

**Interface:**
- `renderToSurface(surface, region)` - Widget draws to virtual surface
- Inherits standard widget methods: `begin()`, `shouldUpdate()`
- Automatic integration with compositor dirty region tracking

### 4. Dirty Region Management

Intelligent tracking of screen areas that need updating.

**Features:**
- Tracks rectangular regions that have changed
- Merges overlapping regions to optimize updates
- Supports both full-screen and partial updates
- Automatic region optimization

## Rendering Pipeline

```
1. Widget Rendering Phase:
   ┌─────────────┐    ┌──────────────────┐    ┌─────────────────┐
   │   Widget    │───▶│  VirtualSurface  │───▶│  Dirty Region   │
   │  render()   │    │   (off-screen)   │    │    Tracking     │
   └─────────────┘    └──────────────────┘    └─────────────────┘

2. Composition Phase:
   ┌─────────────────┐    ┌──────────────────┐
   │  All Widgets    │───▶│   Compositor     │
   │   Complete      │    │   compose()      │
   └─────────────────┘    └──────────────────┘

3. Rendering Phase:
   ┌──────────────────┐    ┌─────────────────┐    ┌──────────────┐
   │   Compositor     │───▶│  Display Mode   │───▶│   Physical   │
   │ render/Partial() │    │   Management    │    │   Display    │
   └──────────────────┘    └─────────────────┘    └──────────────┘
```

## Performance Optimizations

### 1. Single Render Operation
- All widgets draw to virtual surface first
- Single `display.display()` call for entire screen
- Eliminates flickering from multiple display updates

### 2. Partial Update Strategy
- Tracks only changed regions (dirty regions)
- Uses Inkplate's `partialUpdate()` for small changes
- Automatically switches display modes for optimal performance

### 3. Region Optimization
- Merges overlapping dirty regions
- Reduces number of update operations
- Minimizes data transfer to display

### 4. Display Mode Management
- Automatically switches between 1-bit and 3-bit modes
- Uses 1-bit for partial updates (faster)
- Uses 3-bit for full renders (better quality)

## Memory Management

### Virtual Surface Buffer
- Allocates `width × height` bytes for pixel data
- Each pixel stores 3-bit color value (0-7)
- Automatic cleanup in destructor

### Dirty Region Storage
- Uses `std::vector<DirtyRegion>` for dynamic sizing
- Regions automatically merged and optimized
- Cleared after each render cycle

## Integration with Existing Code

### Backward Compatibility
- Original `Widget` class still supported
- Can mix compositor and non-compositor widgets
- Gradual migration path available

### Layout Manager Integration
```cpp
// Before: Direct rendering
widget->render(region);
display.display();

// After: Compositor rendering
compositor->clear();
widget->render(region);  // Draws to virtual surface
compositor->compose();
compositor->render();    // Single optimized render
```

## Configuration and Tuning

### Display Modes
- `INKPLATE_1BIT` - Fast partial updates, black/white only
- `INKPLATE_3BIT` - Full grayscale, slower but higher quality

### Update Strategies
- **Full Render**: Use for major layout changes or initial display
- **Partial Render**: Use for small widget updates (time, battery, etc.)
- **Smart Switching**: Compositor automatically chooses optimal strategy

### Memory Considerations
- Virtual surface uses ~800KB for Inkplate10 (1200×825 pixels)
- Consider available ESP32 memory when using large displays
- Dirty region tracking adds minimal overhead

## Error Handling

### Bounds Checking
- VirtualSurface automatically clips drawing operations
- Invalid coordinates are safely ignored
- No buffer overruns possible

### Graceful Degradation
- Falls back to full render if partial update fails
- Continues operation even if some widgets fail to render
- Comprehensive serial logging for debugging

## Future Enhancements

### Potential Improvements
1. **Compression**: Compress virtual surface data to save memory
2. **Caching**: Cache rendered widget content for faster updates
3. **Animation**: Support for smooth transitions between states
4. **Multi-layer**: Support for overlay layers (UI elements, notifications)
5. **Hardware Acceleration**: Utilize ESP32 DMA for faster memory operations

### Extensibility Points
- Custom drawing primitives can be added to VirtualSurface
- New optimization strategies can be implemented in compositor
- Widget rendering pipeline can be extended with effects/filters
