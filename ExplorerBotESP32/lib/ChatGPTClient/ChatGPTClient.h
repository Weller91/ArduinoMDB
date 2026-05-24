#pragma once

#include <Arduino.h>

class ChatGPTClient {
 public:
  explicit ChatGPTClient(const String& apiKey);

  String suggestAction(const String& robotStateJson,
                       const String& model,
                       const String& endpoint);

  String suggestRequirements(const String& robotStateJson,
                             float budgetAud,
                             const String& model,
                             const String& endpoint);

 private:
  String _apiKey;

  String postPrompt(const String& prompt,
                    const String& model,
                    const String& endpoint);
  String extractOutputText(const String& responseBody);
};
