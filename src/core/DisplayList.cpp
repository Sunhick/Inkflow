#include "DisplayList.h"
#include <cstdarg>
#include <cstdio>

void DisplayList::printf(const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    commands.push_back(DrawCommand::print(String(buf)));
}

void DisplayList::replayTo(Inkplate& display) const {
    for (const auto& cmd : commands) {
        switch (cmd.type) {
            case DrawCmd::SetCursor:
                display.setCursor(cmd.x, cmd.y);
                break;
            case DrawCmd::SetTextSize:
                display.setTextSize(cmd.x);
                break;
            case DrawCmd::SetTextColor:
                display.setTextColor(cmd.color);
                break;
            case DrawCmd::SetTextWrap:
                display.setTextWrap(cmd.x != 0);
                break;
            case DrawCmd::Print:
                display.print(cmd.text);
                break;
            case DrawCmd::FillRect:
                display.fillRect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.color);
                break;
            case DrawCmd::DrawRect:
                display.drawRect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.color);
                break;
            case DrawCmd::DrawLine:
                display.drawLine(cmd.x, cmd.y, cmd.w, cmd.h, cmd.color);
                break;
        }
    }
}
