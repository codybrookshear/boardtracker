#include "BoardDensitySystem.h"
#include <chrono>
#include <iostream>

BoardDensitySystem::BoardDensitySystem(const std::chrono::high_resolution_clock::time_point& startTime) 
    : startTime(startTime)
{

}

void BoardDensitySystem::MeasureDensityReady(int density, int time_uS) {
    std::lock_guard<std::mutex> lock(data_mutex);
    insertSorted(densityList, {density, time_uS});
    clean(densityList);
}

void BoardDensitySystem::MeasurePositionReady(int position_mm, int time_uS) {
    std::lock_guard<std::mutex> lock(data_mutex);
    insertSorted(positionList, {position_mm, time_uS});
    clean(positionList);
}

void BoardDensitySystem::CalculateDensityValues(int min_pos_mm, int max_pos_mm,
        int *mean_density, int *min_density, int *median_density) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    int min_pos_time_uS;
    int max_pos_time_uS;
    int sum_density = 0;
    int count = 0;

    *mean_density = 0;
    *min_density = INT_MAX;
    *median_density = 0;

    // verify the requested board position range is valid
    if (min_pos_mm < 0 || max_pos_mm < 0 || min_pos_mm > max_pos_mm ||
        min_pos_mm < positionList.front().value || max_pos_mm > positionList.back().value) {
        return;
    }

    // find the time ranges that correspond to the given board positions.
    // forward to find the min time
    for (const auto& measurement : positionList) {
        if (measurement.value >= min_pos_mm) {
            min_pos_time_uS = measurement.time_uS;
            break;
        }
    }
    
    // reverse to find the max time
    auto rit = positionList.rbegin();
    while (rit != positionList.rend() && rit->value > max_pos_mm) {
        ++rit;
    }
    max_pos_time_uS = rit->time_uS;

    // now loop through the densities and calc statistics for the requested time range
    for (const auto& density : densityList) {
        if (density.time_uS > min_pos_time_uS && density.time_uS < max_pos_time_uS) {
            // valid, process it's density metrics
            
            if (density.value < *min_density) {
                *min_density = density.value;
            }

            sum_density += density.value;
            count++;

        } else if (density.time_uS > max_pos_time_uS) {
            // went too far. done.
            break;
        }
    }
    
    *mean_density = sum_density / count;

    auto median_count = count / 2;
    if (count % 2 == 1) {
        median_count++; // odd number of samples
    }

    // TODO definition & calculation of median may need some work. today we do this:
    // get the middle density value simply based on how many samples there are
    // for the given distane measurements
    count = 0;
    for (const auto& density : densityList) {
        if (density.time_uS > min_pos_time_uS && density.time_uS < max_pos_time_uS) {
            // valid, process it's density metrics
            count++;

            if (median_count == count) {
                *median_density = density.value;
                return;
            }
        }
    }

}

void BoardDensitySystem::insertSorted(std::list<Measurement>& measureList, const Measurement& newMeasurement) {
        // 99% case - time comes after all others
        if (measureList.empty() || measureList.back().time_uS <= newMeasurement.time_uS) {
            measureList.push_back(newMeasurement);
            return;
        }
    
        // 1% case - measurement time comes out of order
        auto rit = measureList.rbegin();
        while (rit != measureList.rend() && rit->time_uS > newMeasurement.time_uS) {
            ++rit;
        }
        measureList.insert(rit.base(), newMeasurement);
}

void BoardDensitySystem::clean(std::list<Measurement>& measureList) {

    // find the time value that was 5 seconds ago, in microseconds
    auto cutoffTime_uS = elapsed_uS() - 5'000'000;
    int removed = 0;

    // and remove any off the front of the list; that's where the oldest measurements are
    while (!measureList.empty() && measureList.front().time_uS < cutoffTime_uS) {
        measureList.pop_front();
    }

    //if (removed > 0) {
    //    std::cout << "Cleaned " << removed << " measurements from the list." << std::endl;
    //}
}

int BoardDensitySystem::elapsed_uS() {
    // Calculate how many microseconds have elapsed since startTime
    auto now = std::chrono::high_resolution_clock::now();
    int elapsed_uS = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count();

    return elapsed_uS;
}