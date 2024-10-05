#include "TimerSwitch.h"

/// @brief Creates a new TimerSwitch
/// @param RTC A pointer to a ESP32Time object to use
/// @param Pin Pin to use
/// @param ConfigFile The name of the config file to use
TimerSwitch::TimerSwitch(ESP32Time* RTC, int Pin, String ConfigFile) : GenericOutput(Pin, ConfigFile) {
	rtc = RTC;
}

/// @brief Starts a timer switch 
/// @return True on success
bool TimerSwitch::begin() {
	bool configExists = checkConfig(config_path);
	task_config = { .taskName = "Timer Switch", .taskPeriod = 30000 };
	if (GenericOutput::begin()) {
		// Set description
		Description.type = "output";
		Description.name = "Timer Switch";
		Description.actions = {{"state", 0}};
		Description.id = 0;
		if (!configExists) {
			// Set defaults
			return setConfig(R"({"pin":)" + String(output_config.Pin) + R"(, "name": "Timer Switch", "onTime": "9:30", "offTime": "22:15", "enabled": false, "active": "Active high"})", true);
		} else {
			// Load settings
			return setConfig(Storage::readFile(config_path), false);
		}
	}
	return false;
}

/// @brief Gets the current config
/// @return A JSON string of the config
String TimerSwitch::getConfig() {
	// Allocate the JSON document
	JsonDocument doc = addAdditionalConfig();

	// Create string to hold output
	String output;
	// Serialize to string
	serializeJson(doc, output);
	return output;
}

/// @brief Sets the configuration for this device
/// @param config A JSON string of the configuration settings
/// @param save If the configuration should be saved to a file
/// @return True on success
bool TimerSwitch::setConfig(String config, bool save) {
	if (GenericOutput::setConfig(config, false)) {
		// Allocate the JSON document
		JsonDocument doc;
		// Deserialize file contents
		DeserializationError error = deserializeJson(doc, config);
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			return false;
		}
		// Disable task in case name changed
		if (!enableTask(false)) {
			return false;
		}
		// Assign loaded values
		add_config.name = doc["name"].as<String>();
		add_config.onTime = doc["onTime"].as<String>();
		add_config.offTime = doc["offTime"].as<String>();
		add_config.enabled = doc["enabled"].as<bool>();
		add_config.active = doc["active"]["current"].as<std::string>();

		Description.name = add_config.name;
		task_config.taskName = add_config.name.c_str();
		on_hour = add_config.onTime.substring(0, add_config.onTime.indexOf(':')).toInt();
		on_minute = add_config.onTime.substring(add_config.onTime.indexOf(':') + 1).toInt();
		off_hour = add_config.offTime.substring(0, add_config.offTime.indexOf(':')).toInt();
		off_minute = add_config.offTime.substring(add_config.offTime.indexOf(':') + 1).toInt();
		if (save) {
			if (!saveConfig(config_path, getConfig())) {
				return false;
			}
		}
		return enableTask(add_config.enabled) && configureOutput();
		}
	return false;
}

/// @brief Checks the time to see if the timer has triggered
/// @param elapsed The time in ms since this task was last called
void TimerSwitch::runTask(long elapsed) {
	if (add_config.enabled && taskPeriodTriggered(elapsed)) {
		int cur_hour = rtc->getHour(true);
		int cur_min = rtc->getMinute();
		int cur_state = digitalRead(output_config.Pin);
		if (cur_state != states[add_config.active] && cur_hour == on_hour) {
			if (cur_min == on_minute) {
				Logger.println("Timer switch turning on");
				digitalWrite(output_config.Pin, states[add_config.active]);
			}
		} else if (cur_state == states[add_config.active] && cur_hour == off_hour) {
			if (cur_min == off_minute) {
				Logger.println("Timer switch turning off");
				digitalWrite(output_config.Pin, !states[add_config.active]);
			}
		}
	}
}

/// @brief Collects all the base class parameters and additional parameters
/// @return a JSON document with all the parameters
JsonDocument TimerSwitch::addAdditionalConfig() {
	// Allocate the JSON document
  	JsonDocument doc;
	// Deserialize file contents
	DeserializationError error = deserializeJson(doc, GenericOutput::getConfig());
	// Test if parsing succeeds.
	if (error) {
		Logger.print(F("Deserialization failed: "));
		Logger.println(error.f_str());
		return doc;
	}
	doc["name"] = add_config.name;
	doc["onTime"] = add_config.onTime;
	doc["offTime"] = add_config.offTime;
	doc["enabled"] = add_config.enabled;
	doc["active"]["current"] =  add_config.active;
	doc["active"]["options"][0] = "Active low";
	doc["active"]["options"][1] = "Active high";
	return doc;
}