#include <flight/modules/control_tasks/TelemetryControl.hpp>
#include <flight/modules/lib/Util.hpp>
#include <flight/modules/lib/logger_util.hpp>
#include <queue>
#include <flight/modules/mcl/Flag.hpp>
// #include <ArduinoJson.h>


TelemetryControl::TelemetryControl() {
    string msg = "Telemetry control starting";
    global_flag.log_info("response", msg);
}

void TelemetryControl::begin() {
    print("Telemetry control: beginning");
    make_functions();
}

// Store list of all commands that GS can send as functions, add the function pointers to the map and call when necessary
void TelemetryControl::make_functions() {
    print("Telemetry: Making Functions");
    this->functions.emplace("heartbeat", &TelemetryControl::heartbeat);
    this->functions.emplace("soft_abort", &TelemetryControl::soft_abort);
    this->functions.emplace("undo_soft_abort", &TelemetryControl::undo_soft_abort);
    this->functions.emplace("solenoid_actuate", &TelemetryControl::solenoid_actuate);
    this->functions.emplace("sensor_request", &TelemetryControl::sensor_request);
    this->functions.emplace("valve_request", &TelemetryControl::valve_request);
    this->functions.emplace("progress", &TelemetryControl::progress);
    this->functions.emplace("test", &TelemetryControl::test);
}

void TelemetryControl::execute() {
    print("Telemetry control: Controlling");
    if (!global_registry.telemetry.status) {
        global_flag.telemetry.reset = true;
    } else {
        global_flag.telemetry.reset = false;
        auto &ingest_queue = global_registry.telemetry.ingest_queue;
        while (!ingest_queue.empty()) {
            Packet packet = ingest_queue.top();
            ingest_queue.pop();

            string packet_to_str;
            Packet::to_string(packet_to_str, packet);
            print("TelemetryControl packet to string: " + packet_to_str);

            for(const Log& log_ : packet.getLogs()) {
                string to_str_log;
                Log::to_string(to_str_log, log_);
                print("TelemetryControl log to string: " + to_str_log);
                ingest(log_);
            }
        }
    }
}

void TelemetryControl::ingest(const Log& log) {
    string header = log.getHeader();

    // JsonObject message = Util::deserialize(log.getMessage());
    // JsonObject params = Util::deserialize(message["message"].as<string>());

    // string output = log.getMessage();
    // string new_msg_str = output;
    
    // string dump;
    // Log::to_string(dump, log);

    // if(string({dump[0]}) == "{" || string({dump[1]}) == "{") { // if its a converted packet
    //     new_msg_str = Util::replaceAll(new_msg_str, "\\", "");
    //     print("removing slashes");
    //     print(new_msg_str);
    //     // cout << "new str: " << new_msg_str << endl;
    //     // params = json::parse(new_msg_str);
    //     // cout << params.dump() << endl;
    // }

    // Make sure the function exists
    if (this->functions.find(header) == this->functions.end()) {
        print("TelemetryControl Packet Header: " + header);
        throw INVALID_HEADER_ERROR();
    }

    auto function = this->functions.at(header);
    vector<string> argument_order = arguments.at(header);

    // TODO: change the packet format from gs to make it strings instead of enums

    /* if (argument_order.size() != params.size()) {
        throw PACKET_ARGUMENT_ERROR();
    } */

    // output = "";
    // Util::serialize(params, output);
    // print(output);

    // print(params["message"].as<string>());

    vector<string> param_values;

    try {
        for (const string& argument_name : argument_order) {
            param_values.push_back(params[argument_name]);
        }
    } catch (...) {
        string obj = "Invalid function arguments";
        global_flag.log_warning("info", obj);
        throw INVALID_PACKET_ARGUMENTS_ERROR();
    }
    (this->*function)(param_values); // call function which maps to the GS command sent w/ all params necessary
}

void TelemetryControl::heartbeat(const vector<string>& args) {
    // JsonObject obj = Util::deserialize(
    //    "{\"header\": \"heartbeat\", \"response\": \"OK\", \"timestamp\" : \"" + Util::to_string((int) (Util::getTime() - global_flag.general.mcl_start_time) / 1000) + "\" }");
    // global_flag.log_info("heartbeat", obj);
    string header = "HRT"
    string obj = std::format("{}|{}", header, Util::to_string((int) (Util::getTime() - global_flag.general.mcl_start_time) / 1000));
    global_flag.log_info(header, obj);
}

void TelemetryControl::soft_abort(const vector<string>& args) {
    global_registry.general.soft_abort = true;
    string header = "S"
    // JsonObject obj = Util::deserialize("{\"header\": \"Soft Abort\", \"Status\": \"Success\", \"Description\": \"Rocket is undergoing soft abort\"}");
    // global_flag.log_critical("response", obj);
    // JsonObject obj2 = Util::deserialize("{\"header\": \"Soft Abort\", \"mode\": \"Soft Abort\"}");
    // global_flag.log_critical("mode", obj2);
}

void TelemetryControl::undo_soft_abort(const vector<string>& args) {
    global_registry.general.soft_abort = false;
    JsonObject obj = Util::deserialize("{\"header\": \"mode\", \"Status\": \"Success\", \"Description\": \"Undoing soft abort\"}");
    global_flag.log_critical("response", obj);
    JsonObject obj2 = Util::deserialize("{\"header\": \"mode\", \"mode\": \"Normal\"}");
    global_flag.log_critical("mode", obj2);
}
void TelemetryControl::solenoid_actuate(const vector<string>& args) {
    for(const string& s : args) {
        print(s);
    }

    print("SKDJSDKLFJDSLJKF\n\n\n\n\n\n\nSDKLFJDSKLFJSDKLFJSDKLFJSDKLF SOLENOID\n\n");
    if (!global_registry.valve_exists("solenoid", args[0])) {
        JsonObject obj = Util::deserialize("{\"header\": \"Valve actuation\", \"Status\": \"Failure\", \"Description\": \"Unable to find actuatable solenoid\", \"Valve location\": \"" + args[0] + "\"}");
        global_flag.log_critical("Valve actuation", obj);
        throw INVALID_SOLENOID_ERROR();
    }

    int current_priority = int(global_registry.valves["solenoid"][args[0]].actuation_priority);

    if (int(valve_priority_map[args[2]]) < current_priority) {
        JsonObject obj = Util::deserialize("{\"header\": \"Valve actuation\", \"Status\": \"Failure\", \"Description\": \"Priority too low to actuate\", \"Valve location\": \"" + args[0] + "\", \"Actuation type\": \"" + args[1] + "\", \"Priority\": \"" + args[2] + "\"}");
        global_flag.log_critical("Valve actuation", obj);
    }

    print("Actuating solenoid at " + args[0] + " with actuation type " + args[1]);

    try {
        print("\n\ntry\n\ncatch\n\n");
        //TODO: make sure gs packets have the upper case version of the enum as the value for the actuation type
        FlagValveInfo &valve_flag = global_flag.valves["solenoid"][args[0]];
        valve_flag.actuation_type = (ActuationType) std::atoi(args[1].c_str());
        valve_flag.actuation_priority = (ValvePriority) std::atoi(args[2].c_str());
        print("\n\n\nit works\n\n\n\n\n");
    } catch(...) {
        JsonObject obj = Util::deserialize("{\"header\": \"Valve actuation\", \"Status\": \"Failure\", \"Description\": \"Wrong packet message\", \"Valve location\": \"" + args[0] + "\", \"Actuation type\": \"" + args[1] + "\", \"Priority\": \"" + args[2] + "\"}");
        global_flag.log_critical("Valve actuation", obj);
        throw INVALID_PACKET_MESSAGE_ERROR();
    }
    print(args[1]);

    JsonObject obj = Util::deserialize("{\"header\": \"Valve actuation\", \"Status\": \"Success\", \"Description\": \"Successfully actuated solenoid\"}");
    global_flag.log_info("Valve actuation", obj);

    print("REACHED HERE");
}

void TelemetryControl::sensor_request(const vector<string>& args) {
    double value;
    double kalman_value;
    string sensor_status_str;
    string sensor_type = args[0];
    string sensor_loc = args[1];

    if (!global_registry.sensor_exists(sensor_type, sensor_loc)) {
        JsonObject obj = Util::deserialize("{\"header\": \"Sensor data\", \"Status\": \"Failure\", \"Description\": \"Unable to find sensor\", \"Sensor type\": \"" + args[0] + "\", \"Sensor location\": \"" + args[1] + "\"}");
        global_flag.log_critical("response", obj);
        throw INVALID_SENSOR_LOCATION_ERROR();
    }

    auto sensor = global_registry.sensors[sensor_type][sensor_loc];
    value = sensor.measured_value;
    kalman_value = sensor.normalized_value;
    sensor_status_str = sensor_status_map[sensor.status];
    long double millisecond_timestamp = Util::getTime();

    string value_str = Util::to_string((int) value);
    string kalman_str = Util::to_string((int) kalman_value);
    string time_str = Util::to_string((int) (millisecond_timestamp / 1000));

    string toDeserialize = "{\"header\": \"Sensor data request\", \"Status\": \"Success\", \"Sensor type\": \"" + args[0] + "\", \"Sensor location\": \"" + args[1] + "\", \"Sensor status\": \"" + sensor_status_str + "\", \"Measured value\": \"" + value_str + "\", \"Normalized value\": \"" + kalman_str + "\", \"Last updated\": \"" + time_str + "\"}";
    print(toDeserialize);
    JsonObject obj = Util::deserialize(toDeserialize);
    global_flag.log_critical("response", obj);
}

void TelemetryControl::valve_request(const vector<string>& args) {
    string state;
    string actuation_type;
    string actuation_priority;
    string valve_type = args[0];
    string valve_loc = args[1];

    if (!global_registry.valve_exists(valve_type, valve_loc)) {
        print("{\"header\": \"Valve data request\", \"Status\": \"Failure\", \"Description\": Unable to find valve, \"Valve type\": " + valve_type + ", \"Valve location\": " + valve_loc + "}");
        // global_flag.log_critical("response", obj);
        throw INVALID_VALVE_LOCATION_ERROR();
    }

    auto valve_registry = global_registry.valves[valve_type][valve_loc];

    actuation_type = actuation_type_inverse_map.at(valve_registry.actuation_type);
    actuation_priority = valve_priority_inverse_map.at(valve_registry.actuation_priority);
    long double millisecond_timestamp = Util::getTime();

    string time_str = Util::to_string((int) (millisecond_timestamp / 1000));
    JsonObject obj = Util::deserialize("{\"header\": \"Valve data request\", \"Status\": \"Success\", \"Actuation type\": \"" + actuation_type + "\", \"Actuation priority\": \"" + actuation_priority + "\", \"Valve type\": \"" + valve_type + "\", \"Valve location\": \"" + valve_loc + "\", \"Last actuated\": \"" + time_str + "\"}");

    global_flag.log_critical("response", obj);
}
void TelemetryControl::progress(const vector<string>& args) {
    global_flag.general.progress = true;
}
void TelemetryControl::test(const vector<string>& args) {
    print("Test received: " + args[0]);
}