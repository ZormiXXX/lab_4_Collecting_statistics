#pragma once
#include <cmath>
#include <deque>
#include <iomanip>
#include <sstream>
#include "BinaryHeap.hpp"

struct StatisticsSnapshot {
    size_t count;
    double lastValue;
    double minValue;
    double maxValue;
    double mean;
    double variance;
    double standardDeviation;
    double median;
    double windowAverage;
    bool anomalyDetected;
    double anomalyZScore;
    size_t anomalyCount;
};

class OnlineStatistics {
private:
    size_t count;
    double mean;
    double m2;
    double minValue;
    double maxValue;
    BinaryHeap<double, std::greater<double>> lowerHalf;
    BinaryHeap<double, std::less<double>> upperHalf;
    std::deque<double> window;
    size_t windowSize;
    double windowSum;
    double anomalyThreshold;
    bool lastAnomalyDetected;
    double lastAnomalyZScore;
    size_t totalAnomalies;
    double lastValue;

    void RebalanceHeaps() {
        if (lowerHalf.GetSize() > upperHalf.GetSize() + 1) {
            upperHalf.Push(lowerHalf.Pop());
        } else if (upperHalf.GetSize() > lowerHalf.GetSize()) {
            lowerHalf.Push(upperHalf.Pop());
        }
    }

    void PushMedian(double value) {
        if (lowerHalf.IsEmpty() || value <= lowerHalf.Peek()) {
            lowerHalf.Push(value);
        } else {
            upperHalf.Push(value);
        }
        RebalanceHeaps();
    }

public:
    explicit OnlineStatistics(size_t rollingWindowSize = 0, double anomalyZScoreThreshold = 3.0)
        : count(0),
          mean(0.0),
          m2(0.0),
          minValue(0.0),
          maxValue(0.0),
          lowerHalf(),
          upperHalf(),
          window(),
          windowSize(rollingWindowSize),
          windowSum(0.0),
          anomalyThreshold(anomalyZScoreThreshold),
          lastAnomalyDetected(false),
          lastAnomalyZScore(0.0),
          totalAnomalies(0),
          lastValue(0.0) {}

    void Reset() {
        *this = OnlineStatistics(windowSize, anomalyThreshold);
    }

    void Add(double value) {
        lastValue = value;

        if (count >= 2) {
            double varianceBefore = m2 / static_cast<double>(count - 1);
            double deviationBefore = std::sqrt(varianceBefore);
            if (deviationBefore > 1e-12) {
                lastAnomalyZScore = std::fabs(value - mean) / deviationBefore;
                lastAnomalyDetected = lastAnomalyZScore >= anomalyThreshold;
                if (lastAnomalyDetected) {
                    totalAnomalies++;
                }
            } else {
                lastAnomalyDetected = false;
                lastAnomalyZScore = 0.0;
            }
        } else {
            lastAnomalyDetected = false;
            lastAnomalyZScore = 0.0;
        }

        if (count == 0) {
            minValue = value;
            maxValue = value;
        } else {
            if (value < minValue) {
                minValue = value;
            }
            if (value > maxValue) {
                maxValue = value;
            }
        }

        count++;
        double delta = value - mean;
        mean += delta / static_cast<double>(count);
        double delta2 = value - mean;
        m2 += delta * delta2;

        PushMedian(value);

        if (windowSize > 0) {
            window.push_back(value);
            windowSum += value;
            if (window.size() > windowSize) {
                windowSum -= window.front();
                window.pop_front();
            }
        }
    }

    size_t GetCount() const {
        return count;
    }

    double GetMean() const {
        if (count == 0) {
            throw EmptyCollection();
        }
        return mean;
    }

    double GetVariance() const {
        if (count < 2) {
            return 0.0;
        }
        return m2 / static_cast<double>(count - 1);
    }

    double GetStandardDeviation() const {
        return std::sqrt(GetVariance());
    }

    double GetMedian() const {
        if (count == 0) {
            throw EmptyCollection();
        }
        if (lowerHalf.GetSize() == upperHalf.GetSize()) {
            return (lowerHalf.Peek() + upperHalf.Peek()) / 2.0;
        }
        return lowerHalf.Peek();
    }

    double GetWindowAverage() const {
        if (window.empty()) {
            return 0.0;
        }
        return windowSum / static_cast<double>(window.size());
    }

    StatisticsSnapshot GetSnapshot() const {
        if (count == 0) {
            throw EmptyCollection();
        }

        return {
            count,
            lastValue,
            minValue,
            maxValue,
            mean,
            GetVariance(),
            GetStandardDeviation(),
            GetMedian(),
            GetWindowAverage(),
            lastAnomalyDetected,
            lastAnomalyZScore,
            totalAnomalies
        };
    }
};

inline std::string FormatSnapshotRow(size_t index, double value, const StatisticsSnapshot& snapshot) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(3)
           << std::setw(6) << index
           << std::setw(12) << value
           << std::setw(12) << snapshot.mean
           << std::setw(12) << snapshot.median
           << std::setw(12) << snapshot.minValue
           << std::setw(12) << snapshot.maxValue
           << std::setw(12) << snapshot.windowAverage
           << std::setw(10) << (snapshot.anomalyDetected ? "YES" : "no");
    return stream.str();
}
