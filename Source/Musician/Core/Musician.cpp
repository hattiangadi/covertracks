// Source/Musician/Core/Musician.cpp
#include "Musician.h"
#include "MusicPatternSystem.h"
#include "PatternGenerator.h"
#include "PatternParser.h"
#include "../../LLM/Core/PromptManager.h"
#include <iostream>

namespace covertracks {

Musician::Musician(const String& idValue, const String& styleValue)
    : Thread("MusicianThread"),
      id(idValue), 
      style(styleValue), 
      patternSystem(std::make_unique<MusicPatternSystem>()), 
      promptManager(std::make_unique<PromptManager>()),
      waitEvent(true)
{
    // Create the pattern generator immediately (no longer needs LLM client)
    patternGenerator = std::make_unique<PatternGenerator>(*patternSystem, *promptManager);
    
    // Start the background thread
    startThread();
}

Musician::~Musician()
{
    // Stop the background thread properly
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        waitEvent.signal();
        stopThread(5000);
    }
}

void Musician::setLLMClient(std::shared_ptr<LLMClient> client)
{
    llmClient = client;
    
    // Update the system prompt if we have a pattern generator
    if (patternGenerator)
    {
        String systemPrompt = 
            "You are a music pattern generator. Your role is to create music patterns based on the user's requests. "
            "You'll be given specific instructions about the style, instruments, and structure of the music to create. "
            "Always respond in the specified pattern notation format. Do not include any explanations or comments "
            "outside the requested pattern format.";
        
        patternGenerator->setSystemPrompt(systemPrompt);
    }
    
    // Notify listeners of the change
    sendChangeMessage();
}

bool Musician::isGenerating() const
{
    return generating.load();
}

void Musician::generateChart(const var& parameters, 
                           std::function<void(bool success, const var& chart)> callback)
{
    DBG("DEBUG: Musician::generateChart called");
    DBG("DEBUG: PatternGenerator exists: " << (patternGenerator ? "true" : "false"));
    DBG("DEBUG: LLM client exists: " << (llmClient ? "true" : "false"));
    
    if (!patternGenerator || !llmClient)
    {
        DBG("DEBUG: Missing patternGenerator or llmClient - failing immediately");
        if (callback)
            callback(false, var("No LLM client connected"));
        return;
    }
    
    if (generating.load())
    {
        DBG("DEBUG: Generation already in progress - failing immediately");
        if (callback)
            callback(false, var("Pattern generation already in progress"));
        return;
    }
    
    DBG("DEBUG: Musician proceeding with chart generation");
    
    // Extract parameters
    String styleToUse = parameters.hasProperty("style") ? parameters["style"].toString() : this->style;
    
    StringArray instruments;
    if (parameters.hasProperty("instruments"))
    {
        if (parameters["instruments"].isArray())
        {
            for (const auto& instrument : *parameters["instruments"].getArray())
                instruments.add(instrument.toString());
        }
        else
        {
            instruments.add(parameters["instruments"].toString());
        }
    }
    else
    {
        // Default to keys if no instruments specified
        instruments.add("keys");
    }
    
    int tempo = 120;
    if (parameters.hasProperty("tempo") && parameters["tempo"].isInt())
        tempo = parameters["tempo"];
    
    String description = parameters.hasProperty("description") ? parameters["description"].toString() : "";
    
    // Create the prompt using the PatternGenerator utility
    currentPrompt = patternGenerator->createSongPrompt(styleToUse, instruments, tempo, description);
    currentCallback = std::move(callback);
    
    // Set generating flag and signal the thread
    generating = true;
    waitEvent.signal();
}

void Musician::generateChartWithPlayer(const var& parameters,
                                      Player* player,
                                      std::function<void(bool success, const var& midiEventChart)> callback)
{
    DBG("DEBUG: Musician::generateChartWithPlayer called with Player capabilities");
    
    if (!patternGenerator || !llmClient)
    {
        DBG("DEBUG: Missing patternGenerator or llmClient - failing immediately");
        if (callback)
            callback(false, var("No LLM client connected"));
        return;
    }
    
    if (generating.load())
    {
        DBG("DEBUG: Generation already in progress - failing immediately");
        if (callback)
            callback(false, var("Pattern generation already in progress"));
        return;
    }
    
    if (!player)
    {
        DBG("DEBUG: No Player provided, falling back to standard generateChart");
        generateChart(parameters, callback);
        return;
    }
    
    DBG("DEBUG: Musician proceeding with Player-capabilities-based chart generation");
    
    // Extract parameters
    String styleToUse = parameters.hasProperty("style") ? parameters["style"].toString() : this->style;
    
    // Create a vector with the single Player
    std::vector<Player*> players = { player };
    
    int tempo = 120;
    if (parameters.hasProperty("tempo") && parameters["tempo"].isInt())
        tempo = parameters["tempo"];
    
    String description = parameters.hasProperty("description") ? parameters["description"].toString() : "";
    
    // Create the prompt using the NEW PatternGenerator method with Player capabilities
    currentPrompt = patternGenerator->createSongPromptWithPlayers(styleToUse, players, tempo, description);
    currentCallback = std::move(callback);
    
    // Set generating flag and signal the thread
    generating = true;
    waitEvent.signal();
}

void Musician::generateSection(const String& section, const var& parameters,
                             std::function<void(bool success, const var& chart)> callback)
{
    if (!patternGenerator || !llmClient)
    {
        if (callback)
            callback(false, "No LLM client connected");
        return;
    }
    
    if (generating.load())
    {
        if (callback)
            callback(false, "Pattern generation already in progress");
        return;
    }
    
    // Extract parameters
    String styleToUse = parameters.hasProperty("style") ? parameters["style"].toString() : this->style;
    
    StringArray instruments;
    if (parameters.hasProperty("instruments"))
    {
        if (parameters["instruments"].isArray())
        {
            for (const auto& instrument : *parameters["instruments"].getArray())
                instruments.add(instrument.toString());
        }
        else
        {
            instruments.add(parameters["instruments"].toString());
        }
    }
    else
    {
        // Default to keys if no instruments specified
        instruments.add("keys");
    }
    
    String description = parameters.hasProperty("description") ? parameters["description"].toString() : "";
    
    // Create the prompt using the PatternGenerator utility
    currentPrompt = patternGenerator->createSectionPrompt(section, styleToUse, instruments, description);
    currentCallback = std::move(callback);
    
    // Set generating flag and signal the thread
    generating = true;
    waitEvent.signal();
}

void Musician::modifyChart(const var& currentChart, const String& modification,
                         std::function<void(bool success, const var& chart)> callback)
{
    if (!patternGenerator || !llmClient)
    {
        if (callback)
            callback(false, "No LLM client connected");
        return;
    }
    
    if (generating.load())
    {
        if (callback)
            callback(false, "Pattern generation already in progress");
        return;
    }
    
    // Convert the current chart to notation format
    String currentNotation;
    
    if (currentChart.isObject() && currentChart.hasProperty("notation"))
    {
        // Chart already has notation
        currentNotation = currentChart["notation"].toString();
    }
    else if (lastGeneratedChart)
    {
        // Use the last generated chart's notation (from last response)
        currentNotation = lastResponseText;
    }
    else
    {
        // Create a simple chart based on the current chart
        currentNotation = "[4/4: C3 E3 G3 C4]";
    }
    
    // Create the prompt using the PatternGenerator utility
    currentPrompt = patternGenerator->createModificationPrompt(currentNotation, modification);
    currentCallback = std::move(callback);
    
    // Set generating flag and signal the thread
    generating = true;
    waitEvent.signal();
}

void Musician::run()
{
    while (!threadShouldExit())
    {
        // Wait for a generation request
        waitEvent.wait(-1);
        
        if (threadShouldExit())
            break;
        
        if (llmClient && generating.load() && !requestInProgress.load())
        {
            // Set request in progress to prevent infinite loop
            requestInProgress = true;
            
            // Prepare the prompt with system prompt if needed
            String prompt = currentPrompt;
            String systemPrompt = patternGenerator->getSystemPrompt();
            if (systemPrompt.isNotEmpty())
            {
                prompt = systemPrompt + "\n\n" + prompt;
            }
            
            // Send message to LLM
            llmClient->sendMessage(prompt, 
                [this](bool success, const String& response) {
                    // Parse the response using PatternGenerator utility
                    std::shared_ptr<Chart> chart;
                    
                    DBG("DEBUG: About to validate LLM response");
                    if (success && patternGenerator->validateResponse(response))
                    {
                        DBG("DEBUG: Response validation successful, about to parse");
                        chart = patternGenerator->parseResponse(response);
                        DBG("DEBUG: Parse completed, chart: " << (chart ? "valid" : "null"));
                    }
                    else
                    {
                        DBG("DEBUG: Response validation failed or LLM call unsuccessful");
                    }
                    
                    // Store the response
                    lastResponseText = response;
                    lastGeneratedChart = chart;
                    
                    // Create the response object for the callback
                    var resultChart;
                    
                    if (success && chart)
                    {
                        DBG("DEBUG: Chart is valid, about to convert to MIDI");
                        // Set default tempo if not set
                        if (chart->getTempo() <= 0)
                            chart->setTempo(120.0);
                        
                        DBG("DEBUG: About to call chart->toMidiSequence()");
                        // Convert chart to MIDI events
                        auto midiSequence = chart->toMidiSequence();
                        DBG("DEBUG: toMidiSequence() completed, about to convert to var");
                        DBG("DEBUG: MidiSequence has " << midiSequence->getNumEvents() << " events");
                        
                        // Debug first few events in the sequence
                        for (int i = 0; i < std::min(5, midiSequence->getNumEvents()); ++i)
                        {
                            auto event = midiSequence->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                DBG("DEBUG: MidiSequence event " << i << " - note_on: note=" << event->message.getNoteNumber() 
                                    << ", velocity=" << event->message.getVelocity() << ", timestamp=" << event->message.getTimeStamp());
                            }
                            else if (event->message.isNoteOff())
                            {
                                DBG("DEBUG: MidiSequence event " << i << " - note_off: note=" << event->message.getNoteNumber() 
                                    << ", timestamp=" << event->message.getTimeStamp());
                            }
                        }
                        
                        var midiEvents = midiMessageSequenceToVar(*midiSequence, chart->getTempo());
                        DBG("DEBUG: midiMessageSequenceToVar completed");
                        
                        DynamicObject* responseObj = new DynamicObject();
                        responseObj->setProperty("success", true);
                        responseObj->setProperty("command", "play_pattern");
                        responseObj->setProperty("tempo", chart->getTempo());
                        responseObj->setProperty("events", midiEvents);
                        responseObj->setProperty("notation", response);
                        
                        resultChart = var(responseObj);
                    }
                    else
                    {
                        DynamicObject* responseObj = new DynamicObject();
                        responseObj->setProperty("success", false);
                        responseObj->setProperty("error", "Failed to generate chart");
                        responseObj->setProperty("response", response);
                        
                        resultChart = var(responseObj);
                    }
                    
                    // Call the callback on the message thread
                    auto callback = currentCallback;
                    bool actualSuccess = (success && chart != nullptr);
                    DBG("DEBUG: Calling callback with actualSuccess: " << (actualSuccess ? "true" : "false"));
                    MessageManager::callAsync([this, callback, actualSuccess, resultChart]() {
                        if (callback)
                            callback(actualSuccess, resultChart);
                        
                        // Notify listeners of the change
                        sendChangeMessage();
                    });
                    
                    // Reset flags after LLM response
                    generating = false;
                    requestInProgress = false;
                });
        }
        else
        {
            // LLM client not available or conditions not met
            auto callback = currentCallback;
            MessageManager::callAsync([callback]() {
                if (callback)
                    callback(false, var("LLM client not available"));
            });
            
            generating = false;
            // Reset the wait event to prevent infinite loop
            waitEvent.reset();
        }
    }
}

const var& Musician::getLastGeneratedChart() const
{
    if (lastGeneratedChart)
    {
        // Convert the chart to the expected var format
        static var chartVar;
        
        DynamicObject* responseObj = new DynamicObject();
        responseObj->setProperty("success", true);
        responseObj->setProperty("command", "play_pattern");
        responseObj->setProperty("tempo", lastGeneratedChart->getTempo());
        responseObj->setProperty("events", midiMessageSequenceToVar(*lastGeneratedChart->toMidiSequence(), lastGeneratedChart->getTempo()));
        responseObj->setProperty("notation", lastResponseText);
        
        chartVar = var(responseObj);
        return chartVar;
    }
    
    static var emptyChart;
    return emptyChart;
}

var Musician::convertNotationToMidiEvents(const String& notation, double tempo)
{
    // Create result object
    DynamicObject::Ptr result = new DynamicObject();
    
    // Parse the notation to create a chart
    auto parser = PatternParser(*patternSystem);
    auto chart = parser.parseLLMResponse(notation);
    
    if (chart)
    {
        // Set the tempo if not already set
        if (chart->getTempo() <= 0)
            chart->setTempo(tempo);
        
        // Convert the chart to MIDI events
        var midiEvents = midiMessageSequenceToVar(*chart->toMidiSequence(), chart->getTempo());
        
        // Create success response
        result->setProperty("success", true);
        result->setProperty("tempo", chart->getTempo());
        result->setProperty("events", midiEvents);
        
        return var(result.get());
    }
    
    // Create failure response
    result->setProperty("success", false);
    result->setProperty("tempo", tempo);
    result->setProperty("events", Array<var>());
    
    return var(result.get());
}

var Musician::midiMessageSequenceToVar(const MidiMessageSequence& sequence, double /*tempo*/) const
{
    Array<var> events;
    
    // Define the ticks per quarter note used in Chart generation (from MusicPatternComponents.cpp)
    const double TICKS_PER_QUARTER_NOTE = 960.0;
    
    // Track note_on events to match with note_off events for duration calculation
    struct NoteOnEvent {
        int note;
        int channel;
        double timestamp;
        float velocity;
        int eventIndex;
        String source;
    };
    
    Array<NoteOnEvent> pendingNoteOns;
    String currentSource = "pattern"; // Track current source from meta events
    
    for (int i = 0; i < sequence.getNumEvents(); ++i)
    {
        auto event = sequence.getEventPointer(i);
        
        // Convert MIDI tick timestamp to normalized beats (quarter notes)
        double originalTimestamp = event->message.getTimeStamp();
        double normalizedTimestamp = originalTimestamp / TICKS_PER_QUARTER_NOTE;
        
        // Debug logging for timestamp conversion
        if (i < 5 && event->message.isNoteOn()) {
            DBG("DEBUG: Converting timestamp - original: " << originalTimestamp << " ticks, normalized: " << normalizedTimestamp << " beats");
        }
        
        if (event->message.isMetaEvent() && event->message.getMetaEventType() == 1)
        {
            // Text meta event containing source information
            currentSource = event->message.getTextFromTextMetaEvent();
        }
        else if (event->message.isNoteOn())
        {
            // Store this note_on event to match with future note_off
            NoteOnEvent noteOn;
            noteOn.note = event->message.getNoteNumber();
            noteOn.channel = event->message.getChannel();
            noteOn.timestamp = normalizedTimestamp;
            noteOn.velocity = event->message.getFloatVelocity();
            noteOn.eventIndex = events.size(); // Index where we'll store this event
            noteOn.source = currentSource;
            
            pendingNoteOns.add(noteOn);
            
            // Create the note event with placeholder duration (will be updated when note_off is found)
            DynamicObject* eventObj = new DynamicObject();
            eventObj->setProperty("type", "note_on");
            eventObj->setProperty("channel", event->message.getChannel());
            eventObj->setProperty("note", event->message.getNoteNumber());
            eventObj->setProperty("velocity", event->message.getFloatVelocity());
            eventObj->setProperty("timestamp", normalizedTimestamp);
            eventObj->setProperty("duration", 0.25); // Default duration, will be updated
            eventObj->setProperty("source", currentSource);
            
            events.add(var(eventObj));
        }
        else if (event->message.isNoteOff())
        {
            // Find the matching note_on event to calculate duration
            int noteNumber = event->message.getNoteNumber();
            int channel = event->message.getChannel();
            
            for (int j = pendingNoteOns.size() - 1; j >= 0; --j)
            {
                if (pendingNoteOns[j].note == noteNumber && pendingNoteOns[j].channel == channel)
                {
                    // Calculate duration
                    double duration = normalizedTimestamp - pendingNoteOns[j].timestamp;
                    
                    // Update the corresponding note_on event with the calculated duration
                    if (auto* eventObj = events[pendingNoteOns[j].eventIndex].getDynamicObject())
                    {
                        eventObj->setProperty("duration", duration);
                    }
                    
                    // Remove this note_on from pending list
                    pendingNoteOns.remove(j);
                    break;
                }
            }
            
            // Note: We don't add note_off events to the output since we now have duration in note_on events
        }
        else if (event->message.isController())
        {
            DynamicObject* eventObj = new DynamicObject();
            eventObj->setProperty("type", "controller");
            eventObj->setProperty("channel", event->message.getChannel());
            eventObj->setProperty("controller", event->message.getControllerNumber());
            eventObj->setProperty("value", event->message.getControllerValue());
            eventObj->setProperty("timestamp", normalizedTimestamp);
            
            events.add(var(eventObj));
        }
        else if (event->message.isProgramChange())
        {
            DynamicObject* eventObj = new DynamicObject();
            eventObj->setProperty("type", "program_change");
            eventObj->setProperty("channel", event->message.getChannel());
            eventObj->setProperty("program", event->message.getProgramChangeNumber());
            eventObj->setProperty("timestamp", normalizedTimestamp);
            
            events.add(var(eventObj));
        }
    }
    
    // Handle any remaining note_on events that didn't get a matching note_off
    // Give them a default duration of 0.25 beats (quarter note)
    for (const auto& pendingNoteOn : pendingNoteOns)
    {
        if (auto* eventObj = events[pendingNoteOn.eventIndex].getDynamicObject())
        {
            double currentDuration = static_cast<double>(eventObj->getProperty("duration"));
            if (currentDuration == 0.25) // Still has the placeholder duration
            {
                eventObj->setProperty("duration", 0.25); // Keep default quarter note duration
                DBG("DEBUG: Note_on without matching note_off, using default duration: note=" << pendingNoteOn.note);
            }
        }
    }
    
    return events;
}

var Musician::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("id", id);
    obj->setProperty("style", style);
    
    // We don't save the LLM client, pattern generator, etc.
    // Those will be recreated when loaded
    
    return var(obj);
}

std::unique_ptr<Musician> Musician::fromVar(const var& v)
{
    if (v.isObject())
    {
        auto* obj = v.getDynamicObject();
        
        if (obj == nullptr)
            return nullptr;
        
        String id = obj->getProperty("id").toString();
        String style = obj->getProperty("style").toString();
        
        return std::make_unique<Musician>(id, style);
    }
    
    return nullptr;
}

} // namespace covertracks