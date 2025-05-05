#include "TimerSwitch.h"

/// @brief Creates a new TimerSwitch
/// @param Name The device name 
/// @param Pin Pin to use
/// @param ConfigFile The name of the config file to use
TimerSwitch::TimerSwitch(String Name, int Pin, String ConfigFile) : GenericOutput(Name, Pin, ConfigFile) {}

/// @brief Starts a timer switch 
/// @return True on success
bool TimerSwitch::begin() {
	bool configExists = checkConfig(config_path);
	if (GenericOutput::begin()) {
		// Set description
		Description.type = "output";
		Description.actions = {{"state", 0}};
		if (!configExists) {
			// Set defaults 
			task_config.set_taskName(Description.name.c_str());
			task_config.taskPeriod = 1000;
			timer_config.onTime = "9:30";
			timer_config.offTime = "22:15";
			timer_config.enabled = false;
			timer_config.active = "Active High";
			return setConfig(getConfig(), true);
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
		// Assign loaded values
		Description.name = doc["Name"].as<String>();
		timer_config.onTime = doc["onTime"].as<String>();
		timer_config.offTime = doc["offTime"].as<String>();
		timer_config.enabled = doc["enabled"].as<bool>();
		timer_config.active = doc["active"]["current"].as<std::string>();
		task_config.set_taskName(Description.name.c_str());
		task_config.taskPeriod = doc["taskPeriod"].as<long>();

		on_hour = timer_config.onTime.substring(0, timer_config.onTime.indexOf(':')).toInt();
		on_minute = timer_config.onTime.substring(timer_config.onTime.indexOf(':') + 1).toInt();
		off_hour = timer_config.offTime.substring(0, timer_config.offTime.indexOf(':')).toInt();
		off_minute = timer_config.offTime.substring(timer_config.offTime.indexOf(':') + 1).toInt();
		if (save) {
			if (!saveConfig(config_path, getConfig())) {
				return false;
			}
		}
		return enableTask(timer_config.enabled) && configureOutput();
		}
	return false;
}

/// @brief Checks the time to see if the timer has triggered
/// @param elapsed The time in ms since this task was last called
void TimerSwitch::runTask(long elapsed) {
	if (timer_config.enabled && taskPeriodTriggered(elapsed)) {
		int cur_hour = TimeInterface::getFormattedTime("%H").toInt();
		int cur_min = TimeInterface::getFormattedTime("%M").toInt();
		int cur_state = digitalRead(output_config.Pin);
		if (cur_state != states[timer_config.active] && cur_hour == on_hour && cur_min == on_minute) {
			Logger.println("Timer switch turning on");
			digitalWrite(output_config.Pin, states[timer_config.active]);
		} else if (cur_state == states[timer_config.active] && cur_hour == off_hour && cur_min == off_minute) {
			Logger.println("Timer switch turning off");
			digitalWrite(output_config.Pin, !states[timer_config.active]);
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
	doc["Name"] = Description.name;
	doc["onTime"] = timer_config.onTime;
	doc["offTime"] = timer_config.offTime;
	doc["enabled"] = timer_config.enabled;
	doc["active"]["current"] =  timer_config.active;
	doc["active"]["options"][0] = "Active low";
	doc["active"]["options"][1] = "Active high";
	doc["taskPeriod"] = task_config.taskPeriod;
	return doc;
}