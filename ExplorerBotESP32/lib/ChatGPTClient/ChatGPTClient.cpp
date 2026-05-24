#include "ChatGPTClient.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

ChatGPTClient::ChatGPTClient(const String& apiKey) : _apiKey(apiKey) {}

String ChatGPTClient::extractOutputText(const String& responseBody) {
  int key = responseBody.indexOf("\"output_text\":");
  if (key < 0) return responseBody;

  int firstQuote = responseBody.indexOf('"', key + 14);
  if (firstQuote < 0) return responseBody;
  int secondQuote = responseBody.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return responseBody;

  String txt = responseBody.substring(firstQuote + 1, secondQuote);
  txt.replace("\\n", "\n");
  txt.replace("\\\"", "\"");
  return txt;
}

String ChatGPTClient::postPrompt(const String& prompt,
                                 const String& model,
                                 const String& endpoint) {
  WiFiClientSecure secureClient;
  secureClient.setInsecure();  // TODO: pin certificate for production.

  HTTPClient http;
  if (!http.begin(secureClient, endpoint)) return "";

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + _apiKey);

  String body = "{";
  body += "\"model\":\"" + model + "\",";
  body += "\"input\":\"" + prompt + "\"";
  body += "}";

  int code = http.POST(body);
  if (code <= 0) {
    http.end();
    return "";
  }

  String response = http.getString();
  http.end();
  return extractOutputText(response);
}

String ChatGPTClient::suggestAction(const String& robotStateJson,
                                    const String& model,
                                    const String& endpoint) {
  String prompt =
      "You are navigation AI for a small robot. Return ONLY one word: "
      "forward, left, right, or stop. State: " +
      robotStateJson;

  String response = postPrompt(prompt, model, endpoint);
  response.toLowerCase();

  if (response.indexOf("forward") >= 0) return "forward";
  if (response.indexOf("left") >= 0) return "left";
  if (response.indexOf("right") >= 0) return "right";
  return "stop";
}

String ChatGPTClient::suggestRequirements(const String& robotStateJson,
                                          float budgetAud,
                                          const String& model,
                                          const String& endpoint) {
  String prompt =
      "You are a robot upgrade planner. Return a short plain text list with up "
      "to 3 upgrades. Each line format: item | max_price_aud | purchase_url | "
      "benefit. Hard budget=" +
      String(budgetAud, 2) + " AUD. State: " + robotStateJson;

  String response = postPrompt(prompt, model, endpoint);
  if (response.length() == 0) {
    return "No upgrade suggestions. Stay in current configuration.";
  }
  return response;
}
