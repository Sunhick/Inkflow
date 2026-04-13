#ifndef DISPLAY_LIST_H
#define DISPLAY_LIST_H

#include <vector>
#include <WString.h>

#ifdef UNIT_TEST
#include "../../test/mocks/MockInkplate.h"
#else
#include <Inkplate.h>
#endif

// Draw command types
enum class DrawCmd : uint8_t {
    SetCursor,
    SetTextSize,
    SetTextColor,
    SetTextWrap,
    Print,
    FillRect,
    DrawRect,
    DrawLine,
};

// A single recorded draw command
struct DrawCommand {
    DrawCmd type;
    int x, y, w, h;    // geometry (overloaded per command type)
    uint8_t color;
    String text;        // for Print commands

    // Factory methods for clarity
    static DrawCommand setCursor(int x, int y) {
        return {DrawCmd::SetCursor, x, y, 0, 0, 0, ""};
    }
    static DrawCommand setTextSize(int size) {
        return {DrawCmd::SetTextSize, size, 0, 0, 0, 0, ""};
    }
    static DrawCommand setTextColor(uint8_t color) {
        return {DrawCmd::SetTextColor, 0, 0, 0, 0, color, ""};
    }
    static DrawCommand setTextWrap(bool wrap) {
        return {DrawCmd::SetTextWrap, wrap ? 1 : 0, 0, 0, 0, 0, ""};
    }
    static DrawCommand print(const String& text) {
        return {DrawCmd::Print, 0, 0, 0, 0, 0, text};
    }
    static DrawCommand fillRect(int x, int y, int w, int h, uint8_t color) {
        return {DrawCmd::FillRect, x, y, w, h, color, ""};
    }
    static DrawCommand drawRect(int x, int y, int w, int h, uint8_t color) {
        return {DrawCmd::DrawRect, x, y, w, h, color, ""};
    }
    static DrawCommand drawLine(int x1, int y1, int x2, int y2, uint8_t color) {
        return {DrawCmd::DrawLine, x1, y1, x2, y2, color, ""};
    }
};

// Records draw commands and replays them to an Inkplate display.
// Widgets call DisplayList methods instead of display.* directly.
class DisplayList {
public:
    void clear() { commands.clear(); }
    size_t size() const { return commands.size(); }
    bool empty() const { return commands.empty(); }

    // Recording API — mirrors Inkplate/Adafruit GFX interface
    void setCursor(int x, int y) { commands.push_back(DrawCommand::setCursor(x, y)); }
    void setTextSize(int size) { commands.push_back(DrawCommand::setTextSize(size)); }
    void setTextColor(uint8_t color) { commands.push_back(DrawCommand::setTextColor(color)); }
    void setTextWrap(bool wrap) { commands.push_back(DrawCommand::setTextWrap(wrap)); }
    void print(const String& text) { commands.push_back(DrawCommand::print(text)); }
    void printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
    void fillRect(int x, int y, int w, int h, uint8_t color) {
        commands.push_back(DrawCommand::fillRect(x, y, w, h, color));
    }
    void drawRect(int x, int y, int w, int h, uint8_t color) {
        commands.push_back(DrawCommand::drawRect(x, y, w, h, color));
    }
    void drawLine(int x1, int y1, int x2, int y2, uint8_t color) {
        commands.push_back(DrawCommand::drawLine(x1, y1, x2, y2, color));
    }

    // Replay all recorded commands to the actual display
    void replayTo(Inkplate& display) const;

    // Access commands for inspection/debugging
    const std::vector<DrawCommand>& getCommands() const { return commands; }

private:
    std::vector<DrawCommand> commands;
};

#endif
