/*
* This file and associated .cpp file are licensed under the GPLv3 License Copyright (c) 2024 Sam Groveman
* 
* ESP32Time: https://github.com/fbiego/ESP32Time
*
* Contributors: Sam Groveman
*/

#pragma once
#include <Arduino.h>
#include <GenericOutput.h>
#include <PeriodicTask.h>
#include <ESP32Time.h>

/// @brief A class to control an output (switch) on a timer
class TimerSwitch : public GenericOutput, public PeriodicTask {
	protected:
		/// @brief Describes available button active states
		std::unordered_map<std::string, int> states = {{"Active low", LOW}, {"Active high", HIGH}};

		/// @brief Timer Switch configuration
		struct {
			/// @brief The name of this output
			String name;
			
			/// @brief The time at which the switch will turn on
			String onTime;

			/// @brief The time at which the switch will turn off
			String offTime;

			/// @brief Enable timer
			bool enabled;

			/// @brief The active states of the button
			std::string active;
		} add_config;

		/// @brief A pointer to the time object to use
		ESP32Time* rtc;

		/// @brief The hour which the timer should turn on
		int on_hour;

		/// @brief The minute of hte hour which the timer should turn on
		int on_minute;

		/// @brief The hour which the timer should turn off
		int off_hour;

		/// @brief The minute of hte hour which the timer should turn off
		int off_minute;

		JsonDocument addAdditionalConfig();

	public:
		TimerSwitch(ESP32Time* RTC, int Pin, String ConfigFile = "TimerSwitch.json");
		bool begin();
		String getConfig();
		bool setConfig(String config, bool save);
		void runTask(long elapsed);	
};