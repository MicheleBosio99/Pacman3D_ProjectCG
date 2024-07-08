
#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include "irrKlang.h"
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>


class SoundManager {

    public:

        static irrklang::ISoundEngine* engine;
        static std::unordered_map<std::string, irrklang::ISound*> soundsPlaying;
        static std::unordered_map<std::string, irrklang::ISoundSource*> soundSources;

        static std::queue<std::string> soundQueue;

        static std::thread soundQueueThread;
        static std::mutex queueMutex;
        static bool isQueueThreadRunning;

        SoundManager() = delete;
        SoundManager(const SoundManager&) = delete;
        SoundManager& operator=(const SoundManager&) = delete;

        // Initialize the sound manager;
        static bool initSoundManager() {
            engine = irrklang::createIrrKlangDevice(irrklang::ESOD_AUTO_DETECT, irrklang::ESEO_DEFAULT_OPTIONS); // &~irrklang::ESEO_PRINT_DEBUG_INFO_TO_STDOUT);
            if (!engine) { std::cerr << "Error: Could not initialize irrKlang Sound Engine." << std::endl; return false; }

            engine->setRolloffFactor(5.0f); // Set the logaritmic rolloff factor of 3d sounds;

            return true;
        }

        // Deinitialize the sound manager;
        static void deInitSoundManager() { if (engine) { engine->drop(); engine = nullptr; } }

        // Load a sound from file;
        static bool loadSound(const std::string& name, const std::string& filename) {
            checkInitialization();

            irrklang::ISoundSource* soundSource = engine->addSoundSourceFromFile(filename.c_str());
            if (!soundSource) { std::cerr << "Error: Failed to load sound: " << filename << std::endl; return false; }
            soundSources[name] = soundSource;
            return true;
        }

        // Play a sound;
        static void playSound(const std::string& name, float volume = 1.0f) {
            checkInitialization();

            auto it = soundSources.find(name);
            if (it != soundSources.end()) {
                auto sound = engine->play2D(it->second, false, false, true, false);
                if (sound) { soundsPlaying[name] = sound; sound->setVolume(volume); }
            }
            else { std::cerr << "Error: Sound not found: " << name << std::endl; }
        }

        // Play a sound in loop;
        static void playSoundLooped(const std::string& name, float volume = 1.0f, float speed = 1.0f) {
            checkInitialization();

            auto it = soundSources.find(name);
            if (it != soundSources.end()) {
                auto sound = engine->play2D(it->second, true, false, true, false);
                if (sound) { soundsPlaying[name] = sound; sound->setVolume(volume); sound->setPlaybackSpeed(speed); }
            }
            else { std::cerr << "Error: Sound not found: " << name << std::endl; }
        }

        // Check if a sound is playing by sound name;
        static bool isSoundPlaying(const std::string& name) {
            checkInitialization();

            auto it = soundsPlaying.find(name);
            if (it == soundsPlaying.end()) { return false; }

            return !it->second->isFinished();
        }

        // Check if a sound is playing by sound pointer;
        static bool isSoundPlaying(irrklang::ISound* sound) {
            checkInitialization();
            return !sound->isFinished();
        }

        // Stop a playing sound;
        static void stopSound(const std::string& name) {
            checkInitialization();

            auto it = soundsPlaying.find(name); // Search in soundsPlaying, not soundSources
            if (it != soundsPlaying.end()) { it->second->stop(); soundsPlaying.erase(it); }
            else { std::cerr << "Error: Sound not playing: " << name << std::endl; }
        }

        // Play a sound in 3D space;
        static irrklang::ISound* playSound3D(const std::string& name, const irrklang::vec3df& position, bool looped = false, bool startPaused = true) {
            checkInitialization();

            auto it = soundSources.find(name);
            if (it != soundSources.end()) {
                irrklang::ISound* sound = engine->play3D(it->second, position, looped, startPaused, true, false);
                if (sound) { soundsPlaying[name] = sound; return sound; }
            }
            else { std::cerr << "Error: Sound not found: " << name << std::endl; }
            return nullptr;
        }

        // Update the position of a 3D sound;
        static void updateSoundPosition(const std::string& name, const irrklang::vec3df& position) {
            checkInitialization();

            auto it = soundsPlaying.find(name);
            if (it != soundsPlaying.end()) { it->second->setPosition(position); }
            else { std::cerr << "Error: Sound not playing or not a 3D sound: " << name << std::endl; }
        }

        // Update the direction of a 3D sound;
        /*static void updateSoundDirection(const std::string& name, const irrklang::vec3df& direction) {
            checkInitialization();

            auto it = soundsPlaying.find(name);
            if (it != soundsPlaying.end()) { it->second->setVelocity(direction); }
            else { std::cerr << "Error: Sound not playing or not a 3D sound: " << name << std::endl; }
        }*/

        static void queueSound(const std::string& name) {
            checkInitialization();

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                soundQueue.push(name);
            }

            if (!isQueueThreadRunning) {
                isQueueThreadRunning = true;

                soundQueueThread = std::thread([]() {
                    while (!soundQueue.empty()) {
                        std::string name;
                        {
                            std::lock_guard<std::mutex> lock(queueMutex);
                            name = soundQueue.front();

                            if (isSoundPlaying(name)) { break; }

                            soundQueue.pop();
                        }
                        playSound(name);
                    }
                    isQueueThreadRunning = false;
                });

                soundQueueThread.detach();
            }
        }


    // private:

        static void checkInitialization() { if (!engine) { std::cerr << "Error: SoundManager not initialized. Call initSoundManager() first." << std::endl; return; } }

        static void printMap() { for (auto& [name, sound] : soundsPlaying) { std::cout << name << std::endl; } }
};

// Initialize static members
irrklang::ISoundEngine* SoundManager::engine = nullptr;
std::unordered_map<std::string, irrklang::ISoundSource*> SoundManager::soundSources;
std::unordered_map<std::string, irrklang::ISound*> SoundManager::soundsPlaying;
std::queue<std::string> SoundManager::soundQueue;

std::thread SoundManager::soundQueueThread;
std::mutex SoundManager::queueMutex;
bool SoundManager::isQueueThreadRunning = false;



#endif





/*


#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <unordered_map>
#include <string>
#include <iostream>
#include <mutex>

#undef max
#undef min

class SoundManager {

    public:

        // Prevent default construction
        SoundManager() = delete;
        SoundManager(const SoundManager&) = delete; // Prevent copy construction
        SoundManager& operator=(const SoundManager&) = delete; // Prevent assignment


        // Initialize the sound manager;
        static bool initSoundManager() {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (engineInitialized) {
                std::cerr << "Warning: SoundManager already initialized." << std::endl;
                return true;
            }

            ma_result result = ma_engine_init(nullptr, &engine); // Initialize with default config
            if (result != MA_SUCCESS) {
                std::cerr << "Error: Failed to initialize the audio engine: " << ma_result_description(result) << std::endl;
                return false;
            }

            engineInitialized = true;



            return true;
        }

        // Deinitialize the sound manager;
        static void deInitSoundManager() {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (!engineInitialized) {
                std::cerr << "Warning: SoundManager not initialized, cannot deinitialize." << std::endl;
                return;
            }

            for (auto& [name, sound] : sounds) {
                ma_sound_stop(&sound);
                ma_sound_uninit(&sound);
            }
            ma_engine_uninit(&engine);
            sounds.clear();
            engineInitialized = false;
        }

        // Load a sound from file
        static bool loadSound(const std::string& name, const std::string& filename) {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (!engineInitialized) {
                std::cerr << "Error: SoundManager not initialized. Call initSoundManager() first." << std::endl;
                return false;
            }

            ma_sound sound;
            if (ma_sound_init_from_file(&engine, filename.c_str(), 0, NULL, NULL, &sound) != MA_SUCCESS) {
                std::cerr << "Failed to load sound: " << filename << std::endl;
                return false;
            }
            sounds[name] = sound;
            return true;
        }

        // Play a sound
        static void playSound(const std::string& name) {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (!engineInitialized) {
                std::cerr << "Error: SoundManager not initialized. Call initSoundManager() first." << std::endl;
                return;
            }

            auto it = sounds.find(name);
            if (it != sounds.end()) {
                ma_sound_start(&it->second);
            }
            else {
                std::cerr << "Error: Sound not found: " << name << std::endl;
            }
        }

        // Set a sound to loop
        static void setLooping(const std::string& name, bool looping) {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (!engineInitialized) {
                std::cerr << "Error: SoundManager not initialized. Call initSoundManager() first." << std::endl;
                return;
            }

            auto it = sounds.find(name);
            if (it != sounds.end()) {
                ma_sound_set_looping(&it->second, looping);
            }
            else {
                std::cerr << "Error: Sound not found: " << name << std::endl;
            }
        }

        // Stop a sound
        static void stopSound(const std::string& name) {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (!engineInitialized) {
                std::cerr << "Error: SoundManager not initialized. Call initSoundManager() first." << std::endl;
                return;
            }

            auto it = sounds.find(name);
            if (it != sounds.end()) {
                ma_sound_stop(&it->second);
            }
            else {
                std::cerr << "Error: Sound not found: " << name << std::endl;
            }
        }

        // Stop all sounds
        static void stopAllSounds() {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (!engineInitialized) {
                std::cerr << "Error: SoundManager not initialized. Call initSoundManager() first." << std::endl;
                return;
            }
            ma_engine_stop(&engine);
        }

    private:

        static ma_engine engine;
        static std::unordered_map<std::string, ma_sound> sounds;
        static bool engineInitialized;
        static std::mutex engineMutex; // Mutex for thread safety;
};

// Initialize the static members
ma_engine SoundManager::engine;
std::unordered_map<std::string, ma_sound> SoundManager::sounds;
bool SoundManager::engineInitialized = false;
std::mutex SoundManager::engineMutex;


*/