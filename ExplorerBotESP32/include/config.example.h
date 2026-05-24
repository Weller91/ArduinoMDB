#pragma once

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define OPENAI_API_KEY "sk-..."

// OpenAI Responses API endpoint
#define OPENAI_ENDPOINT "https://api.openai.com/v1/responses"

// Cost-aware model selection
#define OPENAI_MODEL "gpt-4.1-mini"

// FTP destination for telemetry + upgrade requirements
#define FTP_HOST "192.168.1.10"
#define FTP_PORT 21
#define FTP_USER "robot"
#define FTP_PASSWORD "robot_password"
#define FTP_BASE_PATH "/explorerbot"

// Device identity / cadence
#define ROBOT_ID "esp32s3-explorer-01"
#define DECISION_INTERVAL_MS 5000
#define REQUIREMENTS_UPLOAD_MS 60000

// Budget limit for parts upgrades (Australian dollars)
#define UPGRADE_BUDGET_AUD 30.0f
