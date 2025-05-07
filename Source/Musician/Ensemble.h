#pragma once

#include "Musician.h"
#include <map>
#include <string>
#include <memory>
#include <vector>

namespace coverband {

/**
 * @brief A class that manages a group of musicians playing together
 * 
 * The Ensemble class coordinates multiple musicians to play together,
 * generating patterns that are synchronized and musically coherent.
 */
class Ensemble : public Musician {
public:
    /**
     * @brief Construct a new Ensemble object
     * 
     * @param client The LLM client to use for pattern generation
     */
    Ensemble(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Destroy the Ensemble object
     */
    ~Ensemble() override = default;
    
    /**
     * @brief Add a musician to the ensemble
     * 
     * @param instrument The instrument type for the musician
     * @param playerId The ID of the player for this musician
     * @param playerController The player controller for MIDI output
     * @return true if the musician was added successfully
     */
    bool addMusician(const std::string& instrument, 
                    const std::string& playerId,
                    PlayerController* playerController);
    
    /**
     * @brief Generate and play a pattern for the entire ensemble
     * 
     * @param parameters The parameters for pattern generation
     * @return json The generated pattern information
     */
    json generateAndPlayPattern(const json& parameters) override;
    
    /**
     * @brief Handle when a pattern has finished playing
     * 
     * @param patternId The ID of the pattern that finished
     */
    void onPatternCompleted(const std::string& patternId) override;
    
    /**
     * @brief Get the player ID for this ensemble
     * 
     * @return std::string The player ID (returns the first musician's player ID)
     */
    std::string getPlayerId() const;
    
private:
    /**
     * @brief Generate a pattern for a specific instrument
     * 
     * @param instrument The instrument to generate for
     * @param parameters The generation parameters
     * @return json The generated pattern
     */
    json generatePatternForInstrument(const std::string& instrument, const json& parameters);
    
    /**
     * @brief Parse the LLM response into instrument-specific patterns
     * 
     * @param response The LLM response
     * @param parameters The original parameters
     * @return json The parsed patterns for each instrument
     */
    json parseEnsembleResponse(const json& response, const json& parameters);
    
    // Map of instrument type to musician
    std::map<std::string, std::unique_ptr<Musician>> musicians;
    
    // Currently active pattern IDs
    std::vector<std::string> activePatternIds;
};

} // namespace coverband
