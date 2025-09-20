#include "NameWidget.h"

NameWidget::NameWidget(Inkplate& display)
    : Widget(display), familyName("Family"), hasRendered(false) {}

NameWidget::NameWidget(Inkplate& display, const String& name)
    : Widget(display), familyName(name), hasRendered(false) {
    Serial.printf("NameWidget created with family name: %s\n", name.c_str());
}

void NameWidget::begin() {
    Serial.println("Initializing name widget...");
    hasRendered = false;
}

bool NameWidget::shouldUpdate() {
    // Name widget only needs to render once unless the name changes
    return !hasRendered;
}

void NameWidget::render(const LayoutRegion& region) {
    Serial.printf("Rendering name widget in region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the widget region
    clearRegion(region);

    // Draw name content within the region
    drawNameDisplay(region);

    hasRendered = true;
}

void NameWidget::setFamilyName(const String& name) {
    if (familyName != name) {
        familyName = name;
        hasRendered = false; // Force re-render when name changes
        Serial.printf("Family name updated to: %s\n", name.c_str());
    }
}

String NameWidget::getFamilyName() const {
    return familyName;
}

void NameWidget::drawNameDisplay(const LayoutRegion& region) {
    // Draw decorative border around the name area first
    int borderMargin = 12;
    int borderX = region.getX() + borderMargin;
    int borderY = region.getY() + borderMargin;
    int borderWidth = region.getWidth() - (borderMargin * 2);
    int borderHeight = region.getHeight() - (borderMargin * 2);

    // Draw double border for elegant look
    display.drawRect(borderX, borderY, borderWidth, borderHeight, 0);
    display.drawRect(borderX + 2, borderY + 2, borderWidth - 4, borderHeight - 4, 0);

    // Add decorative corner elements
    int cornerSize = 8;
    // Top-left corner
    display.drawLine(borderX + 6, borderY + 6, borderX + 6 + cornerSize, borderY + 6, 0);
    display.drawLine(borderX + 6, borderY + 6, borderX + 6, borderY + 6 + cornerSize, 0);

    // Top-right corner
    display.drawLine(borderX + borderWidth - 6 - cornerSize, borderY + 6, borderX + borderWidth - 6, borderY + 6, 0);
    display.drawLine(borderX + borderWidth - 6, borderY + 6, borderX + borderWidth - 6, borderY + 6 + cornerSize, 0);

    // Bottom-left corner
    display.drawLine(borderX + 6, borderY + borderHeight - 6 - cornerSize, borderX + 6, borderY + borderHeight - 6, 0);
    display.drawLine(borderX + 6, borderY + borderHeight - 6, borderX + 6 + cornerSize, borderY + borderHeight - 6, 0);

    // Bottom-right corner
    display.drawLine(borderX + borderWidth - 6 - cornerSize, borderY + borderHeight - 6, borderX + borderWidth - 6, borderY + borderHeight - 6, 0);
    display.drawLine(borderX + borderWidth - 6, borderY + borderHeight - 6 - cornerSize, borderX + borderWidth - 6, borderY + borderHeight - 6, 0);

    // Text rendering area (inside the border)
    int textMargin = 20;
    int textAreaX = borderX + textMargin;
    int textAreaY = borderY + textMargin;
    int textAreaWidth = borderWidth - (textMargin * 2);
    int textAreaHeight = borderHeight - (textMargin * 2);

    // Set text properties
    display.setTextSize(4);
    display.setTextColor(0);
    display.setTextWrap(false); // We'll handle wrapping manually

    // Calculate character dimensions for this text size
    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds("A", 0, 0, &x1, &y1, &charWidth, &charHeight);

    int lineHeight = charHeight + 4; // Add some line spacing
    int maxCharsPerLine = textAreaWidth / (charWidth * 0.8); // Approximate character width

    // Split text into words and wrap them
    String words[20]; // Support up to 20 words
    int wordCount = 0;

    // Split familyName into words
    String tempName = familyName;
    tempName.trim();

    int lastSpace = -1;
    for (int i = 0; i <= tempName.length(); i++) {
        if (i == tempName.length() || tempName.charAt(i) == ' ') {
            if (i > lastSpace + 1) {
                words[wordCount] = tempName.substring(lastSpace + 1, i);
                wordCount++;
                if (wordCount >= 20) break; // Safety limit
            }
            lastSpace = i;
        }
    }

    // Create lines by fitting words
    String lines[10]; // Support up to 10 lines
    int lineCount = 0;
    String currentLine = "";

    for (int i = 0; i < wordCount; i++) {
        String testLine = currentLine;
        if (testLine.length() > 0) testLine += " ";
        testLine += words[i];

        // Check if this line would be too long
        uint16_t testWidth;
        display.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &testWidth, &charHeight);

        if (testWidth <= textAreaWidth || currentLine.length() == 0) {
            // Line fits, add the word
            currentLine = testLine;
        } else {
            // Line too long, start new line
            if (currentLine.length() > 0) {
                lines[lineCount] = currentLine;
                lineCount++;
            }
            currentLine = words[i];
        }
    }

    // Add the last line
    if (currentLine.length() > 0 && lineCount < 10) {
        lines[lineCount] = currentLine;
        lineCount++;
    }

    // Calculate total text height and starting Y position for vertical centering
    int totalTextHeight = lineCount * lineHeight;
    int startY = textAreaY + (textAreaHeight - totalTextHeight) / 2;

    // Draw each line centered horizontally
    for (int i = 0; i < lineCount; i++) {
        uint16_t lineWidth;
        display.getTextBounds(lines[i].c_str(), 0, 0, &x1, &y1, &lineWidth, &charHeight);

        int lineX = textAreaX + (textAreaWidth - lineWidth) / 2;
        int lineY = startY + (i * lineHeight);

        // Draw the line with bold effect (multiple overlapping prints)
        display.setCursor(lineX, lineY);
        display.print(lines[i]);

        display.setCursor(lineX + 1, lineY);
        display.print(lines[i]);

        display.setCursor(lineX, lineY + 1);
        display.print(lines[i]);

        display.setCursor(lineX + 1, lineY + 1);
        display.print(lines[i]);

        Serial.printf("Drew line %d: '%s' at (%d,%d)\n", i, lines[i].c_str(), lineX, lineY);
    }

    Serial.printf("Drew fancy family name '%s' with %d lines, center-aligned with wrapping\n",
                  familyName.c_str(), lineCount);
}
