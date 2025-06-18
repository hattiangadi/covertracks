#include <iostream>
#include <string>

// Simulate JUCE String for testing
class String {
public:
    std::string str;
    
    String(const std::string& s) : str(s) {}
    String(const char* s) : str(s) {}
    
    String trim() const { return String(str); }
    bool startsWith(const char* prefix) const {
        return str.find(prefix) == 0;
    }
    bool endsWith(const char* suffix) const {
        return str.length() >= strlen(suffix) && 
               str.substr(str.length() - strlen(suffix)) == suffix;
    }
    int indexOf(const char* substr) const {
        size_t pos = str.find(substr);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    int lastIndexOf(const char* substr) const {
        size_t pos = str.rfind(substr);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    String substring(int start) const {
        return String(str.substr(start));
    }
    String substring(int start, int end) const {
        return String(str.substr(start, end - start));
    }
    size_t length() const { return str.length(); }
    
    friend std::ostream& operator<<(std::ostream& os, const String& s) {
        return os << s.str;
    }
};

void testJsonExtraction(const String& response) {
    std::cout << "Testing JSON extraction for response:\n" << response << "\n\n";
    
    String jsonContent = response.trim();
    
    // Check for markdown code block
    if (jsonContent.startsWith("```json"))
    {
        int startIndex = jsonContent.indexOf("\n");
        if (startIndex >= 0)
        {
            jsonContent = jsonContent.substring(startIndex + 1);
            int endIndex = jsonContent.lastIndexOf("```");
            if (endIndex >= 0)
            {
                jsonContent = jsonContent.substring(0, endIndex);
            }
        }
        jsonContent = jsonContent.trim();
    }
    else if (jsonContent.startsWith("```"))
    {
        int startIndex = jsonContent.indexOf("\n");
        if (startIndex >= 0)
        {
            jsonContent = jsonContent.substring(startIndex + 1);
            int endIndex = jsonContent.lastIndexOf("```");
            if (endIndex >= 0)
            {
                jsonContent = jsonContent.substring(0, endIndex);
            }
        }
        jsonContent = jsonContent.trim();
    }
    
    // Find JSON object boundaries if not markdown
    if (!jsonContent.startsWith("{"))
    {
        int jsonStart = jsonContent.indexOf("{");
        if (jsonStart >= 0)
        {
            jsonContent = jsonContent.substring(jsonStart);
        }
    }
    
    if (!jsonContent.endsWith("}"))
    {
        int jsonEnd = jsonContent.lastIndexOf("}");
        if (jsonEnd >= 0)
        {
            jsonContent = jsonContent.substring(0, jsonEnd + 1);
        }
    }
    
    std::cout << "Extracted JSON:\n" << jsonContent << "\n";
    std::cout << "Valid: " << (jsonContent.startsWith("{") && jsonContent.endsWith("}")) << "\n\n";
}

int main() {
    // Test cases
    testJsonExtraction("```json\n{\"test\": \"value\"}\n```");
    testJsonExtraction("Here's the JSON:\n{\"test\": \"value\"}\nThat's it!");
    testJsonExtraction("{\"test\": \"value\"}");
    testJsonExtraction("```\n{\"test\": \"value\"}\n```");
    
    return 0;
}