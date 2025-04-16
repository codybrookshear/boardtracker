#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include "BoardDensitySystem.h"

// Helper function to generate random intervals in microseconds
int getRandomInterval(int minHz, int maxHz) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(minHz, maxHz);
    
    // Convert Hz to microseconds
    int hz = dis(gen);
    return 1000000 / hz; // 1,000,000 microseconds per second
}

int main() {
    const std::chrono::high_resolution_clock::time_point& startTime = std::chrono::high_resolution_clock::now();
    BoardDensitySystem bds = BoardDensitySystem(startTime);

    // Simulate board movement (position increases over time)
    int currentPosition = 0;
    int maxPositionSeen = 0;
    const int BOARD_LENGTH = 10000; // 10 meters in mm
    const int BOARD_SPEED = 1000;   // 1 meter per second in mm/s

    // Run for 10 seconds
    auto endTime = startTime + std::chrono::seconds(10);
    auto nextCalculationTime = startTime + std::chrono::seconds(1);
    
    while (std::chrono::high_resolution_clock::now() < endTime) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();
        
        // Simulate density measurement (1500-2000 Hz)
        static auto nextDensityTime = currentTime;
        if (currentTime >= nextDensityTime) {
            // Generate random density value between 0 and 1000
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, 1000);
            int density = dis(gen);
            
            bds.MeasureDensityReady(density, elapsedTime);
            nextDensityTime = currentTime + std::chrono::microseconds(getRandomInterval(1500, 2000));
        }

        // Simulate position measurement (400-600 Hz)
        static auto nextPositionTime = currentTime;
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
            nextPositionTime = currentTime + std::chrono::microseconds(getRandomInterval(400, 600));
        }

        // Calculate density values every second
        if (currentTime >= nextCalculationTime) {
            int mean_density, min_density, median_density;
            
            // Calculate for the last 100mm of the board
            int min_pos = (maxPositionSeen > 100) ? maxPositionSeen - 100 : 0;
            bds.CalculateDensityValues(min_pos, maxPositionSeen, 
                                     &mean_density, &min_density, &median_density);
            
            std::cout << "Time: " << elapsedTime / 1000000.0 << "s" << std::endl;
            std::cout << "  Position range: " << min_pos << "mm to " << maxPositionSeen << "mm" << std::endl;
            std::cout << "  Mean density: " << mean_density << std::endl;
            std::cout << "  Min density: " << min_density << std::endl;
            std::cout << "  Median density: " << median_density << std::endl;
            std::cout << std::endl;
            
            nextCalculationTime = currentTime + std::chrono::seconds(1);
        }

        // Small sleep to prevent CPU overuse
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    return 0;
} 