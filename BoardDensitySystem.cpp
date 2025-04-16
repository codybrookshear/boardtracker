#include "BoardDensitySystem.h"
#include <chrono>
#include <iostream>
#include <limits>
#include <mutex>
#include <deque>
#include <vector>

constexpr std::chrono::microseconds FIVE_SECONDS_US(5'000'000);

BoardDensitySystem::BoardDensitySystem(const std::chrono::high_resolution_clock::time_point& startTime) 
    : startTime(startTime)
{

}

void BoardDensitySystem::MeasureDensityReady(int density, int time_uS) {
    std::lock_guard<std::mutex> lock(data_mutex);
    insertSorted(densities, {density, std::chrono::microseconds(time_uS)});
    density_inserts_since_clean++;
    if (density_inserts_since_clean >= 100) { // Clean every 100 inserts
        clean(densities);
        density_inserts_since_clean = 0;
    }
}

void BoardDensitySystem::MeasurePositionReady(int position_mm, int time_uS) {
    std::lock_guard<std::mutex> lock(data_mutex);
    insertSorted(positions, {position_mm, std::chrono::microseconds(time_uS)});
    position_inserts_since_clean++;
    if (position_inserts_since_clean >= 100) { // Clean every 100 inserts
        clean(positions);
        position_inserts_since_clean = 0;
    }
}

void BoardDensitySystem::CalculateDensityValues(int min_pos_mm, int max_pos_mm,
        int *mean_density, int *min_density, int *median_density) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    std::chrono::microseconds min_pos_time_uS;
    std::chrono::microseconds max_pos_time_uS;
    int sum_density = 0;
    int count = 0;
    std::vector<int> tmp_density_values;

    *mean_density = 0;
    *min_density = std::numeric_limits<int>::max();
    *median_density = 0;

    if (positions.empty() || densities.empty()) {
        // no measurements yet
        return;
    }

    // verify the requested board position range is valid
    if (min_pos_mm < 0 || max_pos_mm < 0 || min_pos_mm > max_pos_mm ||
        min_pos_mm < positions.front().value || max_pos_mm > positions.back().value) {
        return;
    }

    // find the time ranges that correspond to the given board positions.
    // forward to find the min time
    for (const auto& measurement : positions) {
        if (measurement.value >= min_pos_mm) {
            min_pos_time_uS = measurement.time_uS;
            break;
        }
    }
    
    // reverse to find the max time
    auto rit = positions.rbegin();
    while (rit != positions.rend() && rit->value > max_pos_mm) {
        ++rit;
    }
    max_pos_time_uS = rit->time_uS;

    // now loop through the densities and calc statistics for the requested time range
    for (const auto& density : densities) {
        if (density.time_uS > min_pos_time_uS && density.time_uS < max_pos_time_uS) {
            // valid, process it's density metrics
            
            if (density.value < *min_density) {
                *min_density = density.value;
            }

            // add to the list of density values for median calc
            tmp_density_values.push_back(density.value);


            sum_density += density.value;
            count++;

        } else if (density.time_uS > max_pos_time_uS) {
            // went too far. done.
            break;
        }
    }

    if (count == 0) {
        // should never happen, but just in case!
        // no valid density measurements in the requested range
        return;
    }

    // calculate the mean density
    *mean_density = sum_density / count;

    // sort the density values
    std::sort(tmp_density_values.begin(), tmp_density_values.end());
    // find the median density
    if (count % 2 == 0) {
        // even number of elements
        *median_density = (tmp_density_values[count / 2 - 1] + tmp_density_values[count / 2]) / 2;
    } else {
        // odd number of elements
        *median_density = tmp_density_values[count / 2];
    }

}

void BoardDensitySystem::insertSorted(std::deque<Measurement>& measurements, const Measurement& newMeasurement) {
        // 99% case - time comes after all others
        if (measurements.empty() || newMeasurement.time_uS >= measurements.back().time_uS) {
            measurements.push_back(newMeasurement);
            return;
        }
    
        // 1% case - measurement time comes out of order
        auto rit = measurements.rbegin();
        while (rit != measurements.rend() && rit->time_uS > newMeasurement.time_uS) {
            ++rit;
        }
        measurements.insert(rit.base(), newMeasurement);
}

void BoardDensitySystem::clean(std::deque<Measurement>& measurements) {

    // find the time value that was 5 seconds ago, in microseconds
    const std::chrono::microseconds cutoffTime_uS = elapsed_uS() - FIVE_SECONDS_US;

    // and remove any off the front of the list; that's where the oldest measurements are
    while (!measurements.empty() && measurements.front().time_uS < cutoffTime_uS) {
        measurements.pop_front();
    }
}

std::chrono::microseconds BoardDensitySystem::elapsed_uS() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
}