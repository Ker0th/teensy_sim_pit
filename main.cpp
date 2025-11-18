#include "teensy_sim_pit.h"

//#include <irsdk_>
#include <iostream>
#include <thread>
#include <signal.h>
#include <Windows.h>


int main()
{
	TeensySimPIT teensySimPit;
	IracingReader iracingReader;
	iracingReader.connectToIracingSDK();

	// after opening serial
	if (!teensySimPit.wait_for_ready(3000)) {
		std::cerr << "Teensy handshake not received; will retry or run in degraded mode\n";
	}
	// Now start sending — only enqueue when handshake has been observed.

	while (iracingReader.isConnected()) {
		carData playerCarData = iracingReader.getPlayerCarData();
		std::cout << "Car Speed: " << playerCarData.speed << std::endl;
		std::cout << "Car RPM: " << playerCarData.rpm << std::endl;
		std::cout << "Throttle: " << playerCarData.throttle << " Brake: " << playerCarData.brake << std::endl;

		teensySimPit.write_string_to_teensy(teensySimPit.convert_cardata_to_string(playerCarData));
	}
    return 0;
}
