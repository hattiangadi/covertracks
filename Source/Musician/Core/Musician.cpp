// Source/Musician/Core/Musician.cpp
#include "Musician.h"
#include "MusicPatternSystem.h"
#include "PatternGenerator.h"
#include "PatternParser.h"
#include "../../LLM/Core/PromptManager.h"
#include <iostream>

namespace covertracks {

Musician::Musician(const String& idValue, const String& styleValue)
    : id(idValue), style(styleValue), patternSystem(std::make_unique<MusicPatternSystem>()), promptManager(std::make_unique<PromptManager>())
{
    // Initialize pattern generator when LLM client is set
}

Musician::~Musician()
{
    // PatternGenerator will be destroyed automatically
}

void Musician::setLLMClient(std::shared_ptr<LLMClient> client)
{
    llmClient = client;
    
    // Create the pattern generator now that we have an LLM client
    if (llmClient)
    {
        patternGenerator = std::make_unique<PatternGenerator>(*patternSystem, llmClient, *promptManager);
        
        // Set the system prompt
        String systemPrompt = 
            "You are a music pattern generator. Your role is to create music patterns based on the user's requests. "
            "You'll be given specific instructions about the style, instruments, and structure of the music to create. "
            "Always respond in the specified pattern notation format. Do not include any explanations or comments "
            "outside the requested pattern format.";
        
        patternGenerator->setSystemPrompt(systemPrompt);
    }
    else
    {
        patternGenerator.reset();
    }
    
    // Notify listeners of the change
    sendChangeMessage();
}

void Musician::generateChart(const var& parameters, 
                           std::function<void(bool success, const var& chart)> callback)
{
    if (!patternGenerator || !llmClient)
    {
        if (callback)
            callback(false, "No LLM client connected");
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
    
    int tempo = 120;
    if (parameters.hasProperty("tempo") && parameters["tempo"].isInt())
        tempo = parameters["tempo"];
    
    String description = parameters.hasProperty("description") ? parameters["description"].toString() : "";
    
    // Generate the pattern
    patternGenerator->generateSong(styleToUse, instruments, tempo, description, 
        [this, callback, tempo](bool success, std::shared_ptr<Chart> chart, const String& response) {
            // Store the response
            lastResponseText = response;
            lastGeneratedChart = chart;
            
            if (success && chart)
            {
                // Set the tempo if not already set
                if (chart->getTempo() <= 0)
                    chart->setTempo(tempo);
                
                // Convert the chart to MIDI events
                var midiEvents = midiMessageSequenceToVar(*chart->toMidiSequence(), chart->getTempo());
                
                // Create the response object
                DynamicObject* responseObj = new DynamicObject();
                responseObj->setProperty("success", true);
                responseObj->setProperty("command", "play_pattern");
                responseObj->setProperty("tempo", chart->getTempo());
                responseObj->setProperty("events", midiEvents);
                
                // Call the callback
                if (callback)
                    callback(true, var(responseObj));
            }
            else
            {
                // Create an error response
                DynamicObject* responseObj = new DynamicObject();
                responseObj->setProperty("success", false);
                responseObj->setProperty("error", "Failed to generate chart");
                responseObj->setProperty("response", response);
                
                // Call the callback
                if (callback)
                    callback(false, var(responseObj));
            }
            
            // Notify listeners of the change
            sendChangeMessage();
        });
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
    
    // Generate the section
    patternGenerator->generateSection(section, styleToUse, instruments, description, 
        [this, callback](bool success, std::shared_ptr<Chart> chart, const String& response) {
            // Store the response
            lastResponseText = response;
            lastGeneratedChart = chart;
            
            if (success && chart)
            {
                // Convert the chart to MIDI events
                var midiEvents = midiMessageSequenceToVar(*chart->toMidiSequence(), chart->getTempo());
                
                // Create the response object
                DynamicObject* responseObj = new DynamicObject();
                responseObj->setProperty("success", true);
                responseObj->setProperty("command", "play_pattern");
                responseObj->setProperty("tempo", chart->getTempo());
                responseObj->setProperty("events", midiEvents);
                
                // Call the callback
                if (callback)
                    callback(true, var(responseObj));
            }
            else
            {
                // Create an error response
                DynamicObject* responseObj = new DynamicObject();
                responseObj->setProperty("success", false);
                responseObj->setProperty("error", "Failed to generate section");
                responseObj->setProperty("response", response);
                
                // Call the callback
                if (callback)
                    callback(false, var(responseObj));
            }
            
            // Notify listeners of the change
            sendChangeMessage();
        });
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
    
    // Modify the chart
    patternGenerator->modifyPattern(currentNotation, modification, 
        [this, callback](bool success, std::shared_ptr<Chart> chart, const String& response) {
            // Store the response
            lastResponseText = response;
            lastGeneratedChart = chart;
            
            if (success && chart)
            {
                // Convert the chart to MIDI events
                var midiEvents = midiMessageSequenceToVar(*chart->toMidiSequence(), chart->getTempo());
                
                // Create the response object
                DynamicObject* responseObj = new DynamicObject();
                responseObj->setProperty("success", true);
                responseObj->setProperty("command", "play_pattern");
                responseObj->setProperty("tempo", chart->getTempo());
                responseObj->setProperty("events", midiEvents);
                responseObj->setProperty("notation", response);
                
                // Call the callback
                if (callback)
                    callback(true, var(responseObj));
            }
            else
            {
                // Create an error response
                DynamicObject* responseObj = new DynamicObject();
                responseObj->setProperty("success", false);
                responseObj->setProperty("error", "Failed to modify chart");
                responseObj->setProperty("response", response);
                
                // Call the callback
                if (callback)
                    callback(false, var(responseObj));
            }
            
            // Notify listeners of the change
            sendChangeMessage();
        });
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
    // Parse the notation to create a chart
    auto parser = PatternParser(*patternSystem);
    auto chart = parser.parseLLMResponse(notation);
    
    if (chart)
    {
        // Set the tempo if not already set
        if (chart->getTempo() <= 0)
            chart->setTempo(tempo);
        
        // Convert the chart to MIDI events
        var result = midiMessageSequenceToVar(*chart->toMidiSequence(), chart->getTempo());
        
        // Return the MIDI events (don't clear references to avoid destroying the chart)
        return result;
    }
    
    // Return an empty array if parsing failed
    return var(Array<var>());
}

var Musician::midiMessageSequenceToVar(const MidiMessageSequence& sequence, double /*tempo*/) const
{
    Array<var> events;
    
    for (int i = 0; i < sequence.getNumEvents(); ++i)
    {
        auto event = sequence.getEventPointer(i);
        DynamicObject* eventObj = new DynamicObject();
        
        if (event->message.isNoteOn())
        {
            eventObj->setProperty("type", "note_on");
            eventObj->setProperty("channel", event->message.getChannel());
            eventObj->setProperty("note", event->message.getNoteNumber());
            eventObj->setProperty("velocity", event->message.getFloatVelocity());
            eventObj->setProperty("timestamp", event->message.getTimeStamp());
            
            events.add(var(eventObj));
        }
        else if (event->message.isNoteOff())
        {
            eventObj->setProperty("type", "note_off");
            eventObj->setProperty("channel", event->message.getChannel());
            eventObj->setProperty("note", event->message.getNoteNumber());
            eventObj->setProperty("velocity", 0.0f);
            eventObj->setProperty("timestamp", event->message.getTimeStamp());
            
            events.add(var(eventObj));
        }
        else if (event->message.isController())
        {
            eventObj->setProperty("type", "controller");
            eventObj->setProperty("channel", event->message.getChannel());
            eventObj->setProperty("controller", event->message.getControllerNumber());
            eventObj->setProperty("value", event->message.getControllerValue());
            eventObj->setProperty("timestamp", event->message.getTimeStamp());
            
            events.add(var(eventObj));
        }
        else if (event->message.isProgramChange())
        {
            eventObj->setProperty("type", "program_change");
            eventObj->setProperty("channel", event->message.getChannel());
            eventObj->setProperty("program", event->message.getProgramChangeNumber());
            eventObj->setProperty("timestamp", event->message.getTimeStamp());
            
            events.add(var(eventObj));
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