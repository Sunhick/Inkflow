# Compositor Architecture

The Compositor is a virtual framebuffer that sits between widgets and the Inkplate e-ink display. Instead of widgets drawing directly to the hardware, they render to an in-memory surface. The compositor then decides how to push those pixels to the display — full refresh or partial update — minimizing costly e-ink refreshes.

## Why a Compositor?

E-ink displays are slow to refresh (~1-2s for full, ~0.3s for partial). Without a compositor, every widget update triggers a full display refresh. The compositor batches changes and uses partial updates where possible, reducing flicker and power consumption.

## System Overview

```mermaid
graph TD
    subgraph Widgets
        W1[WeatherWidget]
        W2[TimeWidget]
        W3[BatteryWidget]
        W4[ImageWidget]
        W5[NameWidget]
    end

    subgraph Core
        C[Compositor]
        VS[Virtual Surface<br/>1200×825 bytes<br/>PSRAM ~990KB]
        CT[Changed Regions<br/>vector of LayoutRegion]
    end

    subgraph Display
        DM[DisplayManager]
        INK[Inkplate 10<br/>E-Ink Hardware]
    end

    W1 -->|renderToCompositor| C
    W2 -->|renderToCompositor| C
    W3 -->|renderToCompositor| C
    W4 -->|renderToCompositor| C
    W5 -->|renderToCompositor| C

    C --> VS
    C --> CT

    C -->|displayToInkplate| DM
    C -->|partialDisplayToInkplate| DM
    DM --> INK
```

## Initialization Sequence

PSRAM must be available before the compositor allocates its surface buffer. The Inkplate library initializes PSRAM inside `display.begin()`, so compositor init is deferred until after that call.

```mermaid
sequenceDiagram
    participant Main
    participant LM as LayoutManager
    participant DM as DisplayManager
    participant Disp as Inkplate
    participant Comp as Compositor

    Main->>LM: begin()
    LM->>LM: loadConfig()
    LM->>LM: calculateLayoutRegions()
    LM->>DM: new DisplayManager(display)
    LM->>LM: createAndAssignWidgets()
    LM->>LM: initializeComponents()
    LM->>DM: initialize(displayMode)
    DM->>Disp: display.begin()
    Note over Disp: PSRAM now available
    DM->>Disp: setDisplayMode(1-bit or 3-bit)
    LM->>Comp: initialize()
    Comp->>Comp: heap_caps_malloc(990KB, SPIRAM)
    Note over Comp: Single allocation for surface only
    Comp-->>LM: success
    LM->>DM: setCompositor(compositor)
```

## Rendering Pipeline

### Full Render (initial boot, button wake)

```mermaid
flowchart TD
    A[LayoutManager::renderAllRegions] --> B{Compositor<br/>available?}
    B -->|Yes| C[compositor.clear]
    C --> D[For each region]
    D --> E[widget.renderToCompositor]
    E --> F[compositor.clearRegion]
    F --> G[widget draws to virtual surface]
    G --> H[region.markClean]
    H --> D
    D -->|All done| I[layoutWidget.renderToCompositor]
    I --> J[displayManager.renderWithCompositor]
    J --> K[compositor.displayToInkplate]
    K --> L[Copy surface → display buffer]
    L --> M[display.display — full e-ink refresh]

    B -->|No / Fallback| N[Direct rendering]
    N --> O[widget.render → display directly]
    O --> P[displayManager.update]
```

### Partial Render (changed regions only)

```mermaid
flowchart TD
    A[LayoutManager::renderChangedRegions] --> B{Compositor<br/>available?}
    B -->|Yes| C[For each dirty region]
    C --> D[compositor.clearRegion]
    D --> E[widget.renderToCompositor]
    E --> F[markRegionChanged — adds to changedAreas]
    F --> G[region.markClean]
    G --> C
    C -->|Done| H{Any changes?}
    H -->|Yes| I[Merge overlapping regions]
    I --> J[Coalesce nearby regions]
    J --> K[compositor.partialDisplayToInkplate]
    K --> L[Copy only changed regions → display]
    L --> M[display.partialUpdate — fast e-ink refresh]
    H -->|No| N[Skip display update]

    B -->|No / Fallback| O[Direct partial render]
    O --> P[displayManager.smartPartialUpdate]
```

## Memory Layout

```
PSRAM (single allocation, ~990KB):
┌─────────────────────────────────────────┐
│  Virtual Surface Buffer                 │
│  1200 × 825 × 1 byte = 990,000 bytes   │
│  (8-bit grayscale per pixel)            │
└─────────────────────────────────────────┘

Internal SRAM (small, dynamic):
┌─────────────────────────────────────────┐
│  changedAreas: vector<LayoutRegion>     │
│  ~5-10 regions × 16 bytes = ~160 bytes  │
├─────────────────────────────────────────┤
│  regionHistory: vector<RegionHistory>   │
│  update frequency stats per region      │
├─────────────────────────────────────────┤
│  UpdateMetrics struct (~32 bytes)       │
└─────────────────────────────────────────┘
```

Dirty region tracking uses widget-level rectangles (`changedAreas` vector) instead of a per-pixel boolean array. This saves ~990KB of PSRAM — the per-pixel array was redundant since widgets always render to rectangular regions and the partial update logic already operated on rectangles.

## Region Merge Optimization

When multiple small regions change, the compositor merges them to reduce the number of partial updates. This avoids the overhead of many tiny e-ink refreshes.

```mermaid
flowchart LR
    subgraph Before
        R1[Time<br/>0,206<br/>240×206]
        R2[Battery<br/>0,618<br/>240×206]
    end

    subgraph After Merge
        RM[Merged<br/>0,206<br/>240×618]
    end

    R1 --> RM
    R2 --> RM
```

Merge decisions are based on:
- Distance between regions (configurable threshold)
- Merge efficiency ratio (merged area vs sum of individual areas)
- Region update frequency history

## Error Handling & Fallback

```mermaid
stateDiagram-v2
    [*] --> Normal: initialize() success
    Normal --> Normal: render/update OK
    Normal --> Error: allocation fail / render crash
    Error --> Recovery: recoverFromError()
    Recovery --> Normal: recovery success
    Recovery --> Fallback: recovery failed
    Fallback --> Fallback: direct rendering only
    Fallback --> Normal: manual reset

    note right of Normal: Compositor active<br/>Partial updates enabled
    note right of Fallback: Widgets render directly<br/>to Inkplate display
```

When the compositor encounters an error (memory pressure, render failure), it attempts recovery. If recovery fails, it enters fallback mode where widgets render directly to the display hardware — the same behavior as running without a compositor.

## Performance Metrics

The compositor tracks update performance for self-tuning:

| Metric | Purpose |
|---|---|
| `averageUpdateTime` | Decides full vs partial refresh |
| `averagePixelsPerUpdate` | Tunes merge distance threshold |
| `regionHistory` | Tracks per-region update frequency |
| `updateCount` | Total updates since last reset |

## Configuration

`Display.EnableCompositor` in config.json (default: `false`). Set to `true` to enable.

Tunable via code:

```cpp
compositor->setMaxRegionMergeDistance(50);        // px gap to still merge
compositor->setMinRegionSizeForPartialUpdate(100); // min px² for partial
compositor->setUpdateFrequencyThreshold(5000);     // ms between updates
compositor->setRegionMergeEfficiencyThreshold(0.7); // merge if >70% efficient
```

## ESP32 PSRAM Notes

- PSRAM is initialized by `display.begin()` — compositor must init after
- Uses `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` for explicit PSRAM allocation
- `CORE_DEBUG_LEVEL` must be 0 and `CONFIG_HEAP_POISONING_DISABLED=1` to avoid false heap corruption asserts from the Inkplate library's PSRAM usage
- Inkplate 10 v2 has ~4MB PSRAM; compositor uses ~990KB (~24%)
