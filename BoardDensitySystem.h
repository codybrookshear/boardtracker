#ifndef BOARDDENSITYSYSTEM_H
#define BOARDDENSITYSYSTEM_H

#include <chrono>
#include <list>
#include <mutex>

struct Measurement {
    int value; // density (unit?) or position (mm)
    int time_uS;
};

class BoardDensitySystem {
public: 

    BoardDensitySystem(const std::chrono::high_resolution_clock::time_point& startTime);

    /// @brief callback for when a density measurement is ready, between 1500 Hz and 2000 Hz, apx 1800 Hz
    /// @param density "xray sensor measurement"
    /// @param time_uS time elapsed since program start, in microseconds
    void MeasureDensityReady(int density, int time_uS);

    /// @brief callback for position sensor, between 400 Hz and 600 Hz, apx 500 Hz
    /// @param position_mm position of the board when measured from the X-Ray sensor ?
    /// @param time_uS time elapsed since program start, in microseconds
    void MeasurePositionReady(int position_mm, int time_uS);

    /// @brief calculate metrics in the given length interval of the board
    /// @param min_pos_mm minimum length of the board to get statistics for
    /// @param max_pos_mm maximum length of the board to get statistics for
    /// @param mean_density 
    /// @param min_density 
    /// @param median_density 
    void CalculateDensityValues(int min_pos_mm, int max_pos_mm,
        int *mean_density, int *min_density, int *median_density);

    int elapsed_uS();

private:

    void insertSorted(std::list<Measurement>& measureList, const Measurement& newMeasurement);
    void clean(std::list<Measurement>& measureList);
    const std::chrono::high_resolution_clock::time_point& startTime;
    std::list<Measurement> densityList;
    std::list<Measurement> positionList;
    std::mutex data_mutex;
};

#endif // BOARDDENSITYSYSTEM_H