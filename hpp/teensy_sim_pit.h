#pragma once
//#include <irsdk_utils.cpp>
#include <irsdk_defines.h>
#include "irsdk_client.h"
#include <string>
#include <vector>
#include <windows.h>

#include "miniaudio.h"

// Define the implementation in this translation unit only.
//#define MINIAUDIO_IMPLEMENTATION
//#include "dependencies/miniaudio.h"

// Forward declare irsdkCVar before using it in IracingReader
//class irsdkCVar;
struct carData {
    float speed;
    float rpm;
    float throttle;
    float brake;
};
class IracingReader {
public:
    IracingReader() = default;
    ~IracingReader() = default;

	void connectToIracingSDK();
    bool isConnected();

	carData getCarData(int carIndex);
	carData getPlayerCarData();

private:
    int disconnect_count;

    irsdkCVar AirDensity{"AirDensity"}; // (float) kg/m^3, Density of air at start/finish line
    irsdkCVar AirPressure{"AirPressure"}; // (float) Hg, Pressure of air at start/finish line
    irsdkCVar AirTemp{"AirTemp"}; // (float) C, Temperature of air at start/finish line
    irsdkCVar FogLevel{"FogLevel"}; // (float) %, Fog level
    irsdkCVar RelativeHumidity{"RelativeHumidity"}; // (float) %, Relative Humidity
    irsdkCVar Skies{"Skies"}; // (int) Skies (0=clear/1=p cloudy/2=m cloudy/3=overcast)
    irsdkCVar TrackTempCrew{"TrackTempCrew"}; // (float) C, Temperature of track measured by crew around track
    irsdkCVar WeatherType{"WeatherType"}; // (int) Weather type (0=constant 1=dynamic)
    irsdkCVar WindDir{"WindDir"}; // (float) rad, Wind direction at start/finish line
    irsdkCVar WindVel{"WindVel"}; // (float) m/s, Wind velocity at start/finish line

    // session status
    irsdkCVar PitsOpen{"PitsOpen"}; // (bool) True if pit stop is allowed, basically true if caution lights not out
    irsdkCVar RaceLaps{"RaceLaps"}; // (int) Laps completed in race
    irsdkCVar SessionFlags{"SessionFlags"}; // (int) irsdk_Flags, bitfield
    irsdkCVar SessionLapsRemain{"SessionLapsRemain"}; // (int) Laps left till session ends
    irsdkCVar SessionLapsRemainEx{"SessionLapsRemainEx"}; // (int) New improved laps left till session ends
    irsdkCVar SessionNum{"SessionNum"}; // (int) Session number
    irsdkCVar SessionState{"SessionState"}; // (int) irsdk_SessionState, Session state
    irsdkCVar SessionTick{"SessionTick"}; // (int) Current update number
    irsdkCVar SessionTime{"SessionTime"}; // (double), s, Seconds since session start
    irsdkCVar SessionTimeOfDay{"SessionTimeOfDay"}; // (float) s, Time of day in seconds
    irsdkCVar SessionTimeRemain{"SessionTimeRemain"}; // (double) s, Seconds left till session ends
    irsdkCVar SessionUniqueID{"SessionUniqueID"}; // (int) Session ID

    // competitor information, array of up to 64 cars
    irsdkCVar CarIdxEstTime{"CarIdxEstTime"}; // (float) s, Estimated time to reach current location on track
    irsdkCVar CarIdxClassPosition{"CarIdxClassPosition"}; // (int) Cars class position in race by car index
    irsdkCVar CarIdxF2Time{"CarIdxF2Time"}; // (float) s, Race time behind leader or fastest lap time otherwise
    irsdkCVar CarIdxGear{"CarIdxGear"}; // (int) -1=reverse 0=neutral 1..n=current gear by car index
    irsdkCVar CarIdxLap{"CarIdxLap"}; // (int) Lap count by car index
    irsdkCVar CarIdxLapCompleted{"CarIdxLapCompleted"}; // (int) Laps completed by car index
    irsdkCVar CarIdxLapDistPct{"CarIdxLapDistPct"}; // (float) %, Percentage distance around lap by car index
    irsdkCVar CarIdxOnPitRoad{"CarIdxOnPitRoad"}; // (bool) On pit road between the cones by car index
    irsdkCVar CarIdxPosition{"CarIdxPosition"}; // (int) Cars position in race by car index
    irsdkCVar CarIdxRPM{"CarIdxRPM"}; // (float) revs/min, Engine rpm by car index
    irsdkCVar CarRPM{"RPM"}; // (float) revs/min, Engine rpm by car index
    irsdkCVar CarIdxSteer{"CarIdxSteer"}; // (float) rad, Steering wheel angle by car index
    irsdkCVar CarIdxTrackSurface{"CarIdxTrackSurface"}; // (int) irsdk_TrkLoc, Track surface type by car index
    irsdkCVar CarIdxTrackSurfaceMaterial{"CarIdxTrackSurfaceMaterial"}; // (int) irsdk_TrkSurf, Track surface material type by car index

    // new variables
    irsdkCVar CarIdxLastLapTime{"CarIdxLastLapTime"}; // (float) s, Cars last lap time
    irsdkCVar CarIdxBestLapTime{"CarIdxBestLapTime"}; // (float) s, Cars best lap time
    irsdkCVar CarIdxBestLapNum{"CarIdxBestLapNum"}; // (int) Cars best lap number

    irsdkCVar CarIdxP2P_Status{"CarIdxP2P_Status"}; // (bool) Push2Pass active or not
    irsdkCVar CarIdxP2P_Count{"CarIdxP2P_Count"}; // (int) Push2Pass count of usage (or remaining in Race)

    irsdkCVar PaceMode{"PaceMode"}; // (int) irsdk_PaceMode, Are we pacing or not
    irsdkCVar CarIdxPaceLine{"CarIdxPaceLine"}; // (int) What line cars are pacing in, or -1 if not pacing
    irsdkCVar CarIdxPaceRow{"CarIdxPaceRow"}; // (int) What row cars are pacing in, or -1 if not pacing
    irsdkCVar CarIdxPaceFlags{"CarIdxPaceFlags"}; // (int) irsdk_PaceFlags, Pacing status flags for each car

    irsdkCVar throttle_raw{"ThrottleRaw"}; // (float) Raw throttle input 0=off throttle to 1=full throttle
    irsdkCVar brake_raw{"BrakeRaw"}; // (float) Raw brake input 0=brake released to 1=max pedal force

};


class TeensySimPIT {
public:
    TeensySimPIT();
    ~TeensySimPIT();

	void detect_throttle_brake_overlap(const carData& data);
    const char* get_sound_path(const std::string &filename);
    bool setup_miniaudio(const char* wavFilePath);
    void cleanup_miniaudio();
	void play_bloop_sound();
    void write_string_to_teensy(const std::string &data);
	std::string convert_cardata_to_string(const carData& data);

    // Serial helpers
    bool open_serial(const std::string& port_name, unsigned int baud = 115200);
    void close_serial();
    bool write_data(const std::vector<uint8_t>& data);
    bool write_string(const std::string& text);
    bool wait_for_ready(unsigned int timeout_ms = 5000); // returns true if handshake seen

private:
    HANDLE handle_ = nullptr;
    std::string m_soundPathStorage;

    ma_engine engine;
    ma_sound sound;
};
