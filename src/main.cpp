#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include "../include/ArraySequence.hpp"
#include "../include/LazySequence.hpp"
#include "../include/OnlineStatistics.hpp"
#include "../include/Streams.hpp"

extern int RunAllTests();

namespace {

using DoubleLazySequence = LazySequence<double>;

constexpr const char* kMainMenu = R"(
╔════════════════════════════════════════════════════════════════════╗
║         ЛАБОРАТОРНАЯ РАБОТА №4 - LAZY SEQUENCE И STREAMS         ║
╠════════════════════════════════════════════════════════════════════╣
║  1. Демонстрация LazySequence                                    ║
║  2. Демонстрация потоков чтения/записи                           ║
║  3. Онлайн-статистика потока событий                             ║
║  4. Запустить модульные тесты                                    ║
║  0. Выход                                                        ║
╚════════════════════════════════════════════════════════════════════╝
)";

void EnsureOutputDirectories() {
    std::filesystem::create_directories("output");
    std::filesystem::create_directories("data");
}

double ParseDoubleStrict(const std::string& token) {
    size_t consumed = 0;
    try {
        double value = std::stod(token, &consumed);
        if (consumed != token.size()) {
            throw InputError("не удалось полностью распознать число: " + token);
        }
        return value;
    } catch (const std::invalid_argument&) {
        throw InputError("ожидалось число, получено: " + token);
    } catch (const std::out_of_range&) {
        throw InputError("число вне допустимого диапазона: " + token);
    }
}

std::string SerializeDouble(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6) << value;
    return stream.str();
}

template<class T>
T ReadNumber(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        T value{};
        if (std::cin >> value) {
            return value;
        }

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Некорректный ввод. Повторите попытку.\n";
    }
}

std::string ReadLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin >> std::ws, line);
    return line;
}

ArraySequence<double> ParseNumbersLine(const std::string& line) {
    ArraySequence<double> values;
    std::istringstream stream(NormalizeTokenSeparators(line));
    std::string token;
    while (stream >> token) {
        values.Append(ParseDoubleStrict(token));
    }
    return values;
}

DoubleLazySequence MakeManualFiniteSequence() {
    ArraySequence<double> values = ParseNumbersLine(
        ReadLine("Введите числа через пробел/запятую: ")
    );
    return DoubleLazySequence(&values);
}

DoubleLazySequence MakeArithmeticSequence() {
    double start = ReadNumber<double>("Начальное значение: ");
    double step = ReadNumber<double>("Шаг: ");
    double seedValue[1] = {start};
    ArraySequence<double> seed(seedValue, 1);

    return DoubleLazySequence(
        [step](size_t, const Sequence<double>* materialized) -> double {
            return materialized->Get(materialized->GetLength() - 1) + step;
        },
        &seed
    );
}

DoubleLazySequence MakeFibonacciSequence() {
    double first = ReadNumber<double>("Первый элемент: ");
    double second = ReadNumber<double>("Второй элемент: ");
    double seeds[2] = {first, second};
    ArraySequence<double> seed(seeds, 2);

    return DoubleLazySequence(
        [](size_t, const Sequence<double>* materialized) -> double {
            int length = materialized->GetLength();
            return materialized->Get(length - 1) + materialized->Get(length - 2);
        },
        &seed
    );
}

DoubleLazySequence MakeRandomWalkSequence() {
    double start = ReadNumber<double>("Старт random walk: ");
    double maxStep = ReadNumber<double>("Максимальный модуль шага: ");
    unsigned int seed = ReadNumber<unsigned int>("Seed генератора: ");
    auto rng = std::make_shared<std::mt19937>(seed);
    auto last = std::make_shared<double>(start);

    return DoubleLazySequence::GenerateIndexed(
        [rng, last, start, distribution = std::uniform_real_distribution<double>(-maxStep, maxStep)]
        (size_t index) mutable -> double {
            if (index == 0) {
                return start;
            }
            *last += distribution(*rng);
            return *last;
        }
    );
}

DoubleLazySequence MakeSensorSequence() {
    double baseline = ReadNumber<double>("Базовый уровень сигнала: ");
    double amplitude = ReadNumber<double>("Амплитуда колебаний: ");
    double anomalyJump = ReadNumber<double>("Размер выброса: ");
    int anomalyPeriod = ReadNumber<int>("Период выбросов (например 20): ");

    return DoubleLazySequence::GenerateIndexed(
        [baseline, amplitude, anomalyJump, anomalyPeriod](size_t index) -> double {
            double wave = baseline + amplitude * std::sin(static_cast<double>(index) * 0.25);
            double drift = static_cast<double>(index % 7) * 0.15;
            if (anomalyPeriod > 0 && index > 0 && index % static_cast<size_t>(anomalyPeriod) == 0) {
                return wave + drift + anomalyJump;
            }
            return wave + drift;
        }
    );
}

DoubleLazySequence CreateLazySequenceFromUser() {
    std::cout << "\nИсточник LazySequence:\n"
              << "  1. Конечная последовательность, введённая вручную\n"
              << "  2. Арифметическая прогрессия\n"
              << "  3. Последовательность Фибоначчи\n"
              << "  4. Random walk\n"
              << "  5. Псевдо-датчик с выбросами\n";

    int choice = ReadNumber<int>("Выбор: ");
    switch (choice) {
        case 1: return MakeManualFiniteSequence();
        case 2: return MakeArithmeticSequence();
        case 3: return MakeFibonacciSequence();
        case 4: return MakeRandomWalkSequence();
        case 5: return MakeSensorSequence();
        default: throw InputError("неизвестный сценарий LazySequence");
    }
}

void PrintPrefix(const DoubleLazySequence& sequence, size_t count) {
    std::cout << "Первые " << count << " элементов: [";
    for (size_t index = 0; index < count; index++) {
        if (index > 0) {
            std::cout << ", ";
        }
        std::cout << std::fixed << std::setprecision(3)
                  << sequence.Get(static_cast<int>(index));
    }
    std::cout << "]\n";
}

void PrintLengthInfo(const DoubleLazySequence& sequence) {
    std::cout << "Длина: " << sequence.GetLength() << "\n";
    std::cout << "Материализовано элементов: " << sequence.GetMaterializedCount() << "\n";
}

void PrintTuplePrefix(const LazySequence<Tuple<double, double>>& zipped, size_t count) {
    std::cout << "Первые " << count << " пар: [";
    for (size_t index = 0; index < count; index++) {
        if (index > 0) {
            std::cout << ", ";
        }
        Tuple<double, double> item = zipped.Get(static_cast<int>(index));
        std::cout << "(" << std::fixed << std::setprecision(3)
                  << item.first << ", " << item.second << ")";
    }
    std::cout << "]\n";
}

void RunLazySequenceDemo() {
    DoubleLazySequence current = CreateLazySequenceFromUser();

    while (true) {
        std::cout << "\nLazySequence меню:\n"
                  << "  1. Показать префикс\n"
                  << "  2. Получить элемент по индексу\n"
                  << "  3. Показать длину и число материализованных элементов\n"
                  << "  4. Prepend\n"
                  << "  5. InsertAt\n"
                  << "  6. Concat с коротким хвостом\n"
                  << "  7. Map (x * factor)\n"
                  << "  8. Where (x >= threshold)\n"
                  << "  9. Zip с конечной последовательностью\n"
                  << "  10. Взять подпоследовательность\n"
                  << "  0. Назад\n";

        int choice = ReadNumber<int>("Выбор: ");
        if (choice == 0) {
            return;
        }

        switch (choice) {
            case 1: {
                size_t count = ReadNumber<size_t>("Сколько элементов вывести: ");
                PrintPrefix(current, count);
                PrintLengthInfo(current);
                break;
            }
            case 2: {
                int index = ReadNumber<int>("Индекс: ");
                std::cout << "Значение: " << std::fixed << std::setprecision(6)
                          << current.Get(index) << "\n";
                PrintLengthInfo(current);
                break;
            }
            case 3: {
                PrintLengthInfo(current);
                break;
            }
            case 4: {
                double value = ReadNumber<double>("Значение для вставки в начало: ");
                current = current.Prepend(value);
                std::cout << "Операция выполнена.\n";
                break;
            }
            case 5: {
                int index = ReadNumber<int>("Позиция вставки: ");
                double value = ReadNumber<double>("Значение: ");
                current = current.InsertAt(value, index);
                std::cout << "Операция выполнена.\n";
                break;
            }
            case 6: {
                ArraySequence<double> tail = ParseNumbersLine(
                    ReadLine("Введите хвост через пробел/запятую: ")
                );
                DoubleLazySequence suffix(&tail);
                current = current.Concat(suffix);
                std::cout << "Сцепление выполнено.\n";
                break;
            }
            case 7: {
                double factor = ReadNumber<double>("Множитель: ");
                current = current.Map<double>([factor](double value) {
                    return value * factor;
                });
                std::cout << "Map выполнен.\n";
                break;
            }
            case 8: {
                double threshold = ReadNumber<double>("Порог threshold: ");
                current = current.Where([threshold](double value) {
                    return value >= threshold;
                });
                std::cout << "Where выполнен.\n";
                break;
            }
            case 9: {
                ArraySequence<double> other = ParseNumbersLine(
                    ReadLine("Введите вторую последовательность через пробел/запятую: ")
                );
                LazySequence<Tuple<double, double>> zipped = current.Zip(other);
                size_t count = ReadNumber<size_t>("Сколько пар показать: ");
                PrintTuplePrefix(zipped, count);
                break;
            }
            case 10: {
                int left = ReadNumber<int>("startIndex: ");
                int right = ReadNumber<int>("endIndex: ");
                current = current.GetSubsequence(left, right);
                std::cout << "Подпоследовательность сохранена как текущая.\n";
                break;
            }
            default:
                std::cout << "Неизвестная команда.\n";
                break;
        }
    }
}

std::unique_ptr<ReadOnlyStream<double>> CreateReadStreamFromUser() {
    std::cout << "\nИсточник потока:\n"
              << "  1. Строка чисел\n"
              << "  2. LazySequence\n"
              << "  3. Файл\n";
    int choice = ReadNumber<int>("Выбор: ");

    switch (choice) {
        case 1: {
            std::string line = ReadLine("Введите числа через пробел/запятую: ");
            return std::make_unique<StringReadStream<double>>(line, ParseDoubleStrict);
        }
        case 2: {
            DoubleLazySequence sequence = CreateLazySequenceFromUser();
            return std::make_unique<LazySequenceReadStream<double>>(sequence);
        }
        case 3: {
            std::string path = ReadLine("Путь к файлу (Enter для data/sample_stream.txt): ");
            if (path.empty()) {
                path = "data/sample_stream.txt";
            }
            return std::make_unique<FileReadStream<double>>(path, ParseDoubleStrict);
        }
        default:
            throw InputError("неизвестный источник потока");
    }
}

void RunStreamDemo() {
    EnsureOutputDirectories();
    std::unique_ptr<ReadOnlyStream<double>> reader = CreateReadStreamFromUser();
    reader->Open();

    size_t previewCount = ReadNumber<size_t>("Сколько элементов прочитать для предварительного просмотра: ");
    std::cout << "Прочитанные элементы: [";
    for (size_t index = 0; index < previewCount; index++) {
        if (reader->IsEndOfStream()) {
            break;
        }
        if (index > 0) {
            std::cout << ", ";
        }
        std::cout << std::fixed << std::setprecision(3) << reader->Read();
    }
    std::cout << "]\n";
    std::cout << "Текущая позиция: " << reader->GetPosition() << "\n";

    if (reader->IsCanSeek()) {
        size_t seekIndex = ReadNumber<size_t>("Перейти на позицию (seek): ");
        reader->Seek(seekIndex);
        std::cout << "Позиция после seek: " << reader->GetPosition() << "\n";
        if (!reader->IsEndOfStream()) {
            std::cout << "Следующий элемент после seek: " << reader->Read() << "\n";
        }
    }

    if (reader->IsCanSeek()) {
        reader->Seek(0);
    }

    size_t copyCount = ReadNumber<size_t>("Сколько элементов скопировать в память и файл: ");
    MemoryWriteStream<double> memoryWriter;
    FileWriteStream<double> fileWriter("output/stream_dump.txt", SerializeDouble);
    memoryWriter.Open();
    fileWriter.Open();

    size_t copied = Pump(*reader, memoryWriter, copyCount);
    reader->Close();

    for (int index = 0; index < memoryWriter.GetItems().GetLength(); index++) {
        fileWriter.Write(memoryWriter.GetItems().Get(index));
    }
    fileWriter.Close();

    std::cout << "Скопировано элементов: " << copied << "\n";
    std::cout << "Дамп записан в output/stream_dump.txt\n";
}

std::string ToCsvRow(size_t index, double value, const StatisticsSnapshot& snapshot) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6)
           << index << ","
           << value << ","
           << snapshot.mean << ","
           << snapshot.median << ","
           << snapshot.minValue << ","
           << snapshot.maxValue << ","
           << snapshot.variance << ","
           << snapshot.standardDeviation << ","
           << snapshot.windowAverage << ","
           << (snapshot.anomalyDetected ? 1 : 0) << ","
           << snapshot.anomalyZScore << ","
           << snapshot.anomalyCount;
    return stream.str();
}

void PrintStatisticsHeader() {
    std::cout << std::setw(6) << "idx"
              << std::setw(12) << "value"
              << std::setw(12) << "mean"
              << std::setw(12) << "median"
              << std::setw(12) << "min"
              << std::setw(12) << "max"
              << std::setw(12) << "window"
              << std::setw(10) << "anomaly"
              << "\n";
}

void RunOnlineStatisticsDemo() {
    EnsureOutputDirectories();
    std::unique_ptr<ReadOnlyStream<double>> reader = CreateReadStreamFromUser();
    size_t maxItems = ReadNumber<size_t>("Сколько элементов обработать: ");
    size_t windowSize = ReadNumber<size_t>("Размер скользящего окна (0 - выключить): ");
    double anomalyThreshold = ReadNumber<double>("Порог z-score для выбросов: ");
    size_t printEvery = ReadNumber<size_t>("Печатать каждую k-ю строку (1 - все): ");

    OnlineStatistics stats(windowSize, anomalyThreshold);
    FileWriteStream<std::string> csvWriter(
        "output/latest_stats.csv",
        [](const std::string& line) { return line; }
    );

    reader->Open();
    csvWriter.Open();
    csvWriter.Write("index,value,mean,median,min,max,variance,stddev,window_average,anomaly,zscore,anomaly_count");

    size_t processed = 0;
    PrintStatisticsHeader();
    while (processed < maxItems && !reader->IsEndOfStream()) {
        double value = reader->Read();
        stats.Add(value);
        StatisticsSnapshot snapshot = stats.GetSnapshot();
        processed++;

        csvWriter.Write(ToCsvRow(processed, value, snapshot));
        if (printEvery == 0 || processed == 1 || processed % printEvery == 0 || processed == maxItems) {
            std::cout << FormatSnapshotRow(processed, value, snapshot) << "\n";
        }
    }

    reader->Close();
    csvWriter.Close();

    if (processed == 0) {
        std::cout << "Поток оказался пустым.\n";
        return;
    }

    StatisticsSnapshot summary = stats.GetSnapshot();
    std::cout << "\nИтоговая сводка:\n";
    std::cout << "  Обработано элементов: " << summary.count << "\n";
    std::cout << "  Mean: " << std::fixed << std::setprecision(6) << summary.mean << "\n";
    std::cout << "  Median: " << summary.median << "\n";
    std::cout << "  Min/Max: " << summary.minValue << " / " << summary.maxValue << "\n";
    std::cout << "  StdDev: " << summary.standardDeviation << "\n";
    std::cout << "  Выбросов обнаружено: " << summary.anomalyCount << "\n";
    std::cout << "CSV-лог сохранён в output/latest_stats.csv\n";
}

}  

int main() {
    setlocale(LC_ALL, "Russian");
    EnsureOutputDirectories();

    while (true) {
        try {
            std::cout << kMainMenu;
            int choice = ReadNumber<int>("> ");

            switch (choice) {
                case 1:
                    RunLazySequenceDemo();
                    break;
                case 2:
                    RunStreamDemo();
                    break;
                case 3:
                    RunOnlineStatisticsDemo();
                    break;
                case 4:
                    RunAllTests();
                    break;
                case 0:
                    return 0;
                default:
                    std::cout << "Неизвестная команда.\n";
                    break;
            }
        } catch (const Exception& error) {
            std::cout << "Ошибка: " << error.what() << "\n";
        } catch (const std::exception& error) {
            std::cout << "std::exception: " << error.what() << "\n";
        }
    }
}
