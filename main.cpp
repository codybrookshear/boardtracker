#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <locale>
#include <sstream>
#include "BoardDensitySystem.h"

// Helper function to generate random intervals in microseconds for a given Hz range
int getRandomIntervalFromHz(int minHz, int maxHz) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // Convert Hz to microseconds (lower Hz = longer interval, higher Hz = shorter interval)
    int minUs = 1000000 / maxHz;
    int maxUs = 1000000 / minHz;
    std::uniform_int_distribution<> dis(minUs, maxUs);
    return dis(gen);
}

int getRandomDensity() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100); // Random density between 0 and 100
    return dis(gen);
}
std::string formatWithCommas(int64_t value) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << value;
    return ss.str();
}

int main() {
    const std::chrono::high_resolution_clock::time_point& startTime = std::chrono::high_resolution_clock::now();
    BoardDensitySystem bds(startTime);

    // Simulate board movement (position increases over time)
    int currentPosition = 0;
    int maxPositionSeen = 0;

    const int BOARD_LENGTH = 5000; // 5 meters in mm
    const int BOARD_SPEED = 500;   // 1/2 meter per second in mm/s
    const int DENSITY_LOW_HZ = 1500; // 1500 Hz
    const int DENSITY_HIGH_HZ = 2000; // 2000 Hz
    const int POSITION_LOW_HZ = 400; // 400 Hz
    const int POSITION_HIGH_HZ = 600; // 600 Hz

    // Run for 10 seconds
    auto endTime = startTime + std::chrono::seconds(10);

    auto nextDensityTime = startTime;
    auto nextPositionTime = startTime;
    
    while (std::chrono::high_resolution_clock::now() < endTime) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();
        
        // Simulate density measurement (1500-2000 Hz)
        if (currentTime >= nextDensityTime) {
            
            bds.MeasureDensityReady(getRandomDensity(), elapsedTime);
            nextDensityTime = currentTime + std::chrono::microseconds(getRandomIntervalFromHz(DENSITY_LOW_HZ, DENSITY_HIGH_HZ));
        
            //std::cout << "elapsed " << formatWithCommas(elapsedTime) << ", density " << density << std::endl;
        }

        // Simulate position measurement (400-600 Hz)
        if (currentTime >= nextPositionTime) {
            // Update position based on time elapsed
            currentPosition = (elapsedTime * BOARD_SPEED) / 1000000; // Convert microseconds to seconds
            if (currentPosition > BOARD_LENGTH) {
                currentPosition = 0; // Reset when board end is reached
            }
            
            // Update max position seen
            if (currentPosition > maxPositionSeen) {
                maxPositionSeen = currentPosition;
            }
            
            bds.MeasurePositionReady(currentPosition, elapsedTime);
            nextPositionTime = currentTime + std::chrono::microseconds(getRandomIntervalFromHz(400, 600));
            //std::cout << "elapsed " << formatWithCommas(elapsedTime) << ", pos  " << currentPosition << " mm" << std::endl;
        }

        // Small sleep to prevent CPU overuse
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }



    int mean_density, min_density, median_density;
    
    // Calculate for the last 100mm of the board
    int min_pos = (maxPositionSeen > 100) ? maxPositionSeen - 100 : 0;
    bds.CalculateDensityValues(min_pos, maxPositionSeen, 
                                &mean_density, &min_density, &median_density);
    
    std::cout << "  Position range: " << min_pos << "mm to " << maxPositionSeen << "mm" << std::endl;
    std::cout << "  Mean density: " << mean_density << std::endl;
    std::cout << "  Min density: " << min_density << std::endl;
    std::cout << "  Median density: " << median_density << std::endl;
    std::cout << std::endl;


    return 0;
}