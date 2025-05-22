#pragma once

#include <JuceHeader.h>

namespace covertracks {

/**
 * @brief Utility class for standard application fonts
 */
class AppFonts
{
public:
    /** @brief Get a title font (24pt, bold) */
    static juce::Font getTitleFont() {
        return juce::Font(juce::FontOptions("Default", "Bold", 24.0f));
    }
    
    /** @brief Get a header font (16pt, bold) */
    static juce::Font getHeaderFont() {
        return juce::Font(juce::FontOptions("Default", "Bold", 16.0f));
    }
    
    /** @brief Get a standard body font (14pt, regular) */
    static juce::Font getBodyFont() {
        return juce::Font(juce::FontOptions(14.0f));
    }
    
    /** @brief Get a small font (12pt, regular) */
    static juce::Font getSmallFont() {
        return juce::Font(juce::FontOptions(12.0f));
    }
};

} // namespace covertracks
