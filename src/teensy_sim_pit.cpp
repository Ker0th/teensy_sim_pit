// teensy_sim_pit.cpp : Defines the entry point for the application.
//
#pragma once
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

void TeensySimPIT::write_string_to_teensy(const std::string &data)
{
    // Example: open COM3 (change as needed), send a simple text payload and close.
    const std::string port = "COM4"; // change to your port, e.g. "COM4"
    if (!open_serial(port, 115200)) {
        std::cerr << "Failed to open serial port " << port << '\n';
        return;
    }

    if (!write_string(data)) {
        std::cerr << "Failed to write payload\n";
    }

    // small delay to ensure device receives data
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    close_serial();
}

std::string TeensySimPIT::convert_cardata_to_string(const carData& data)
{
    std::string result = 
		"carData|SPEED:" + std::to_string(data.speed) +
        ",RPM:" + std::to_string(data.rpm) +
        ",THROTTLE:" + std::to_string(data.throttle) +
		",BRAKE:" + std::to_string(data.brake) + "|";

    return result;
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

void IracingReader::connectToIracingSDK()
{
    for (int i = 0; i < 60 * 30; i++) {
		std::cout << "Connecting to iRacing SDK... " << "timeout in: " << 60 * 30 - i << " seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (irsdkClient::instance().waitForData(200)) {
            std::cout << "Connected to iRacing SDK!\n";
            break;
        }
    }
}

bool IracingReader::isConnected()
{
    if (irsdkClient::instance().isConnected()) {
        disconnect_count = 0;
    } else if (disconnect_count < 5) {
        disconnect_count++;
	} else {
        std::cout << "Disconnected from iRacing SDK!\n";
        return false;
	}

    return true;
}

carData IracingReader::getCarData(int carIndex)
{
    return carData();
}

carData IracingReader::getPlayerCarData()
{
	carData data;
    if (irsdkClient::instance().waitForData(16)) {
		data.speed = irsdkClient::instance().getVarFloat("Speed", 0) * 3.6; // from m/s to km/h
	    data.rpm = irsdkClient::instance().getVarFloat("RPM", 0);
	    data.throttle = irsdkClient::instance().getVarFloat("ThrottleRaw", 0);
	    data.brake = irsdkClient::instance().getVarFloat("BrakeRaw", 0);
    }
	return data;
}
