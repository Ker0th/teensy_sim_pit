#pragma once

#include <string>
#include <vector>
#include <windows.h>

class TeensySimPIT {
public:
    TeensySimPIT();
    ~TeensySimPIT();

    // Telemetry / device methods
    void read_iracing_telemetry();
    void write_wind_to_teensy();

    // Serial helpers
    bool open_serial(const std::string& port_name, unsigned int baud = 115200);
    void close_serial();
    bool write_data(const std::vector<uint8_t>& data);
    bool write_string(const std::string& text);

private:
    HANDLE handle_ = nullptr;
};