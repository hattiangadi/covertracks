#pragma once
#include <JuceHeader.h>
#include <vector>

namespace covertracks {
namespace mj3 {

struct ConvertedLeadSheet {
    juce::String skeleton;        // 3.0 JSON skeleton with FILL placeholders
    juce::String melodyReference; // Human-readable melody for LLM reference
    int totalBars = 0;
};

class LeadSheetConverter {
public:
    static ConvertedLeadSheet convert(const juce::String& leadSheetJson);

private:
    struct Section {
        juce::String chord;
        int startMeasure;
        int numBars;          // bar count for repeat (1 even for half-bar splits)
        double beats;         // total beats (2.0 for half a 4/4 bar)
        int sourceMeasures;   // how many source measures this section covers (0 for first half of split)
    };

    /** Convert legacy "sections" format to "measures" format for backward compat */
    static juce::var convertSectionsToMeasures(const juce::var& parsed);
    static std::vector<Section> groupMeasures(const juce::var& measures);
    static juce::String buildSkeleton(const std::vector<Section>& sections,
                                       const juce::var& leadSheet);
    static juce::String buildMelodyReference(const juce::var& measures,
                                              const std::vector<Section>& sections);
};

} // namespace mj3
} // namespace covertracks
