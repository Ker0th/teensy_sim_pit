// teensy_sim_pit.cpp : Defines the entry point for the application.
//
#include "teensy_sim_pit.h"

#include <iostream>
#include <chrono>
#include <thread>

TeensySimPIT::TeensySimPIT()
{
}

TeensySimPIT::~TeensySimPIT()
{
    close_serial();
}

void TeensySimPIT::read_iracing_telemetry()
{
    // stub
}

void TeensySimPIT::write_wind_to_teensy()
{
    // Example: open COM3 (change as needed), send a simple text payload and close.
    const std::string port = "COM3"; // change to your port, e.g. "COM4"
    if (!open_serial(port, 115200)) {
        std::cerr << "Failed to open serial port " << port << '\n';
        return;
    }

    const std::string payload = "WIND:123\n";
    if (!write_string(payload)) {
        std::cerr << "Failed to write payload\n";
    }

    // small delay to ensure device receives data
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    close_serial();
}

bool TeensySimPIT::write_string(const std::string& text)
{
    std::vector<uint8_t> bytes(text.begin(), text.end());
    return write_data(bytes);
}

bool TeensySimPIT::write_data(const std::vector<uint8_t>& data)
{
    if (!handle_) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(handle_, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
    if (!ok || written != data.size()) {
        std::cerr << "WriteFile failed, err=" << GetLastError() << '\n';
        return false;
    }
    return true;
}

bool TeensySimPIT::open_serial(const std::string& port_name, unsigned int baud)
{
    if (handle_) close_serial();

    std::string path = port_name;
    // For COM ports >= COM10 on Windows, prepend "\\.\"
    if (port_name.size() >= 4 && port_name.rfind("COM", 0) == 0) {
        int num = 0;
        try { num = std::stoi(port_name.substr(3)); } catch (...) { num = 0; }
        if (num >= 10) path = "\\\\.\\" + port_name;
    }

    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFileA failed for " << path << ", err=" << GetLastError() << '\n';
        return false;
    }

    // Setup baud rate and serial parameters
    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &dcb)) {
        std::cerr << "GetCommState failed, err=" << GetLastError() << '\n';
        CloseHandle(h);
        return false;
    }

    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState(h, &dcb)) {
        std::cerr << "SetCommState failed, err=" << GetLastError() << '\n';
        CloseHandle(h);
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &timeouts);

    handle_ = h;
    return true;
}

void TeensySimPIT::close_serial()
{
    if (handle_) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
}