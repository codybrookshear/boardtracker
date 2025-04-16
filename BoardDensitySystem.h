#ifndef BOARDDENSITYSYSTEM_H
#define BOARDDENSITYSYSTEM_H

#include <chrono>
#include <deque>
#include <mutex>

struct Measurement {
    int value; // density (units?) or position (mm)
    std::chrono::microseconds time_uS;
};

class BoardDensitySystem {
public: 

    /// @brief Construct a new BoardDensitySystem object, using the given start time
    /// @param startTime 
    BoardDensitySystem(const std::chrono::high_resolution_clock::time_point& startTime);

    /// @brief callback for when a density measurement is ready, between 1500 Hz and 2000 Hz, apx 1800 Hz
    /// @param density "xray sensor measurement"
    /// @param time_uS time elapsed since program start, in microseconds
    void MeasureDensityReady(int density, int time_uS);

    /// @brief callback for position sensor, between 400 Hz and 600 Hz, apx 500 Hz
    /// @param position_mm position of the board when measured from the X-Ray sensor ?
    /// @param time_uS time elapsed since program start, in microseconds
    void MeasurePositionReady(int position_mm, int time_uS);

    /// @brief calculate metrics in the given length interval of the board. If there was an error, the
    ///        mean_density, and median_density values will be set to 0, and min_density will be set to
    ///        std::numeric_limits<int>::max()
    /// @param min_pos_mm minimum length of the board to get statistics for
    /// @param max_pos_mm maximum length of the board to get statistics for
    /// @param mean_density average density in the given range
    /// @param min_density minimum density in the given range
    /// @param median_density middle density in the given range
    void CalculateDensityValues(int min_pos_mm, int max_pos_mm,
        int *mean_density, int *min_density, int *median_density);

private:

    void insertSorted(std::deque<Measurement>& measurements, const Measurement& newMeasurement);
    void clean(std::deque<Measurement>& measurements);
    std::chrono::microseconds elapsed_uS() const;

    const std::chrono::high_resolution_clock::time_point& startTime; // time when the program started
    std::deque<Measurement> densities;
    std::deque<Measurement> positions;
    std::mutex data_mutex; // thread safety for density and position measurements
    int density_inserts_since_clean = 0; 
    int position_inserts_since_clean = 0; 
};

#endif // BOARDDENSITYSYSTEM_H