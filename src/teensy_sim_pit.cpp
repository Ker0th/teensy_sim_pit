// teensy_sim_pit.cpp : Defines the entry point for the application.
//
/*
PSEUDOCODE / PLAN (detailed):

1. Determine path of the running executable:
   - Use GetModuleFileNameA(nullptr, buffer, MAX_PATH).
   - If this fails, fall back to std::filesystem::current_path().

2. Build the absolute path to the sound file:
   - Use std::filesystem::path to combine executable directory and "sounds/bloop.wav".
   - This avoids relying on the current working directory.

3. Verify the file exists:
   - Use std::filesystem::exists(path) to check.
   - If not found, log an error and return without calling PlaySound.

4. Play the WAV file:
   - Use PlaySoundA with flags: SND_FILENAME | SND_ASYNC | SND_NODEFAULT
     - SND_FILENAME tells PlaySound the string is a filename.
     - SND_ASYNC plays asynchronously.
     - SND_NODEFAULT prevents playing the system default sound on failure.
   - If PlaySound fails, log an error with GetLastError().

5. Keep function small and robust:
   - Do not change global state.
   - Provide clear error messages to help debugging.

This replaces the previous call which omitted SND_FILENAME and SND_NODEFAULT,
which caused PlaySound to play the system error sound when the file wasn't found
or flags were incorrect.
*/

#include "teensy_sim_pit.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <Windows.h>
#include <filesystem>
#include <mutex>

//#define MINIAUDIO_IMPLEMENTATION


TeensySimPIT::TeensySimPIT()
{
}

TeensySimPIT::~TeensySimPIT()
{
    close_serial();
}

// Global engine instance and init-once flag.
// The engine must remain alive for sounds to keep playing asynchronously.
static ma_engine g_miniaudio_engine;
static std::once_flag g_miniaudio_init_flag;
static bool g_miniaudio_init_ok = false;

void TeensySimPIT::detect_throttle_brake_overlap(const carData& data)
{
    const float overlap_threshold = 0.05f; // threshold to consider both throttle and brake as "pressed"
    if (data.throttle > overlap_threshold && data.brake > overlap_threshold) {
        std::cout << "Warning: Throttle and Brake overlap detected! Throttle: " 
                  << data.throttle << ", Brake: " << data.brake << '\n';
        this->play_bloop_sound();
    }
}

const char* TeensySimPIT::get_sound_path(const std::string &filename)
{
    // Get executable path
    char exePath[MAX_PATH] = { 0 };
    DWORD len = GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    std::filesystem::path wavPath;
    if (len == 0 || exePath[0] == '\0') {
        // Fallback to current working directory if we couldn't get module path
        wavPath = std::filesystem::current_path() / "sounds" / filename;
    }
    else {
        std::filesystem::path exeDir = std::filesystem::path(std::string(exePath)).parent_path();
        wavPath = exeDir / "sounds" / filename;
    }

    if (!std::filesystem::exists(wavPath)) {
        std::cout << "bloop sound file not found: " << wavPath.string() << std::endl;
        return nullptr;
    }
    std::string m_soundPathStorage = wavPath.string();
    std::cout << "sound path: " << m_soundPathStorage << std::endl;
    return m_soundPathStorage.c_str();
}

bool TeensySimPIT::setup_miniaudio(const char* wavFilePath)
{
    ma_result result;

    // A. Initialize the audio engine (ONCE)
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize engine." << std::endl;
        return false;
    }

    // B. Initialize the sound object from file (ONCE)
    // Use MA_SOUND_FLAG_DECODE to load the data into memory for quick restarting
    result = ma_sound_init_from_file(&engine, wavFilePath, MA_SOUND_FLAG_DECODE, NULL, NULL, &sound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize sound. Error: " << result << std::endl;
        ma_engine_uninit(&engine);
        return false;
    }
    // In your setup function
    ma_sound_set_volume(&sound, 1.0f);

    ma_uint64 length_in_frames = 0;
    ma_sound_get_length_in_pcm_frames(&sound, &length_in_frames);

    if (length_in_frames == 0) {
        std::cerr << "CRITICAL WARNING: Sound initialized successfully, but length is 0 frames!" << std::endl;
        std::cerr << "File is either empty, corrupt, or Miniaudio failed decoding silently." << std::endl;
        // You may want to treat this as a fatal failure, even though result was MA_SUCCESS
        // ma_sound_uninit(&sound); 
        // ma_engine_uninit(&engine);
        // return false; 
    }
    else {
        std::cout << "DEBUG: Sound initialized with " << length_in_frames << " PCM frames of data." << std::endl;
    }
    // ---------------------------------

    std::cout << "Miniaudio Setup complete. Sound is ready to play." << std::endl;
    return true;
}

void TeensySimPIT::cleanup_miniaudio()
{
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
    std::cout << "Miniaudio cleaned up." << std::endl;
}

void TeensySimPIT::play_bloop_sound()
{
    if (!ma_sound_is_playing(&this->sound)) {
        // 3. Start the playback
        ma_sound_start(&this->sound);
        std::cout << "starting to play sound" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    //ma_sound_stop(&sound);
    // 1. Stop any current playback of this sound object

    // 2. Seek back to the beginning of the file/data
    // This ensures it starts from the start, not where it was stopped.
    //ma_sound_seek_to_pcm_frame(&sound, 0);

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


bool TeensySimPIT::wait_for_ready(unsigned int timeout_ms)
{
    
    return true;
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
