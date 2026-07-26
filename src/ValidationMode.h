#pragma once

#ifdef NOVA_VALIDATION_MODE

#include <Arduino.h>
#include <RTClib.h>

#include <array>
#include <cstring>

class ValidationMode {
 public:
  void begin(uint32_t nowMs) {
    randomSeed(1);
    selectedAtMs_ = nowMs;
    Serial.println();
    Serial.println("Nova Lights hardware validation firmware");
    printHelp();
    printScenario();
  }

  void update(uint32_t nowMs) {
    readCommands(nowMs);
    if (autoRun_ && nowMs - selectedAtMs_ >= SCENARIO_DURATION_MS) {
      select((selected_ + 1) % SCENARIOS.size(), nowMs);
    }
  }

  DateTime now() const {
    const Scenario& scenario = SCENARIOS[selected_];
    return DateTime(scenario.year, scenario.month, scenario.day, scenario.hour,
                    scenario.minute, scenario.second);
  }

  size_t selected() const { return selected_; }

 private:
  struct Scenario {
    const char* name;
    const char* description;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  };

  static constexpr uint32_t SCENARIO_DURATION_MS = 10000;
  static constexpr std::array<Scenario, 9> SCENARIOS{{
      {"normal", "Monday work-period chase and middle pulse", 2026, 7, 27, 10,
       10, 0},
      {"quarter", "Quarter-hour accent: 25% lit / 75% dark", 2026, 7, 27, 10,
       15, 1},
      {"blend", "Approaching next hour with 40% next-period color", 2026, 7,
       27, 10, 57, 0},
      {"transition", "Fast rainbow at the hourly transition", 2026, 7, 27, 10,
       59, 46},
      {"friday", "Friday random-color cycle", 2026, 7, 31, 10, 10, 0},
      {"sleep", "Scheduled all-lights-off state", 2026, 7, 27, 22, 0, 0},
      {"morning", "Morning blue top-light chase", 2026, 7, 27, 7, 0, 0},
      {"sunday", "Sunday bottom-light white weekday color", 2026, 7, 26, 10,
       10, 0},
      {"saturday", "Saturday bottom-light red weekday color", 2026, 8, 1, 10,
       10, 0},
  }};

  void readCommands(uint32_t nowMs) {
    while (Serial.available() > 0) {
      const char input = static_cast<char>(Serial.read());
      if (input == '\r') {
        continue;
      }
      if (input == '\n') {
        command_[commandLength_] = '\0';
        executeCommand(nowMs);
        commandLength_ = 0;
      } else if (commandLength_ < sizeof(command_) - 1) {
        command_[commandLength_++] = input;
      }
    }
  }

  void executeCommand(uint32_t nowMs) {
    for (size_t index = 0; index < SCENARIOS.size(); ++index) {
      if (std::strcmp(command_, SCENARIOS[index].name) == 0) {
        autoRun_ = false;
        select(index, nowMs);
        return;
      }
    }

    if (std::strcmp(command_, "next") == 0 ||
        std::strcmp(command_, "n") == 0) {
      autoRun_ = false;
      select((selected_ + 1) % SCENARIOS.size(), nowMs);
    } else if (std::strcmp(command_, "previous") == 0 ||
               std::strcmp(command_, "prev") == 0 ||
               std::strcmp(command_, "p") == 0) {
      autoRun_ = false;
      select((selected_ + SCENARIOS.size() - 1) % SCENARIOS.size(), nowMs);
    } else if (std::strcmp(command_, "auto") == 0) {
      autoRun_ = true;
      selectedAtMs_ = nowMs;
      Serial.println("Automatic scenario advance enabled (10 seconds each).");
    } else if (std::strcmp(command_, "hold") == 0) {
      autoRun_ = false;
      Serial.println("Holding the current scenario.");
    } else if (std::strcmp(command_, "list") == 0) {
      printList();
    } else if (std::strcmp(command_, "status") == 0) {
      printScenario();
    } else if (std::strcmp(command_, "help") == 0 ||
               std::strcmp(command_, "?") == 0) {
      printHelp();
    } else if (commandLength_ > 0) {
      Serial.print("Unknown command: ");
      Serial.println(command_);
      printHelp();
    }
  }

  void select(size_t index, uint32_t nowMs) {
    selected_ = index;
    selectedAtMs_ = nowMs;
    randomSeed(1);
    printScenario();
  }

  void printScenario() const {
    const Scenario& scenario = SCENARIOS[selected_];
    Serial.print("[");
    Serial.print(selected_ + 1);
    Serial.print("/");
    Serial.print(SCENARIOS.size());
    Serial.print("] ");
    Serial.print(scenario.name);
    Serial.print(": ");
    Serial.println(scenario.description);
  }

  void printList() const {
    for (size_t index = 0; index < SCENARIOS.size(); ++index) {
      Serial.print(index == selected_ ? "* " : "  ");
      Serial.print(index + 1);
      Serial.print(". ");
      Serial.print(SCENARIOS[index].name);
      Serial.print(" - ");
      Serial.println(SCENARIOS[index].description);
    }
  }

  void printHelp() const {
    Serial.println(
        "Commands: next (n), previous (p), auto, hold, list, status, help");
    Serial.println("You can also enter a scenario name shown by list.");
  }

  size_t selected_ = 0;
  uint32_t selectedAtMs_ = 0;
  bool autoRun_ = false;
  char command_[24]{};
  size_t commandLength_ = 0;
};

constexpr std::array<ValidationMode::Scenario, 9>
    ValidationMode::SCENARIOS;

#endif  // NOVA_VALIDATION_MODE
