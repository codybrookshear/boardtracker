#include "BoardDensitySystem.h"
#include <chrono>

BoardDensitySystem::BoardDensitySystem(const std::chrono::high_resolution_clock::time_point& startTime) 
    : startTime(startTime)
{

}

void BoardDensitySystem::MeasureDensityReady(int density, int time_uS) {
    insertSorted(densityList, {density, time_uS});
    clean(densityList);
}

void BoardDensitySystem::MeasurePositionReady(int position_mm, int time_uS) {
    insertSorted(positionList, {position_mm, time_uS});
    clean(positionList);
}

void BoardDensitySystem::CalculateDensityValues(int min_pos_mm, int max_pos_mm,
        int *mean_density, int *min_density, int *median_density) {
    
    int min_pos_time_uS;
    int max_pos_time_uS;
    int sum_density;
    int min_density_val;
    bool first = true;
    int count = 0;

    // first, find the time ranges that correspond to the given board positions.
    for (const auto& measurement : positionList) {
        if (min_pos_mm >= measurement.value) {
            min_pos_time_uS = measurement.time_uS;
            break;
        }
    }
    
    // reverse
    auto rit = positionList.rbegin();
    while (rit != positionList.rend() && rit->value > max_pos_mm) {
        ++rit;
    }
    max_pos_time_uS = rit->time_uS;

    // now loop through the densities and calc statistics for the requested time range
    for (const auto& density : densityList) {
        if (density.time_uS > min_pos_time_uS && density.time_uS < max_pos_time_uS) {
            // valid, process it's density metrics
            
            // -- minimum ---
            if (first) {
                min_density_val = density.value;
                first = false;
            }
            if (density.value < min_density_val) {
                min_density_val = density.value;
            }

            sum_density += density.value;
            count++;

        } else if (density.time_uS > max_pos_time_uS) {
            // went too far. done.
            break;
        }
    }

    // we need to consider that MeasureDensityReady or MeasurePositionReady could fire while we are in here
    // gathering stastics, and thus clean removing old measurements and messing up our lists and/or iterators we are using on them
    // i.e. we aren't using a thread-safe data structure here
    // return measurements in pointers.
    
    min_density = &min_density_val;
    
    auto mean_density_val = sum_density / count;
    mean_density = &mean_density_val;

    auto median_count = count / 2;
    if (count % 2 == 1) {
        median_count++; // odd number of samples
    }

    // TODO definition of median needs some work. today we do this:
    // get the middle density value simply based on how many samples there are
    // for the given distane measurements
    count = 0;
    for (const auto& density : densityList) {
        if (density.time_uS > min_pos_time_uS && density.time_uS < max_pos_time_uS) {
            // valid, process it's density metrics
            count++;

            if (median_count == count) {
                auto d = density.value;
                median_density = &d;
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

    // and remove any off the front of the list; that's where the oldest measurements are
    while (!measureList.empty() && measureList.front().time_uS < cutoffTime_uS) {
        measureList.pop_front();
    }

}

int BoardDensitySystem::elapsed_uS() {
    // Calculate how many microseconds have elapsed since startTime
    auto now = std::chrono::high_resolution_clock::now();
    int elapsed_uS = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count();

    return elapsed_uS;
}