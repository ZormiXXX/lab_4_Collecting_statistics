#pragma once
#include <cmath>
#include "BinaryHeap.hpp"
#include "CircularBuffer.hpp"

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
    CircularBuffer<double>* window;
    size_t windowSize;
    double windowSum;
    double anomalyThreshold;
    bool lastAnomalyDetected;
    double lastAnomalyZScore;
    size_t totalAnomalies;
    double lastValue;

    void RebalanceHeaps();
    void PushMedian(double value);
    void UpdateAnomalyState(double value);
    void UpdateMinMax(double value);
    void UpdateMoments(double value);
    void UpdateWindow(double value);

public:
    explicit OnlineStatistics(size_t rollingWindowSize = 0, double anomalyZScoreThreshold = 3.0);

    OnlineStatistics(const OnlineStatistics&) = delete;
    OnlineStatistics& operator=(const OnlineStatistics&) = delete;

    ~OnlineStatistics();

    void Reset();
    void Add(double value);
    size_t GetCount() const;
    double GetMean() const;
    double GetVariance() const;
    double GetStandardDeviation() const;
    double GetMedian() const;
    double GetWindowAverage() const;
    StatisticsSnapshot GetSnapshot() const;
};

#include "OnlineStatistics.tpp"
