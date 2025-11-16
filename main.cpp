#include "teensy_sim_pit.h"
#include <irsdk_utils.cpp>
#include <irsdk_defines.h>
#include "irsdk_client.h"
//#include <irsdk_>
#include <iostream>
#include <thread>
#include <signal.h>
#include <Windows.h>

// Live weather info, may change as session progresses
irsdkCVar g_AirDensity("AirDensity"); // (float) kg/m^3, Density of air at start/finish line
irsdkCVar g_AirPressure("AirPressure"); // (float) Hg, Pressure of air at start/finish line
irsdkCVar g_AirTemp("AirTemp"); // (float) C, Temperature of air at start/finish line
irsdkCVar g_FogLevel("FogLevel"); // (float) %, Fog level
irsdkCVar g_RelativeHumidity("RelativeHumidity"); // (float) %, Relative Humidity
irsdkCVar g_Skies("Skies"); // (int) Skies (0=clear/1=p cloudy/2=m cloudy/3=overcast)
irsdkCVar g_TrackTempCrew("TrackTempCrew"); // (float) C, Temperature of track measured by crew around track
irsdkCVar g_WeatherType("WeatherType"); // (int) Weather type (0=constant 1=dynamic)
irsdkCVar g_WindDir("WindDir"); // (float) rad, Wind direction at start/finish line
irsdkCVar g_WindVel("WindVel"); // (float) m/s, Wind velocity at start/finish line

// session status
irsdkCVar g_PitsOpen("PitsOpen"); // (bool) True if pit stop is allowed, basically true if caution lights not out
irsdkCVar g_RaceLaps("RaceLaps"); // (int) Laps completed in race
irsdkCVar g_SessionFlags("SessionFlags"); // (int) irsdk_Flags, bitfield
irsdkCVar g_SessionLapsRemain("SessionLapsRemain"); // (int) Laps left till session ends
irsdkCVar g_SessionLapsRemainEx("SessionLapsRemainEx"); // (int) New improved laps left till session ends
irsdkCVar g_SessionNum("SessionNum"); // (int) Session number
irsdkCVar g_SessionState("SessionState"); // (int) irsdk_SessionState, Session state
irsdkCVar g_SessionTick("SessionTick"); // (int) Current update number
irsdkCVar g_SessionTime("SessionTime"); // (double), s, Seconds since session start
irsdkCVar g_SessionTimeOfDay("SessionTimeOfDay"); // (float) s, Time of day in seconds
irsdkCVar g_SessionTimeRemain("SessionTimeRemain"); // (double) s, Seconds left till session ends
irsdkCVar g_SessionUniqueID("SessionUniqueID"); // (int) Session ID

// competitor information, array of up to 64 cars
irsdkCVar g_CarIdxEstTime("CarIdxEstTime"); // (float) s, Estimated time to reach current location on track
irsdkCVar g_CarIdxClassPosition("CarIdxClassPosition"); // (int) Cars class position in race by car index
irsdkCVar g_CarIdxF2Time("CarIdxF2Time"); // (float) s, Race time behind leader or fastest lap time otherwise
irsdkCVar g_CarIdxGear("CarIdxGear"); // (int) -1=reverse 0=neutral 1..n=current gear by car index
irsdkCVar g_CarIdxLap("CarIdxLap"); // (int) Lap count by car index
irsdkCVar g_CarIdxLapCompleted("CarIdxLapCompleted"); // (int) Laps completed by car index
irsdkCVar g_CarIdxLapDistPct("CarIdxLapDistPct"); // (float) %, Percentage distance around lap by car index
irsdkCVar g_CarIdxOnPitRoad("CarIdxOnPitRoad"); // (bool) On pit road between the cones by car index
irsdkCVar g_CarIdxPosition("CarIdxPosition"); // (int) Cars position in race by car index
irsdkCVar g_CarIdxRPM("CarIdxRPM"); // (float) revs/min, Engine rpm by car index
irsdkCVar g_CarRPM("RPM"); // (float) revs/min, Engine rpm by car index
irsdkCVar g_CarIdxSteer("CarIdxSteer"); // (float) rad, Steering wheel angle by car index
irsdkCVar g_CarIdxTrackSurface("CarIdxTrackSurface"); // (int) irsdk_TrkLoc, Track surface type by car index
irsdkCVar g_CarIdxTrackSurfaceMaterial("CarIdxTrackSurfaceMaterial"); // (int) irsdk_TrkSurf, Track surface material type by car index

// new variables
irsdkCVar g_CarIdxLastLapTime("CarIdxLastLapTime"); // (float) s, Cars last lap time
irsdkCVar g_CarIdxBestLapTime("CarIdxBestLapTime"); // (float) s, Cars best lap time
irsdkCVar g_CarIdxBestLapNum("CarIdxBestLapNum"); // (int) Cars best lap number

irsdkCVar g_CarIdxP2P_Status("CarIdxP2P_Status"); // (bool) Push2Pass active or not
irsdkCVar g_CarIdxP2P_Count("CarIdxP2P_Count"); // (int) Push2Pass count of usage (or remaining in Race)

irsdkCVar g_PaceMode("PaceMode"); // (int) irsdk_PaceMode, Are we pacing or not
irsdkCVar g_CarIdxPaceLine("CarIdxPaceLine"); // (int) What line cars are pacing in, or -1 if not pacing
irsdkCVar g_CarIdxPaceRow("CarIdxPaceRow"); // (int) What row cars are pacing in, or -1 if not pacing
irsdkCVar g_CarIdxPaceFlags("CarIdxPaceFlags"); // (int) irsdk_PaceFlags, Pacing status flags for each car

irsdkCVar throttle_raw("ThrottleRaw"); // (float) Raw throttle input 0=off throttle to 1=full throttle
irsdkCVar brake_raw("BrakeRaw"); // (float) Raw brake input 0=brake released to 1=max pedal force

TeensySimPIT teensySimPit;


int main()
{
	for (int i = 0; i < 60*30; i++) {
		teensySimPit.write_wind_to_teensy();
		std::cout << "Connecting to iRacing SDK...\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		if (irsdkClient::instance().waitForData(200)) {
			std::cout << "Connected to iRacing SDK!\n";

			break;
		}
	}
    printf("Reload custom car textures for all cars\n");
	while (irsdkClient::instance().isConnected()) {
		if (irsdkClient::instance().waitForData(16)) {
			printf("Data is available\n");
			float car_speed = irsdkClient::instance().getVarFloat("Speed", 0);
			float carRpm = g_CarRPM.getFloat();
			std::cout << "Car Speed: " << car_speed << std::endl;
			std::cout << "Car RPM: " << carRpm << std::endl;
			float throttle = throttle_raw.getFloat();
			float brake = brake_raw.getFloat();
			std::cout << "Throttle: " << throttle << " Brake: " << brake << std::endl;

		}
	}
    return 0;
}
