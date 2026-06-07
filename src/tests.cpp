#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "../include/ArraySequence.hpp"
#include "../include/CircularBuffer.hpp"
#include "../include/LazySequence.hpp"
#include "../include/OnlineStatistics.hpp"
#include "../include/Streams.hpp"

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

struct TestResult {
    std::string name;
    bool passed;
    int assertions;
    int passedAssertions;
};

static ArraySequence<TestResult>* allResults = new ArraySequence<TestResult>();
static int totalAssertions = 0;
static int totalPassed = 0;
static bool currentTestPassed = false;
static int currentTestAssertions = 0;

#define TEST(name) void name()

#define ASSERT_TRUE(condition, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    if (condition) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define ASSERT_EQ(expected, actual, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    auto expectedValue = (expected); \
    auto actualValue = (actual); \
    if (expectedValue == actualValue) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
        std::cout << "    Ожидалось: " << expectedValue << ", Получено: " << actualValue << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        std::cout << "    " COLOR_YELLOW "Ожидалось: " COLOR_RESET << expectedValue << std::endl; \
        std::cout << "    " COLOR_RED "Получено: " COLOR_RESET << actualValue << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define ASSERT_CARDINAL_EQ(expected, actual, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    Cardinal expectedValue = (expected); \
    Cardinal actualValue = (actual); \
    if (expectedValue == actualValue) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
        std::cout << "    Ожидалось: " << expectedValue.ToString() << ", Получено: " << actualValue.ToString() << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        std::cout << "    " COLOR_YELLOW "Ожидалось: " COLOR_RESET << expectedValue.ToString() << std::endl; \
        std::cout << "    " COLOR_RED "Получено: " COLOR_RESET << actualValue.ToString() << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define ASSERT_NEAR(expected, actual, eps, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    double expectedValue = static_cast<double>(expected); \
    double actualValue = static_cast<double>(actual); \
    if (std::fabs(expectedValue - actualValue) <= (eps)) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
        std::cout << "    Ожидалось: " << expectedValue << ", Получено: " << actualValue << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        std::cout << "    " COLOR_YELLOW "Ожидалось: " COLOR_RESET << expectedValue << std::endl; \
        std::cout << "    " COLOR_RED "Получено: " COLOR_RESET << actualValue << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define ASSERT_THROWS(expr, exc, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    bool caught = false; \
    try { \
        expr; \
    } catch (const exc&) { \
        caught = true; \
    } catch (...) { \
        caught = false; \
    } \
    if (caught) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        std::cout << "    Ожидалось исключение " << #exc << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define RUN_TEST(name) do { \
    std::cout << COLOR_BOLD COLOR_CYAN "\n════════════════════════════════════════" COLOR_RESET << std::endl; \
    std::cout << COLOR_BOLD COLOR_BLUE " ТЕСТ: " COLOR_RESET << #name << std::endl; \
    std::cout << COLOR_CYAN "════════════════════════════════════════" COLOR_RESET << std::endl; \
    currentTestPassed = true; \
    currentTestAssertions = 0; \
    try { \
        name(); \
    } catch (const Exception& e) { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " Непредвиденное исключение: " << e.what() << std::endl; \
        currentTestPassed = false; \
    } catch (const std::exception& e) { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " std::exception: " << e.what() << std::endl; \
        currentTestPassed = false; \
    } \
    allResults->Append({#name, currentTestPassed, currentTestAssertions, currentTestPassed ? currentTestAssertions : 0}); \
    std::cout << COLOR_BOLD "\n Результат: " COLOR_RESET; \
    if (currentTestPassed) { \
        std::cout << COLOR_GREEN "ПРОЙДЕН" COLOR_RESET << std::endl; \
    } else { \
        std::cout << COLOR_RED "ПРОВАЛЕН" COLOR_RESET << std::endl; \
    } \
} while (0)

namespace {

double ParseDoubleStrictTest(const std::string& text) {
    size_t consumed = 0;
    double value = std::stod(text, &consumed);
    if (consumed != text.size()) {
        throw InputError("bad token");
    }
    return value;
}

int ParseIntStrictTest(const std::string& text) {
    size_t consumed = 0;
    int value = std::stoi(text, &consumed);
    if (consumed != text.size()) {
        throw InputError("bad token");
    }
    return value;
}

void PrintSubHeader(const std::string& text) {
    std::cout << COLOR_CYAN "\n  -- " COLOR_RESET << text << COLOR_CYAN " --" COLOR_RESET << std::endl;
}

}  

TEST(TestLazySequenceFiniteMemoization) {
    PrintSubHeader("Конечная ленивость и мемоизация");
    int values[] = {2, 4, 6, 8};
    ArraySequence<int> seed(values, 4);
    LazySequence<int> sequence(&seed);

    ASSERT_CARDINAL_EQ(Cardinal::Finite(4), sequence.GetLength(), "Корректная длина");
    ASSERT_EQ(0, static_cast<int>(sequence.GetMaterializedCount()), "До чтения ничего не материализовано");
    ASSERT_EQ(6, sequence.Get(2), "Чтение элемента по индексу");
    ASSERT_EQ(3, static_cast<int>(sequence.GetMaterializedCount()), "Материализован префикс до нужного индекса");
    ASSERT_EQ(8, sequence.GetLast(), "Корректный последний элемент");
}

TEST(TestLazySequenceFibonacci) {
    PrintSubHeader("Рекуррентное порождение Фибоначчи");
    int seeds[] = {1, 1};
    ArraySequence<int> seed(seeds, 2);
    LazySequence<int> fibonacci(
        [](size_t, const Sequence<int>* materialized) -> int {
            int length = materialized->GetLength();
            return materialized->Get(length - 1) + materialized->Get(length - 2);
        },
        &seed
    );

    ASSERT_EQ(1, fibonacci.Get(0), "F0");
    ASSERT_EQ(1, fibonacci.Get(1), "F1");
    ASSERT_EQ(2, fibonacci.Get(2), "F2");
    ASSERT_EQ(3, fibonacci.Get(3), "F3");
    ASSERT_EQ(5, fibonacci.Get(4), "F4");
    ASSERT_EQ(8, fibonacci.Get(5), "F5");
    ASSERT_EQ(6, static_cast<int>(fibonacci.GetMaterializedCount()), "Мемоизация хранит первые 6 значений");
}

TEST(TestLazySequenceComposition) {
    PrintSubHeader("Prepend, InsertAt, Concat, Map, Reduce");
    int values[] = {10, 20, 30};
    ArraySequence<int> seed(values, 3);
    LazySequence<int> sequence(&seed);

    LazySequence<int> updated = sequence.Prepend(5).InsertAt(15, 2);
    int tailValues[] = {40, 50};
    ArraySequence<int> tailSeed(tailValues, 2);
    updated = updated.Concat(LazySequence<int>(&tailSeed));
    LazySequence<int> mapped = updated.Map<int>([](int value) { return value / 5; });

    ASSERT_EQ(5, updated.Get(0), "Prepend работает");
    ASSERT_EQ(15, updated.Get(2), "InsertAt работает");
    ASSERT_EQ(40, updated.Get(5), "Concat добавляет хвост");
    ASSERT_EQ(2, mapped.Get(1), "Map преобразует элементы");
    ASSERT_EQ(34, mapped.Reduce([](int acc, int value) { return acc + value; }, 0), "Reduce суммирует");
}

TEST(TestLazyWhereZipAndSubsequence) {
    PrintSubHeader("Where, Zip и подпоследовательность");
    int values[] = {1, 2, 3, 4, 5, 6};
    ArraySequence<int> seed(values, 6);
    LazySequence<int> sequence(&seed);
    LazySequence<int> filtered = sequence.Where([](int value) { return value % 2 == 0; });

    ASSERT_CARDINAL_EQ(Cardinal::Finite(3), filtered.GetLength(), "Количество чётных элементов");
    ASSERT_EQ(2, filtered.Get(0), "Первый отфильтрованный элемент");
    ASSERT_EQ(6, filtered.Get(2), "Последний отфильтрованный элемент");

    int otherValues[] = {10, 20, 30};
    ArraySequence<int> other(otherValues, 3);
    LazySequence<Tuple<int, int>> zipped = filtered.Zip(other);
    Tuple<int, int> pair = zipped.Get(1);

    ASSERT_EQ(4, pair.first, "Zip сохраняет левый элемент");
    ASSERT_EQ(20, pair.second, "Zip сохраняет правый элемент");

    LazySequence<int> middle = sequence.GetSubsequence(2, 4);
    ASSERT_EQ(3, middle.Get(0), "Subsequence left");
    ASSERT_EQ(5, middle.Get(2), "Subsequence right");
}

TEST(TestInfiniteLazyTake) {
    PrintSubHeader("Бесконечная прогрессия и взятие префикса");
    int seedValue[] = {3};
    ArraySequence<int> seed(seedValue, 1);
    LazySequence<int> arithmetic(
        [](size_t, const Sequence<int>* materialized) -> int {
            return materialized->Get(materialized->GetLength() - 1) + 3;
        },
        &seed
    );

    ASSERT_TRUE(arithmetic.GetLength().IsInfinite(), "Длина распознана как бесконечная");
    LazySequence<int> firstFive = arithmetic.Take(5);
    ASSERT_EQ(3, firstFive.Get(0), "Первый элемент");
    ASSERT_EQ(15, firstFive.Get(4), "Пятый элемент");
    ASSERT_THROWS(arithmetic.GetLast(), InfinityError, "GetLast запрещён для бесконечной последовательности");
}

TEST(TestOrdinalBasics) {
    PrintSubHeader("Ординалы n, omega и omega + n");
    ASSERT_EQ(std::string("7"), Ordinal::Finite(7).ToString(), "Конечный ординал печатается как число");
    ASSERT_EQ(std::string("omega"), Ordinal::Omega().ToString(), "Чистая omega печатается корректно");
    ASSERT_EQ(std::string("omega + 3"), Ordinal::OmegaPlus(3).ToString(), "omega + n печатается корректно");
    ASSERT_TRUE(Ordinal::OmegaPlus(2).HasOmega(), "Ординал с omega содержит omega-часть");
    ASSERT_EQ(5, static_cast<int>(Ordinal::Omega().Add(5).GetOffset()), "Смещение после omega хранится корректно");
}

TEST(TestLazySequenceOrdinalAccess) {
    PrintSubHeader("Доступ к элементам через omega и omega + n");
    int seedValue[] = {1};
    ArraySequence<int> seed(seedValue, 1);
    LazySequence<int> arithmetic(
        [](size_t, const Sequence<int>* materialized) -> int {
            return materialized->Get(materialized->GetLength() - 1) + 1;
        },
        &seed
    );

    ASSERT_THROWS(arithmetic.Get(Ordinal::Omega()), IndexOutOfRange, "У обычной бесконечной последовательности нет элемента в omega");

    LazySequence<int> appended = arithmetic.Append(999).Append(1000);
    ASSERT_EQ(1, appended.Get(0), "Финитный индекс по-прежнему работает");
    ASSERT_EQ(999, appended.Get(Ordinal::Omega()), "Append к бесконечной последовательности даёт элемент в omega");
    ASSERT_EQ(1000, appended.Get(Ordinal::OmegaPlus(1)), "Следующий append даёт элемент в omega + 1");
    ASSERT_EQ(1000, appended.GetLast(), "Для omega + конечный хвост последний элемент доступен");
    ASSERT_TRUE(appended.TryGet(Ordinal::OmegaPlus(2)).IsNone(), "За пределами omega-хвоста возвращается None");
}

TEST(TestLazySequenceOrdinalConcatAndMap) {
    PrintSubHeader("omega-доступ сохраняется после Concat и Map");
    int seedValue[] = {10};
    ArraySequence<int> seed(seedValue, 1);
    LazySequence<int> arithmetic(
        [](size_t, const Sequence<int>* materialized) -> int {
            return materialized->Get(materialized->GetLength() - 1) + 10;
        },
        &seed
    );

    int tailValues[] = {7, 8, 9};
    ArraySequence<int> tailSeed(tailValues, 3);
    LazySequence<int> concatenated = arithmetic.Concat(LazySequence<int>(&tailSeed));
    LazySequence<int> mapped = concatenated.Map<int>([](int value) { return value * 2; });

    ASSERT_EQ(7, concatenated.Get(Ordinal::Omega()), "Concat переносит первый элемент хвоста в omega");
    ASSERT_EQ(9, concatenated.Get(Ordinal::OmegaPlus(2)), "Concat переносит весь конечный хвост в omega + n");
    ASSERT_EQ(14, mapped.Get(Ordinal::Omega()), "Map сохраняет доступ к omega-элементу");
    ASSERT_EQ(18, mapped.Get(Ordinal::OmegaPlus(2)), "Map сохраняет доступ к omega + n");
}

TEST(TestSequenceAndStringStreams) {
    PrintSubHeader("Потоки из Sequence и строки");
    int values[] = {7, 8, 9};
    ArraySequence<int> sequence(values, 3);
    SequenceReadStream<int> sequenceStream(sequence);
    sequenceStream.Open();

    ASSERT_EQ(7, sequenceStream.Read(), "Чтение первого элемента");
    ASSERT_EQ(1, static_cast<int>(sequenceStream.GetPosition()), "Позиция сдвигается");
    ASSERT_EQ(1, static_cast<int>(sequenceStream.Seek(1)), "Seek возвращает новую позицию");
    ASSERT_EQ(8, sequenceStream.Read(), "Чтение после seek");
    sequenceStream.Close();

    StringReadStream<double> stringStream("1.5, 2.5 3.5", ParseDoubleStrictTest);
    stringStream.Open();
    ASSERT_NEAR(1.5, stringStream.Read(), 1e-9, "Парсинг первого числа");
    ASSERT_NEAR(2.5, stringStream.Read(), 1e-9, "Парсинг второго числа");
    ASSERT_EQ(2, static_cast<int>(stringStream.GetPosition()), "Позиция после двух чтений");
}

TEST(TestLazyAndFileStreams) {
    PrintSubHeader("Поток из LazySequence и файловые потоки");
    int seeds[] = {5};
    ArraySequence<int> seed(seeds, 1);
    LazySequence<int> arithmetic(
        [](size_t, const Sequence<int>* materialized) -> int {
            return materialized->Get(materialized->GetLength() - 1) + 5;
        },
        &seed
    );
    LazySequenceReadStream<int> lazyStream(arithmetic);
    lazyStream.Open();
    ASSERT_EQ(5, lazyStream.Read(), "Первый элемент из ленивого потока");
    ASSERT_EQ(10, lazyStream.Read(), "Второй элемент из ленивого потока");
    ASSERT_EQ(1, static_cast<int>(lazyStream.Seek(1)), "Seek на ленивом потоке");
    ASSERT_EQ(10, lazyStream.Read(), "Повторное чтение после возврата назад");
    lazyStream.Close();

    const char* filePath = "/tmp/lab4_stream_test.txt";
    FileWriteStream<int> writer(filePath, [](const int& value) {
        return std::to_string(value);
    });
    writer.Open();
    writer.Write(11);
    writer.Write(22);
    writer.Write(33);
    writer.Close();

    FileReadStream<int> reader(filePath, ParseIntStrictTest);
    reader.Open();
    ASSERT_EQ(11, reader.Read(), "Чтение из файла");
    ASSERT_EQ(22, reader.Read(), "Чтение второго элемента");
    ASSERT_EQ(0, static_cast<int>(reader.Seek(0)), "Возврат в начало файла");
    ASSERT_EQ(11, reader.Read(), "Повторное чтение после seek");
    reader.Close();
    std::remove(filePath);
}

TEST(TestPumpAndMemoryWriter) {
    PrintSubHeader("Копирование потока в память");
    StringReadStream<int> reader("1 2 3 4 5", ParseIntStrictTest);
    MemoryWriteStream<int> writer;
    reader.Open();
    writer.Open();

    ASSERT_EQ(3, static_cast<int>(Pump(reader, writer, 3)), "Pump копирует limit элементов");
    ASSERT_EQ(3, writer.GetItems().GetLength(), "В памяти накоплено 3 элемента");
    ASSERT_EQ(1, writer.GetItems().Get(0), "Первый элемент сохранён");
    ASSERT_EQ(3, writer.GetItems().Get(2), "Третий элемент сохранён");
}

TEST(TestCircularBufferRollover) {
    PrintSubHeader("Циклический буфер и вытеснение старых значений");
    CircularBuffer<int> buffer(3);

    ASSERT_TRUE(buffer.AppendReturningEvicted(10).IsNone(), "Первое добавление без вытеснения");
    ASSERT_TRUE(buffer.AppendReturningEvicted(20).IsNone(), "Второе добавление без вытеснения");
    ASSERT_TRUE(buffer.AppendReturningEvicted(30).IsNone(), "Третье добавление без вытеснения");
    Option<int> evicted = buffer.AppendReturningEvicted(40);

    ASSERT_TRUE(evicted.IsSome(), "На переполнении есть вытесненное значение");
    ASSERT_EQ(10, evicted.GetValue(), "Вытесняется самый старый элемент");
    ASSERT_EQ(3, static_cast<int>(buffer.GetLength()), "Длина остаётся равной capacity");
    ASSERT_EQ(20, buffer.Get(0), "Первый актуальный элемент после rollover");
    ASSERT_EQ(40, buffer.Get(2), "Последний элемент после rollover");
}

TEST(TestCsvAndJsonStreams) {
    PrintSubHeader("CSV/JSON адаптеры потоков");
    CsvReadStream<double> csvStream("1.5,2.5\n3.5, 4.5", ParseDoubleStrictTest);
    csvStream.Open();

    ASSERT_NEAR(1.5, csvStream.Read(), 1e-9, "CSV первое число");
    ASSERT_NEAR(2.5, csvStream.Read(), 1e-9, "CSV второе число");
    ASSERT_EQ(2, static_cast<int>(csvStream.Seek(2)), "CSV seek");
    ASSERT_NEAR(3.5, csvStream.Read(), 1e-9, "CSV третье число после seek");

    JsonArrayReadStream<double> jsonStream("[10, 20.5, -3, 8]", ParseDoubleStrictTest);
    jsonStream.Open();
    ASSERT_NEAR(10.0, jsonStream.Read(), 1e-9, "JSON первое число");
    ASSERT_NEAR(20.5, jsonStream.Read(), 1e-9, "JSON второе число");
    ASSERT_NEAR(-3.0, jsonStream.Read(), 1e-9, "JSON третье число");
}

TEST(TestTryReadBehavior) {
    PrintSubHeader("TryRead возвращает None на конце потока");
    StringReadStream<int> reader("4 5", ParseIntStrictTest);
    reader.Open();

    ASSERT_TRUE(reader.TryRead().IsSome(), "Первое TryRead успешно");
    ASSERT_TRUE(reader.TryRead().IsSome(), "Второе TryRead успешно");
    ASSERT_TRUE(reader.TryRead().IsNone(), "На конце потока возвращается None");
}

TEST(TestOnlineStatisticsSnapshot) {
    PrintSubHeader("Онлайн-метрики на детерминированных данных");
    OnlineStatistics statistics(2, 3.0);
    statistics.Add(1.0);
    statistics.Add(2.0);
    statistics.Add(3.0);
    statistics.Add(4.0);
    StatisticsSnapshot snapshot = statistics.GetSnapshot();

    ASSERT_EQ(4, static_cast<int>(snapshot.count), "Count");
    ASSERT_NEAR(2.5, snapshot.mean, 1e-9, "Mean");
    ASSERT_NEAR(2.5, snapshot.median, 1e-9, "Median");
    ASSERT_NEAR(1.0, snapshot.minValue, 1e-9, "Min");
    ASSERT_NEAR(4.0, snapshot.maxValue, 1e-9, "Max");
    ASSERT_NEAR(1.6666666667, snapshot.variance, 1e-6, "Выборочная дисперсия");
    ASSERT_NEAR(3.5, snapshot.windowAverage, 1e-9, "Скользящее среднее по последним 2 элементам");
}

TEST(TestOnlineStatisticsAnomalies) {
    PrintSubHeader("Обнаружение выбросов");
    OnlineStatistics statistics(3, 2.0);
    statistics.Add(10.0);
    statistics.Add(12.0);
    statistics.Add(11.0);
    statistics.Add(50.0);
    StatisticsSnapshot snapshot = statistics.GetSnapshot();

    ASSERT_TRUE(snapshot.anomalyDetected, "Последнее значение распознано как выброс");
    ASSERT_EQ(1, static_cast<int>(snapshot.anomalyCount), "Счётчик выбросов увеличился");
    ASSERT_TRUE(snapshot.anomalyZScore > 2.0, "z-score превышает заданный порог");
}

int RunAllTests() {
    totalAssertions = 0;
    totalPassed = 0;
    delete allResults;
    allResults = new ArraySequence<TestResult>();

    RUN_TEST(TestLazySequenceFiniteMemoization);
    RUN_TEST(TestLazySequenceFibonacci);
    RUN_TEST(TestLazySequenceComposition);
    RUN_TEST(TestLazyWhereZipAndSubsequence);
    RUN_TEST(TestInfiniteLazyTake);
    RUN_TEST(TestOrdinalBasics);
    RUN_TEST(TestLazySequenceOrdinalAccess);
    RUN_TEST(TestLazySequenceOrdinalConcatAndMap);
    RUN_TEST(TestSequenceAndStringStreams);
    RUN_TEST(TestLazyAndFileStreams);
    RUN_TEST(TestPumpAndMemoryWriter);
    RUN_TEST(TestCircularBufferRollover);
    RUN_TEST(TestCsvAndJsonStreams);
    RUN_TEST(TestTryReadBehavior);
    RUN_TEST(TestOnlineStatisticsSnapshot);
    RUN_TEST(TestOnlineStatisticsAnomalies);

    std::cout << COLOR_BOLD COLOR_CYAN "\n════════════════════════════════════════" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD "ИТОГО:" COLOR_RESET << " пройдено " << totalPassed
              << " из " << totalAssertions << " проверок." << std::endl;

    bool success = totalPassed == totalAssertions;
    std::cout << (success ? COLOR_GREEN : COLOR_RED)
              << (success ? "Все тесты пройдены." : "Есть непройденные тесты.")
              << COLOR_RESET << std::endl;

    delete allResults;
    allResults = nullptr;

    return success ? 0 : 1;
}
