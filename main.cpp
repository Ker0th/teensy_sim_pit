#include "teensy_sim_pit.h"

//#include <irsdk_>
#include <iostream>
#include <thread>
#include <signal.h>
#include <Windows.h>

const char* HARDCODED_WAV_PATH = R"(C:\Code\teensy_sim_pit\out\build\x64-Debug\sounds\bloop.wav)";
int main()
{
	TeensySimPIT teensySimPit;
	IracingReader iracingReader;
	iracingReader.connectToIracingSDK();

	// Start background reader to avoid busy polling in main loop
	iracingReader.start_reader();

	auto sound_path = teensySimPit.get_sound_path("bloop.wav");
	//std::cout << "sound path: " << sound_path << std::endl;
	teensySimPit.setup_miniaudio(HARDCODED_WAV_PATH);

	// after opening serial
	if (!teensySimPit.wait_for_ready(3000)) {
		std::cerr << "Teensy handshake not received; will retry or run in degraded mode\n";
	}
	// Now start sending — only enqueue when handshake has been observed.

	while (iracingReader.isConnected()) {
		// Wait up to 100ms for a new update (reduces CPU usage compared to tight polling)
		if (!iracingReader.wait_for_update(100)) {
			// timed out waiting for data; loop to check connection status again
			continue;
		}

		carData playerCarData = iracingReader.getLatestData();
		teensySimPit.detect_throttle_brake_overlap(playerCarData);
		std::cout << "Car Speed: " << playerCarData.speed << std::endl;
		std::cout << "Car RPM: " << playerCarData.rpm << std::endl;
		std::cout << "Throttle: " << playerCarData.throttle << " Brake: " << playerCarData.brake << std::endl;

		teensySimPit.write_string_to_teensy(teensySimPit.convert_cardata_to_string(playerCarData));
	}

	// stop background reader
	iracingReader.stop_reader();

	teensySimPit.cleanup_miniaudio();
    return 0;
}
