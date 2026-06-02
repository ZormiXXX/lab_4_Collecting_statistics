#pragma once

inline void OnlineStatistics::RebalanceHeaps() {
    if (lowerHalf.GetSize() > upperHalf.GetSize() + 1) {
        upperHalf.Push(lowerHalf.Pop());
    } else if (upperHalf.GetSize() > lowerHalf.GetSize()) {
        lowerHalf.Push(upperHalf.Pop());
    }
}

inline void OnlineStatistics::PushMedian(double value) {
    if (lowerHalf.IsEmpty() || value <= lowerHalf.Peek()) {
        lowerHalf.Push(value);
    } else {
        upperHalf.Push(value);
    }
    RebalanceHeaps();
}

inline OnlineStatistics::OnlineStatistics(size_t rollingWindowSize, double anomalyZScoreThreshold)
    : count(0),
      mean(0.0),
      m2(0.0),
      minValue(0.0),
      maxValue(0.0),
      lowerHalf(),
      upperHalf(),
      window(rollingWindowSize > 0 ? new CircularBuffer<double>(rollingWindowSize) : nullptr),
      windowSize(rollingWindowSize),
      windowSum(0.0),
      anomalyThreshold(anomalyZScoreThreshold),
      lastAnomalyDetected(false),
      lastAnomalyZScore(0.0),
      totalAnomalies(0),
      lastValue(0.0) {}

inline OnlineStatistics::~OnlineStatistics() {
    delete window;
}

inline void OnlineStatistics::Reset() {
    count = 0;
    mean = 0.0;
    m2 = 0.0;
    minValue = 0.0;
    maxValue = 0.0;
    lowerHalf = BinaryHeap<double, std::greater<double>>();
    upperHalf = BinaryHeap<double, std::less<double>>();
    windowSum = 0.0;
    lastAnomalyDetected = false;
    lastAnomalyZScore = 0.0;
    totalAnomalies = 0;
    lastValue = 0.0;

    delete window;
    window = windowSize > 0 ? new CircularBuffer<double>(windowSize) : nullptr;
}

inline void OnlineStatistics::Add(double value) {
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

    if (window) {
        Option<double> evicted = window->AppendReturningEvicted(value);
        windowSum += value;
        if (evicted.IsSome()) {
            windowSum -= evicted.GetValue();
        }
    }
}

inline size_t OnlineStatistics::GetCount() const {
    return count;
}

inline double OnlineStatistics::GetMean() const {
    if (count == 0) {
        throw EmptyCollection();
    }
    return mean;
}

inline double OnlineStatistics::GetVariance() const {
    if (count < 2) {
        return 0.0;
    }
    return m2 / static_cast<double>(count - 1);
}

inline double OnlineStatistics::GetStandardDeviation() const {
    return std::sqrt(GetVariance());
}

inline double OnlineStatistics::GetMedian() const {
    if (count == 0) {
        throw EmptyCollection();
    }
    if (lowerHalf.GetSize() == upperHalf.GetSize()) {
        return (lowerHalf.Peek() + upperHalf.Peek()) / 2.0;
    }
    return lowerHalf.Peek();
}

inline double OnlineStatistics::GetWindowAverage() const {
    if (!window || window->IsEmpty()) {
        return 0.0;
    }
    return windowSum / static_cast<double>(window->GetLength());
}

inline StatisticsSnapshot OnlineStatistics::GetSnapshot() const {
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
