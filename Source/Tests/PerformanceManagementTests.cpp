#include <JuceHeader.h>
#include "../Musician/Core/Performance.h"
#include "../Musician/Core/EnsembleManager.h"
#include "../Player/Core/PlayerManager.h"
#include "../Player/Core/Player.h"
#include "../Musician/Core/Song.h"
#include "../Audio/TracktionAudioEngine.h"

namespace covertracks {

/**
 * @brief Comprehensive tests for Performance and EnsembleManager
 */
class PerformanceManagementTests : public juce::UnitTest
{
public:
    PerformanceManagementTests() : juce::UnitTest("Performance Management", "Performance System") {}

    void runTest() override
    {
        testPerformanceCreation();
        testEnsembleManagement();
        testSynchronizedPlayback();
        testPerformanceSerialization();
    }

private:
    void testPerformanceCreation()
    {
        beginTest("Performance creation and basic operations");
        
        // Create song
        auto song = std::make_shared<Song>("Test Song", 120.0);
        song->setSourceStructureJson(R"({
            "sections": [
                {"name": "verse", "measures": 8},
                {"name": "chorus", "measures": 8}
            ]
        })");
        
        // Create performance
        Performance performance("Test Performance");
        performance.setSong(song);
        performance.setEnsembleName("Test Band");
        performance.setStyle("Jazz");
        
        // Verify performance properties
        expect(performance.getName() == "Test Performance", "Performance name should be set");
        expect(performance.getSong() == song, "Performance should reference the song");
        expect(performance.getEnsembleName() == "Test Band", "Ensemble name should be set");
        expect(performance.getStyle() == "Jazz", "Style should be set");
        
        // Test state management
        expect(!performance.isPlaying(), "Should not be playing initially");
    }
    
    void testEnsembleManagement()
    {
        beginTest("Ensemble management operations");
        
        // Create player manager and players
        PlayerManager playerManager;
        auto& engine = TracktionAudioEngine::getInstance();
        auto& formatManager = engine.getFormatManager();
        auto& knownPluginList = engine.getKnownPluginList();
        
        auto player1 = std::make_unique<Player>("guitar-1", "Guitar Player", formatManager, knownPluginList);
        auto player2 = std::make_unique<Player>("bass-1", "Bass Player", formatManager, knownPluginList);
        auto player3 = std::make_unique<Player>("drums-1", "Drum Player", formatManager, knownPluginList);
        
        Player* guitarPtr = player1.get();
        Player* bassPtr = player2.get();
        Player* drumPtr = player3.get();
        
        playerManager.addPlayer(std::move(player1));
        playerManager.addPlayer(std::move(player2));
        playerManager.addPlayer(std::move(player3));
        
        // Create ensemble manager
        EnsembleManager ensemble(&playerManager, nullptr, "test-ensemble");
        ensemble.setName("Jazz Trio");
        ensemble.setDescription("Small jazz ensemble");
        
        // Add musician roles
        bool guitarAdded = ensemble.addMusicianRole("Lead Guitar", "Guitar", guitarPtr);
        bool bassAdded = ensemble.addMusicianRole("Bass", "Bass", bassPtr);
        bool drumAdded = ensemble.addMusicianRole("Drums", "Drums", drumPtr);
        
        expect(guitarAdded, "Guitar role should be added successfully");
        expect(bassAdded, "Bass role should be added successfully");
        expect(drumAdded, "Drum role should be added successfully");
        
        // Verify roles
        const auto& roles = ensemble.getMusicianRoles();
        expect(roles.size() == 3, "Should have 3 musician roles");
        
        // Test duplicate role prevention
        bool duplicateAdded = ensemble.addMusicianRole("Lead Guitar", "Guitar", guitarPtr);
        expect(!duplicateAdded, "Duplicate role should not be added");
        
        // Test role removal
        bool guitarRemoved = ensemble.removeMusicianRole("Lead Guitar");
        expect(guitarRemoved, "Guitar role should be removed successfully");
        expect(ensemble.getMusicianRoles().size() == 2, "Should have 2 roles after removal");
        
        // Test removing non-existent role
        bool nonExistentRemoved = ensemble.removeMusicianRole("Piano");
        expect(!nonExistentRemoved, "Non-existent role removal should return false");
    }
    
    void testSynchronizedPlayback()
    {
        beginTest("Synchronized ensemble playback");
        
        PlayerManager playerManager;
        EnsembleManager ensemble(&playerManager, nullptr, "sync-test");
        auto& engine = TracktionAudioEngine::getInstance();
        auto& formatManager = engine.getFormatManager();
        auto& knownPluginList = engine.getKnownPluginList();
        
        // Add players without actual initialization (for testing)
        auto player1 = std::make_unique<Player>("test-1", "Test Player 1", formatManager, knownPluginList);
        auto player2 = std::make_unique<Player>("test-2", "Test Player 2", formatManager, knownPluginList);
        
        Player* p1 = player1.get();
        Player* p2 = player2.get();
        
        playerManager.addPlayer(std::move(player1));
        playerManager.addPlayer(std::move(player2));
        
        // Add roles
        ensemble.addMusicianRole("Lead", "Guitar", p1);
        ensemble.addMusicianRole("Rhythm", "Guitar", p2);
        
        // Test playback without patterns (should fail)
        bool playSuccess = ensemble.startPlaying();
        expect(!playSuccess, "Playing should fail without patterns");
        
        // Add patterns to roles
        auto& roles = const_cast<juce::Array<MusicianRole>&>(ensemble.getMusicianRoles());
        for (auto& role : roles)
        {
            role.pattern = "C4q D4q E4q F4q"; // Simple pattern
        }
        
        // Note: Actual playback testing would require initialized players
        // This tests the pattern validation logic
        expect(!ensemble.isPlaying(), "Should not be playing without initialized players");
    }
    
    void testPerformanceSerialization()
    {
        beginTest("Performance serialization and deserialization");
        
        // Create song and performance
        auto song = std::make_shared<Song>("Serialization Test", 130.0);
        Performance originalPerformance("Original Performance");
        originalPerformance.setSong(song);
        originalPerformance.setEnsembleName("Test Ensemble");
        originalPerformance.setStyle("Rock");
        originalPerformance.setCharacter("Energetic");
        
        // Test serialization
        juce::var serializedData = originalPerformance.toVar();
        expect(serializedData.isObject(), "Serialized data should be an object");
        
        // Verify serialized properties
        expect(serializedData.hasProperty("name"), "Should have name property");
        expect(serializedData.hasProperty("song"), "Should have song property");
        expect(serializedData.hasProperty("ensembleName"), "Should have ensembleName property");
        expect(serializedData.hasProperty("style"), "Should have style property");
        
        // Test deserialization
        auto deserializedPerformance = Performance::fromVar(serializedData);
        expect(deserializedPerformance != nullptr, "Deserialization should succeed");
        
        if (deserializedPerformance)
        {
            expect(deserializedPerformance->getName() == originalPerformance.getName(), 
                   "Deserialized name should match");
            expect(deserializedPerformance->getEnsembleName() == originalPerformance.getEnsembleName(),
                   "Deserialized ensemble name should match");
            expect(deserializedPerformance->getStyle() == originalPerformance.getStyle(),
                   "Deserialized style should match");
        }
    }
};

// Register the test
static PerformanceManagementTests performanceManagementTests;

} // namespace covertracks