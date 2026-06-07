#include <algorithm>
#include <cerrno>
#include <cmath>
#include <clocale>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ncurses.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "../include/ArraySequence.hpp"
#include "../include/LazySequence.hpp"
#include "../include/OnlineStatistics.hpp"
#include "../include/Streams.hpp"

extern int RunAllTests();

namespace {

using DoubleLazySequence = LazySequence<double>;

enum class Key {
    Up,
    Down,
    Enter,
    Quit,
    Resize,
    Other
};

struct MenuItem {
    int id;
    std::string label;
    std::string description;
    bool selectable;
};

struct LabState {
    bool hasSequence;
    DoubleLazySequence sequence;
    std::string sourceName;
    std::string lastStreamInfo;
    std::string lastStatisticsInfo;

    LabState()
        : hasSequence(false),
          sequence(),
          sourceName("не создана"),
          lastStreamInfo("потоки ещё не запускались"),
          lastStatisticsInfo("статистика ещё не запускалась") {}
};

std::string Truncate(const std::string& text, std::size_t limit) {
    if (text.size() <= limit) {
        return text;
    }
    if (limit <= 3) {
        return text.substr(0, limit);
    }
    return text.substr(0, limit - 3) + "...";
}

std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back("");
    }
    return lines;
}

void FlushWrappedLine(std::vector<std::string>& wrapped, std::string& current) {
    if (!current.empty()) {
        wrapped.push_back(current);
        current.clear();
    }
}

void AppendLongWordChunks(std::vector<std::string>& wrapped, const std::string& word, int width) {
    std::size_t start = 0;
    while (start < word.size()) {
        const std::size_t count =
            std::min<std::size_t>(static_cast<std::size_t>(width), word.size() - start);
        wrapped.push_back(word.substr(start, count));
        start += count;
    }
}

void AppendWordToWrappedLine(
    std::vector<std::string>& wrapped,
    std::string& current,
    const std::string& word,
    int width
) {
    if (static_cast<int>(word.size()) > width) {
        FlushWrappedLine(wrapped, current);
        AppendLongWordChunks(wrapped, word, width);
        return;
    }

    if (current.empty()) {
        current = word;
        return;
    }

    if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
        current += " " + word;
        return;
    }

    FlushWrappedLine(wrapped, current);
    current = word;
}

void WrapSourceLine(std::vector<std::string>& wrapped, const std::string& sourceLine, int width) {
    if (sourceLine.empty()) {
        wrapped.push_back("");
        return;
    }

    std::istringstream words(sourceLine);
    std::string word;
    std::string current;

    while (words >> word) {
        AppendWordToWrappedLine(wrapped, current, word, width);
    }

    FlushWrappedLine(wrapped, current);
}

std::vector<std::string> WrapText(const std::string& text, int width) {
    if (width <= 1) {
        return {""};
    }

    std::vector<std::string> wrapped;
    for (const std::string& sourceLine : SplitLines(text)) {
        WrapSourceLine(wrapped, sourceLine, width);
    }

    if (wrapped.empty()) {
        wrapped.push_back("");
    }
    return wrapped;
}

std::string FormatDouble(double value, int precision = 3) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string SerializeDouble(double value) {
    return FormatDouble(value, 6);
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

void EnsureOutputDirectories() {
    auto ensureDirectory = [](const char* path) {
#ifdef _WIN32
        int result = _mkdir(path);
#else
        int result = mkdir(path, 0755);
#endif
        if (result != 0 && errno != EEXIST) {
            throw InvalidState(std::string("не удалось создать директорию: ") + path);
        }
    };

    ensureDirectory("output");
    ensureDirectory("data");
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

std::string FormatSequencePrefix(const DoubleLazySequence& sequence, std::size_t count) {
    std::string result = "[";
    bool stopped = false;

    for (std::size_t index = 0; index < count; index++) {
        try {
            if (index > 0) {
                result += ", ";
            }
            result += FormatDouble(sequence.Get(static_cast<int>(index)));
        } catch (const Exception&) {
            stopped = true;
            break;
        }
    }

    if (stopped) {
        result += result.size() == 1 ? "..." : ", ...";
    }

    result += "]";
    return result;
}

std::string FormatTuplePrefix(const LazySequence<Tuple<double, double>>& sequence, std::size_t count) {
    std::string result = "[";
    for (std::size_t index = 0; index < count; index++) {
        Tuple<double, double> item = sequence.Get(static_cast<int>(index));
        if (index > 0) {
            result += ", ";
        }
        result += "(" + FormatDouble(item.first) + ", " + FormatDouble(item.second) + ")";
    }
    result += "]";
    return result;
}

std::string DescribeLabState(const LabState& state) {
    std::ostringstream builder;
    builder << "LazySequence: ";
    if (!state.hasSequence) {
        builder << "не создана";
    } else {
        const std::size_t materialized = state.sequence.GetMaterializedCount();
        builder << state.sourceName
                << "\nДлина: " << state.sequence.GetLength().ToString()
                << "\nМатериализовано: " << materialized;
        if (materialized == 0) {
            builder << "\nПрефикс: ещё не материализован";
        } else {
            builder << "\nПрефикс: "
                    << FormatSequencePrefix(state.sequence, std::min<std::size_t>(materialized, 5));
        }
    }

    builder << "\n\nStreams:\n" << state.lastStreamInfo;
    builder << "\n\nOnline statistics:\n" << state.lastStatisticsInfo;
    return builder.str();
}

std::vector<MenuItem> BuildMenuItems() {
    return {
        {-1, "СОЗДАНИЕ LAZYSEQUENCE", "", false},
        {1, "1. Конечная последовательность", "Создать LazySequence из чисел, введённых вручную.", true},
        {2, "2. Арифметическая прогрессия", "Создать бесконечную арифметическую прогрессию.", true},
        {3, "3. Последовательность Фибоначчи", "Создать бесконечную последовательность по двум первым элементам.", true},
        {4, "4. Random walk", "Создать детерминированный random walk по seed и максимальному шагу.", true},
        {5, "5. Псевдо-датчик с выбросами", "Создать поток значений датчика с периодическими аномалиями.", true},
        {-1, "ОПЕРАЦИИ LAZYSEQUENCE", "", false},
        {6, "6. Показать префикс", "Показать первые k элементов текущей ленивой последовательности.", true},
        {7, "7. Получить элемент по индексу", "Получить элемент по обычному конечному индексу.", true},
        {8, "8. Получить элемент через omega", "Проверить ординальный доступ: omega или omega + n.", true},
        {9, "9. Показать длину", "Показать Cardinal-длину и число материализованных элементов.", true},
        {10, "10. Prepend", "Добавить элемент в начало текущей последовательности.", true},
        {11, "11. InsertAt", "Вставить элемент в указанную позицию.", true},
        {12, "12. Concat с хвостом", "Сцепить текущую последовательность с конечным хвостом.", true},
        {13, "13. Map (x * factor)", "Лениво умножить каждый элемент на заданный множитель.", true},
        {14, "14. Where (x >= threshold)", "Лениво оставить элементы не меньше порога.", true},
        {15, "15. Zip с конечной последовательностью", "Построить пары из текущей и введённой последовательности.", true},
        {16, "16. Взять подпоследовательность", "Сделать текущей подпоследовательность по диапазону.", true},
        {-1, "STREAMS И СТАТИСТИКА", "", false},
        {17, "17. Предпросмотр потока", "Выбрать источник потока и прочитать первые k элементов.", true},
        {18, "18. Скопировать поток в файл", "Скопировать k элементов в память и output/stream_dump.txt.", true},
        {19, "19. Онлайн-статистика", "Обработать поток и записать CSV/JSON-результаты.", true},
        {-1, "ПРОВЕРКА", "", false},
        {20, "20. Запустить модульные тесты", "Запускает все автоматические тесты лабораторной работы.", true},
        {-1, "ЗАВЕРШЕНИЕ", "", false},
        {0, "0. Выход", "Завершает работу программы.", true},
    };
}

int FindFirstSelectable(const std::vector<MenuItem>& items) {
    for (std::size_t i = 0; i < items.size(); i++) {
        if (items[i].selectable) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

int MoveSelection(const std::vector<MenuItem>& items, int currentIndex, int direction) {
    int nextIndex = currentIndex;
    const int size = static_cast<int>(items.size());

    do {
        nextIndex += direction;
        if (nextIndex < 0) {
            nextIndex = size - 1;
        }
        if (nextIndex >= size) {
            nextIndex = 0;
        }
    } while (!items[static_cast<std::size_t>(nextIndex)].selectable && nextIndex != currentIndex);

    return nextIndex;
}

class TerminalUi {
public:
    TerminalUi()
        : interactive_(isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0),
          cursesStarted_(false),
          suspended_(false),
          rows_(0),
          cols_(0),
          menuOffset_(0),
          menuWin_(nullptr),
          stateWin_(nullptr),
          statusWin_(nullptr),
          footerWin_(nullptr) {
        if (!interactive_) {
            return;
        }

        initscr();
        cursesStarted_ = true;
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        set_escdelay(25);

        if (has_colors()) {
            start_color();
            use_default_colors();
            init_pair(1, COLOR_BLACK, COLOR_CYAN);
            init_pair(2, COLOR_CYAN, -1);
            init_pair(3, COLOR_YELLOW, -1);
        }

        UpdateLayout();
    }

    ~TerminalUi() {
        DestroyWindows();
        if (cursesStarted_) {
            endwin();
        }
    }

    bool IsInteractive() const {
        return interactive_;
    }

    Key ReadKey() {
        if (!interactive_) {
            return Key::Other;
        }

        UpdateLayout();
        WINDOW* source = menuWin_ != nullptr ? menuWin_ : stdscr;
        const int code = wgetch(source);
        switch (code) {
            case KEY_UP:
                return Key::Up;
            case KEY_DOWN:
                return Key::Down;
            case 10:
            case 13:
            case KEY_ENTER:
                return Key::Enter;
            case 'q':
            case 'Q':
                return Key::Quit;
            case KEY_RESIZE:
                return Key::Resize;
            default:
                return Key::Other;
        }
    }

    std::string ReadLine(const std::string& prompt) {
        return PromptText("Ввод", prompt);
    }

    int PromptInt(const std::string& prompt) {
        while (true) {
            const std::string line = ReadLine(prompt);
            std::istringstream input(line);
            int value = 0;
            char extra = '\0';
            if ((input >> value) && !(input >> extra)) {
                return value;
            }
            ShowDialogMessage("Ошибка ввода", "Введите целое число.");
        }
    }

    double PromptDouble(const std::string& prompt) {
        while (true) {
            const std::string line = ReadLine(prompt);
            std::istringstream input(line);
            double value = 0.0;
            char extra = '\0';
            if ((input >> value) && !(input >> extra)) {
                return value;
            }
            ShowDialogMessage("Ошибка ввода", "Введите вещественное число.");
        }
    }

    void SuspendForStdIo() {
        if (!interactive_ || !cursesStarted_ || suspended_) {
            return;
        }

        def_prog_mode();
        endwin();
        suspended_ = true;
    }

    void ResumeFromStdIo() {
        if (!interactive_ || !cursesStarted_ || !suspended_) {
            return;
        }

        reset_prog_mode();
        refresh();
        keypad(stdscr, TRUE);
        noecho();
        cbreak();
        curs_set(0);
        suspended_ = false;
        UpdateLayout();
    }

    void RenderMenu(const std::vector<MenuItem>& items,
                    int selectedIndex,
                    const LabState& state,
                    const std::string& status) {
        if (!interactive_) {
            return;
        }

        UpdateLayout();

        if (IsTooSmall()) {
            RenderTooSmallMessage();
            return;
        }

        erase();
        DrawHeader();
        DrawMenuWindow(items, selectedIndex);
        DrawStateWindow(state);
        DrawStatusWindow(status);
        DrawFooter(items, selectedIndex);
        refresh();
        wrefresh(menuWin_);
        wrefresh(stateWin_);
        wrefresh(statusWin_);
        wrefresh(footerWin_);
    }

private:
    static constexpr int kMinRows = 27;
    static constexpr int kMinCols = 90;

    void DestroyWindows() {
        if (menuWin_ != nullptr) {
            delwin(menuWin_);
            menuWin_ = nullptr;
        }
        if (stateWin_ != nullptr) {
            delwin(stateWin_);
            stateWin_ = nullptr;
        }
        if (statusWin_ != nullptr) {
            delwin(statusWin_);
            statusWin_ = nullptr;
        }
        if (footerWin_ != nullptr) {
            delwin(footerWin_);
            footerWin_ = nullptr;
        }
    }

    bool IsTooSmall() const {
        return rows_ < kMinRows || cols_ < kMinCols;
    }

    void UpdateLayout() {
        if (!interactive_) {
            return;
        }

        int newRows = 0;
        int newCols = 0;
        getmaxyx(stdscr, newRows, newCols);
        if (newRows == rows_ && newCols == cols_ &&
            menuWin_ != nullptr && stateWin_ != nullptr &&
            statusWin_ != nullptr && footerWin_ != nullptr) {
            return;
        }

        rows_ = newRows;
        cols_ = newCols;

        DestroyWindows();
        if (IsTooSmall()) {
            return;
        }

        const int headerHeight = 3;
        const int footerHeight = 4;
        const int bodyHeight = rows_ - headerHeight - footerHeight;
        const int menuWidth = std::max(40, cols_ * 42 / 100);
        const int rightWidth = cols_ - menuWidth;
        const int stateHeight = std::max(9, bodyHeight / 3);
        const int statusHeight = bodyHeight - stateHeight;

        menuWin_ = newwin(bodyHeight, menuWidth, headerHeight, 0);
        stateWin_ = newwin(stateHeight, rightWidth, headerHeight, menuWidth);
        statusWin_ = newwin(statusHeight, rightWidth, headerHeight + stateHeight, menuWidth);
        footerWin_ = newwin(footerHeight, cols_, rows_ - footerHeight, 0);

        keypad(menuWin_, TRUE);
    }

    void DrawBoxTitle(WINDOW* win, const std::string& title) {
        box(win, 0, 0);
        wattron(win, A_BOLD);
        mvwprintw(win, 0, 2, " %s ", title.c_str());
        wattroff(win, A_BOLD);
    }

    void DrawHeader() {
        attron(A_BOLD);
        mvprintw(0, 2, "Лабораторная работа №4");
        mvprintw(1, 2, "Полноэкранный интерфейс: LazySequence, Streams и онлайн-статистика");
        attroff(A_BOLD);
        mvhline(2, 0, ACS_HLINE, cols_);
    }

    void AdjustMenuOffset(const std::vector<MenuItem>& items, int selectedIndex, int visibleLines) {
        if (visibleLines <= 0) {
            menuOffset_ = 0;
            return;
        }

        if (selectedIndex < menuOffset_) {
            menuOffset_ = selectedIndex;
        } else if (selectedIndex >= menuOffset_ + visibleLines) {
            menuOffset_ = selectedIndex - visibleLines + 1;
        }

        const int maxOffset = std::max(0, static_cast<int>(items.size()) - visibleLines);
        menuOffset_ = std::max(0, std::min(menuOffset_, maxOffset));
    }

    void PrintWindowLine(WINDOW* win, int row, int col, int maxWidth, const std::string& text, int attributes = 0) {
        std::string cropped = Truncate(text, static_cast<std::size_t>(std::max(0, maxWidth)));
        if (attributes != 0) {
            wattron(win, attributes);
        }
        mvwprintw(win, row, col, "%-*s", maxWidth, cropped.c_str());
        if (attributes != 0) {
            wattroff(win, attributes);
        }
    }

    void DrawMenuWindow(const std::vector<MenuItem>& items, int selectedIndex) {
        werase(menuWin_);
        DrawBoxTitle(menuWin_, "Меню");

        const int innerHeight = getmaxy(menuWin_) - 2;
        const int innerWidth = getmaxx(menuWin_) - 4;
        AdjustMenuOffset(items, selectedIndex, innerHeight);

        for (int row = 0; row < innerHeight; row++) {
            const int itemIndex = menuOffset_ + row;
            if (itemIndex >= static_cast<int>(items.size())) {
                break;
            }

            const MenuItem& item = items[static_cast<std::size_t>(itemIndex)];
            if (!item.selectable) {
                PrintWindowLine(menuWin_, row + 1, 2, innerWidth, item.label, A_BOLD);
                continue;
            }

            int attributes = 0;
            if (itemIndex == selectedIndex) {
                attributes = A_BOLD | A_REVERSE;
                if (has_colors()) {
                    attributes |= COLOR_PAIR(1);
                }
            }
            PrintWindowLine(menuWin_, row + 1, 2, innerWidth, item.label, attributes);
        }

        if (menuOffset_ > 0) {
            mvwprintw(menuWin_, 1, getmaxx(menuWin_) - 3, "^");
        }
        if (menuOffset_ + innerHeight < static_cast<int>(items.size())) {
            mvwprintw(menuWin_, getmaxy(menuWin_) - 2, getmaxx(menuWin_) - 3, "v");
        }
    }

    void DrawStateWindow(const LabState& state) {
        werase(stateWin_);
        DrawBoxTitle(stateWin_, "Состояние");

        const int innerWidth = getmaxx(stateWin_) - 4;
        const std::vector<std::string> lines = WrapText(DescribeLabState(state), innerWidth);
        const int visible = std::min<int>(static_cast<int>(lines.size()), getmaxy(stateWin_) - 2);

        for (int i = 0; i < visible; i++) {
            PrintWindowLine(stateWin_, i + 1, 2, innerWidth, lines[static_cast<std::size_t>(i)]);
        }
    }

    void DrawStatusWindow(const std::string& status) {
        werase(statusWin_);
        DrawBoxTitle(statusWin_, "Результат");

        const int innerWidth = getmaxx(statusWin_) - 4;
        const int innerHeight = getmaxy(statusWin_) - 2;
        const std::vector<std::string> lines = WrapText(status, innerWidth);
        const int visible = std::min<int>(static_cast<int>(lines.size()), innerHeight);

        for (int i = 0; i < visible; i++) {
            PrintWindowLine(statusWin_, i + 1, 2, innerWidth, lines[static_cast<std::size_t>(i)]);
        }

        if (static_cast<int>(lines.size()) > innerHeight) {
            PrintWindowLine(statusWin_, innerHeight, 2, innerWidth, "...");
        }
    }

    void DrawFooter(const std::vector<MenuItem>& items, int selectedIndex) {
        werase(footerWin_);
        box(footerWin_, 0, 0);

        const MenuItem& currentItem = items[static_cast<std::size_t>(selectedIndex)];
        const std::string help = "Стрелки: выбор  Enter: выполнить  q: выход";
        const std::string current = "Текущий пункт: " + currentItem.label;
        const std::string description = currentItem.selectable
            ? "Что делает: " + currentItem.description
            : "Что делает: это раздел меню.";

        PrintWindowLine(footerWin_, 1, 2, cols_ - 4, help);
        if (cols_ > 50) {
            PrintWindowLine(footerWin_, 1, std::max(2, cols_ / 2), cols_ / 2 - 3, current);
        }
        PrintWindowLine(footerWin_, 2, 2, cols_ - 4, description);
    }

    void RenderTooSmallMessage() {
        erase();
        attron(A_BOLD);
        mvprintw(1, 2, "Окно терминала слишком маленькое");
        attroff(A_BOLD);
        mvprintw(3, 2, "Для полноэкранного интерфейса нужно минимум %d x %d.", kMinCols, kMinRows);
        mvprintw(5, 2, "Увеличьте окно терминала или запустите программу без TTY для обычного режима.");
        mvprintw(7, 2, "Нажмите q для выхода.");
        refresh();
    }

    std::string PromptText(const std::string& title, const std::string& prompt) {
        UpdateLayout();
        if (IsTooSmall()) {
            throw InvalidState("увеличьте окно терминала для ввода данных");
        }

        const int maxDialogWidth = std::max(40, cols_ - 10);
        const int dialogWidth = std::min(maxDialogWidth, std::max(56, static_cast<int>(prompt.size()) + 8));
        const std::vector<std::string> promptLines = WrapText(prompt, dialogWidth - 4);
        const int dialogHeight = std::max(8, static_cast<int>(promptLines.size()) + 5);
        const int startY = std::max(1, (rows_ - dialogHeight) / 2);
        const int startX = std::max(1, (cols_ - dialogWidth) / 2);

        WINDOW* dialog = newwin(dialogHeight, dialogWidth, startY, startX);
        keypad(dialog, TRUE);
        DrawBoxTitle(dialog, title);

        for (std::size_t i = 0; i < promptLines.size() && static_cast<int>(i) < dialogHeight - 4; i++) {
            PrintWindowLine(dialog, static_cast<int>(i) + 1, 2, dialogWidth - 4, promptLines[i]);
        }

        PrintWindowLine(dialog, dialogHeight - 2, 2, dialogWidth - 6, "> ");
        wmove(dialog, dialogHeight - 2, 4);
        wrefresh(dialog);

        echo();
        curs_set(1);

        char buffer[256] = {};
        wgetnstr(dialog, buffer, 255);

        noecho();
        curs_set(0);

        const std::string result(buffer);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return result;
    }

    void ShowDialogMessage(const std::string& title, const std::string& message) {
        UpdateLayout();
        if (IsTooSmall()) {
            return;
        }

        const int dialogWidth = std::min(cols_ - 6, std::max(50, static_cast<int>(message.size()) + 8));
        const std::vector<std::string> messageLines = WrapText(message, dialogWidth - 4);
        const int dialogHeight = std::max(7, static_cast<int>(messageLines.size()) + 4);
        const int startY = std::max(1, (rows_ - dialogHeight) / 2);
        const int startX = std::max(1, (cols_ - dialogWidth) / 2);

        WINDOW* dialog = newwin(dialogHeight, dialogWidth, startY, startX);
        keypad(dialog, TRUE);
        DrawBoxTitle(dialog, title);

        for (std::size_t i = 0; i < messageLines.size() && static_cast<int>(i) < dialogHeight - 3; i++) {
            PrintWindowLine(dialog, static_cast<int>(i) + 1, 2, dialogWidth - 4, messageLines[i]);
        }
        PrintWindowLine(dialog, dialogHeight - 2, 2, dialogWidth - 4, "Нажмите любую клавишу...");

        wrefresh(dialog);
        wgetch(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
    }

    bool interactive_;
    bool cursesStarted_;
    bool suspended_;
    int rows_;
    int cols_;
    int menuOffset_;
    WINDOW* menuWin_;
    WINDOW* stateWin_;
    WINDOW* statusWin_;
    WINDOW* footerWin_;
};

std::string ReadText(const std::string& prompt, TerminalUi* ui = nullptr) {
    if (ui != nullptr && ui->IsInteractive()) {
        return ui->ReadLine(prompt);
    }

    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputError("ввод прерван");
    }
    return line;
}

int ReadInt(const std::string& prompt, TerminalUi* ui = nullptr) {
    if (ui != nullptr && ui->IsInteractive()) {
        return ui->PromptInt(prompt);
    }

    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            throw InputError("ввод прерван");
        }

        std::istringstream input(line);
        int value = 0;
        char extra = '\0';
        if ((input >> value) && !(input >> extra)) {
            return value;
        }

        std::cout << "Некорректный ввод. Введите целое число." << std::endl;
    }
}

std::size_t ReadSize(const std::string& prompt, TerminalUi* ui = nullptr) {
    const int value = ReadInt(prompt, ui);
    if (value < 0) {
        throw InputError("значение не может быть отрицательным");
    }
    return static_cast<std::size_t>(value);
}

double ReadDouble(const std::string& prompt, TerminalUi* ui = nullptr) {
    if (ui != nullptr && ui->IsInteractive()) {
        return ui->PromptDouble(prompt);
    }

    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            throw InputError("ввод прерван");
        }

        std::istringstream input(line);
        double value = 0.0;
        char extra = '\0';
        if ((input >> value) && !(input >> extra)) {
            return value;
        }

        std::cout << "Некорректный ввод. Введите вещественное число." << std::endl;
    }
}

void ShowNumericMenu(const LabState& state) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        ЛАБОРАТОРНАЯ РАБОТА №4 - LAZYSEQUENCE И STREAMS          ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  СОЗДАНИЕ LAZYSEQUENCE:                                          ║" << std::endl;
    std::cout << "║   1. Конечная последовательность                                 ║" << std::endl;
    std::cout << "║   2. Арифметическая прогрессия                                   ║" << std::endl;
    std::cout << "║   3. Последовательность Фибоначчи                                ║" << std::endl;
    std::cout << "║   4. Random walk                                                 ║" << std::endl;
    std::cout << "║   5. Псевдо-датчик с выбросами                                  ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ОПЕРАЦИИ LAZYSEQUENCE:                                          ║" << std::endl;
    std::cout << "║   6. Показать префикс       12. Concat с хвостом                 ║" << std::endl;
    std::cout << "║   7. Получить по индексу    13. Map (x * factor)                 ║" << std::endl;
    std::cout << "║   8. Получить через omega   14. Where (x >= threshold)           ║" << std::endl;
    std::cout << "║   9. Показать длину         15. Zip                              ║" << std::endl;
    std::cout << "║   10. Prepend               16. Взять подпоследовательность      ║" << std::endl;
    std::cout << "║   11. InsertAt                                                   ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  STREAMS И СТАТИСТИКА:                                           ║" << std::endl;
    std::cout << "║   17. Предпросмотр потока                                        ║" << std::endl;
    std::cout << "║   18. Скопировать поток в файл                                   ║" << std::endl;
    std::cout << "║   19. Онлайн-статистика                                          ║" << std::endl;
    std::cout << "║   20. Запустить модульные тесты                                  ║" << std::endl;
    std::cout << "║   0. Выход                                                       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Текущее состояние:\n" << DescribeLabState(state) << std::endl;
    std::cout << "> ";
}

DoubleLazySequence MakeManualFiniteSequence(TerminalUi* ui) {
    ArraySequence<double> values = ParseNumbersLine(
        ReadText("Введите числа через пробел/запятую: ", ui)
    );
    return DoubleLazySequence(&values);
}

DoubleLazySequence MakeArithmeticSequence(TerminalUi* ui) {
    double start = ReadDouble("Начальное значение: ", ui);
    double step = ReadDouble("Шаг: ", ui);
    double seedValue[1] = {start};
    ArraySequence<double> seed(seedValue, 1);

    return DoubleLazySequence(
        [step](std::size_t, const Sequence<double>* materialized) -> double {
            return materialized->Get(materialized->GetLength() - 1) + step;
        },
        &seed
    );
}

DoubleLazySequence MakeFibonacciSequence(TerminalUi* ui) {
    double first = ReadDouble("Первый элемент: ", ui);
    double second = ReadDouble("Второй элемент: ", ui);
    double seeds[2] = {first, second};
    ArraySequence<double> seed(seeds, 2);

    return DoubleLazySequence(
        [](std::size_t, const Sequence<double>* materialized) -> double {
            int length = materialized->GetLength();
            return materialized->Get(length - 1) + materialized->Get(length - 2);
        },
        &seed
    );
}

DoubleLazySequence MakeRandomWalkSequence(TerminalUi* ui) {
    double start = ReadDouble("Старт random walk: ", ui);
    double maxStep = ReadDouble("Максимальный модуль шага: ", ui);
    int seedInput = ReadInt("Seed генератора: ", ui);
    unsigned int seed = static_cast<unsigned int>(seedInput);
    double seedValue[1] = {start};
    ArraySequence<double> sequenceSeed(seedValue, 1);

    return DoubleLazySequence(
        [seed, maxStep](std::size_t index, const Sequence<double>* materialized) -> double {
            std::mt19937 generator(seed + static_cast<unsigned int>(index));
            std::uniform_real_distribution<double> distribution(-maxStep, maxStep);
            return materialized->Get(materialized->GetLength() - 1) + distribution(generator);
        },
        &sequenceSeed
    );
}

DoubleLazySequence MakeSensorSequence(TerminalUi* ui) {
    double baseline = ReadDouble("Базовый уровень сигнала: ", ui);
    double amplitude = ReadDouble("Амплитуда колебаний: ", ui);
    double anomalyJump = ReadDouble("Размер выброса: ", ui);
    int anomalyPeriod = ReadInt("Период выбросов (например 20): ", ui);

    return DoubleLazySequence::GenerateIndexed(
        [baseline, amplitude, anomalyJump, anomalyPeriod](std::size_t index) -> double {
            double wave = baseline + amplitude * std::sin(static_cast<double>(index) * 0.25);
            double drift = static_cast<double>(index % 7) * 0.15;
            if (anomalyPeriod > 0 && index > 0 && index % static_cast<std::size_t>(anomalyPeriod) == 0) {
                return wave + drift + anomalyJump;
            }
            return wave + drift;
        }
    );
}

DoubleLazySequence CreateLazySequenceFromUser(TerminalUi* ui, std::string& sourceName) {
    const std::string prompt =
        "Источник LazySequence: 1 - конечная вручную, 2 - арифметическая прогрессия, "
        "3 - Фибоначчи, 4 - random walk, 5 - псевдо-датчик.";
    const int choice = ReadInt(prompt + " Выбор: ", ui);

    switch (choice) {
        case 1:
            sourceName = "конечная последовательность";
            return MakeManualFiniteSequence(ui);
        case 2:
            sourceName = "арифметическая прогрессия";
            return MakeArithmeticSequence(ui);
        case 3:
            sourceName = "последовательность Фибоначчи";
            return MakeFibonacciSequence(ui);
        case 4:
            sourceName = "random walk";
            return MakeRandomWalkSequence(ui);
        case 5:
            sourceName = "псевдо-датчик с выбросами";
            return MakeSensorSequence(ui);
        default:
            throw InputError("неизвестный сценарий LazySequence");
    }
}

void SetSequence(LabState& state, const DoubleLazySequence& sequence, const std::string& sourceName) {
    state.sequence = sequence;
    state.sourceName = sourceName;
    state.hasSequence = true;
}

void RequireSequence(const LabState& state) {
    if (!state.hasSequence) {
        throw InvalidState("сначала создайте LazySequence");
    }
}

Ordinal ReadOrdinal(TerminalUi* ui) {
    const int kind = ReadInt("Индекс: 1 - конечный n, 2 - omega, 3 - omega + n. Выбор: ", ui);
    if (kind == 1) {
        return Ordinal::Finite(ReadSize("n: ", ui));
    }
    if (kind == 2) {
        return Ordinal::Omega();
    }
    if (kind == 3) {
        return Ordinal::OmegaPlus(ReadSize("n после omega: ", ui));
    }
    throw InputError("неизвестный тип ординала");
}

std::string BuildLengthStatus(const LabState& state) {
    std::ostringstream builder;
    builder << "Длина: " << state.sequence.GetLength().ToString()
            << "\nМатериализовано элементов: " << state.sequence.GetMaterializedCount();
    return builder.str();
}

std::string ReadPathOrDefault(
    const std::string& prompt,
    const std::string& defaultPath,
    TerminalUi* ui
) {
    std::string path = ReadText(prompt, ui);
    return path.empty() ? defaultPath : path;
}

ReadOnlyStream<double>* CreateStringReadStream(TerminalUi* ui) {
    std::string line = ReadText("Введите числа через пробел/запятую: ", ui);
    return new StringReadStream<double>(line, ParseDoubleStrict);
}

ReadOnlyStream<double>* CreateCurrentLazyReadStream(const LabState& state) {
    RequireSequence(state);
    return new LazySequenceReadStream<double>(state.sequence);
}

ReadOnlyStream<double>* CreateNewLazyReadStream(TerminalUi* ui) {
    std::string sourceName;
    DoubleLazySequence sequence = CreateLazySequenceFromUser(ui, sourceName);
    return new LazySequenceReadStream<double>(sequence);
}

ReadOnlyStream<double>* CreateTextFileReadStream(TerminalUi* ui) {
    std::string path = ReadPathOrDefault(
        "Путь к TXT-файлу (Enter для data/sample_stream.txt): ",
        "data/sample_stream.txt",
        ui
    );
    return new FileReadStream<double>(path, ParseDoubleStrict);
}

ReadOnlyStream<double>* CreateCsvFileReadStream(TerminalUi* ui) {
    std::string path = ReadPathOrDefault(
        "Путь к CSV-файлу (Enter для data/sample_stream.csv): ",
        "data/sample_stream.csv",
        ui
    );
    return new CsvReadStream<double>(CsvReadStream<double>::FromFile(path, ParseDoubleStrict));
}

ReadOnlyStream<double>* CreateJsonFileReadStream(TerminalUi* ui) {
    std::string path = ReadPathOrDefault(
        "Путь к JSON-файлу (Enter для data/sample_stream.json): ",
        "data/sample_stream.json",
        ui
    );
    return new JsonArrayReadStream<double>(JsonArrayReadStream<double>::FromFile(path, ParseDoubleStrict));
}

ReadOnlyStream<double>* CreateReadStreamFromUser(const LabState& state, TerminalUi* ui) {
    const std::string prompt =
        "Источник потока: 1 - строка чисел, 2 - текущая LazySequence, "
        "3 - новая LazySequence, 4 - TXT, 5 - CSV, 6 - JSON.";
    const int choice = ReadInt(prompt + " Выбор: ", ui);

    switch (choice) {
        case 1: return CreateStringReadStream(ui);
        case 2: return CreateCurrentLazyReadStream(state);
        case 3: return CreateNewLazyReadStream(ui);
        case 4: return CreateTextFileReadStream(ui);
        case 5: return CreateCsvFileReadStream(ui);
        case 6: return CreateJsonFileReadStream(ui);
        default:
            throw InputError("неизвестный источник потока");
    }
}

std::string ToCsvRow(std::size_t index, double value, const StatisticsSnapshot& snapshot) {
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

std::string BuildStatisticsSummaryJson(const StatisticsSnapshot& summary) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6)
           << "{\n"
           << "  \"count\": " << summary.count << ",\n"
           << "  \"lastValue\": " << summary.lastValue << ",\n"
           << "  \"minValue\": " << summary.minValue << ",\n"
           << "  \"maxValue\": " << summary.maxValue << ",\n"
           << "  \"mean\": " << summary.mean << ",\n"
           << "  \"variance\": " << summary.variance << ",\n"
           << "  \"standardDeviation\": " << summary.standardDeviation << ",\n"
           << "  \"median\": " << summary.median << ",\n"
           << "  \"windowAverage\": " << summary.windowAverage << ",\n"
           << "  \"anomalyDetected\": " << (summary.anomalyDetected ? "true" : "false") << ",\n"
           << "  \"anomalyZScore\": " << summary.anomalyZScore << ",\n"
           << "  \"anomalyCount\": " << summary.anomalyCount << "\n"
           << "}";
    return stream.str();
}

std::string FormatStatisticsSummary(const StatisticsSnapshot& summary) {
    std::ostringstream stream;
    stream << "Обработано элементов: " << summary.count
           << "\nMean: " << FormatDouble(summary.mean, 6)
           << "\nMedian: " << FormatDouble(summary.median, 6)
           << "\nMin/Max: " << FormatDouble(summary.minValue, 6)
           << " / " << FormatDouble(summary.maxValue, 6)
           << "\nStdDev: " << FormatDouble(summary.standardDeviation, 6)
           << "\nВыбросов обнаружено: " << summary.anomalyCount;
    return stream.str();
}

struct StatisticsRunSettings {
    std::size_t maxItems;
    std::size_t windowSize;
    double anomalyThreshold;
};

StatisticsRunSettings ReadStatisticsRunSettings(TerminalUi* ui) {
    StatisticsRunSettings settings{};
    settings.maxItems = ReadSize("Сколько элементов обработать: ", ui);
    settings.windowSize = ReadSize("Размер скользящего окна (0 - выключить): ", ui);
    settings.anomalyThreshold = ReadDouble("Порог z-score для выбросов: ", ui);
    return settings;
}

std::size_t ProcessStatisticsRows(
    ReadOnlyStream<double>& reader,
    OnlineStatistics& stats,
    FileWriteStream<std::string>& csvWriter,
    std::size_t maxItems
) {
    std::size_t processed = 0;
    while (processed < maxItems && !reader.IsEndOfStream()) {
        double value = reader.Read();
        stats.Add(value);
        StatisticsSnapshot snapshot = stats.GetSnapshot();
        processed++;
        csvWriter.Write(ToCsvRow(processed, value, snapshot));
    }
    return processed;
}

void WriteStatisticsSummaryJson(const StatisticsSnapshot& summary) {
    FileWriteStream<std::string> jsonWriter(
        "output/latest_stats_summary.json",
        [](const std::string& line) { return line; }
    );
    jsonWriter.Open();
    jsonWriter.Write(BuildStatisticsSummaryJson(summary));
    jsonWriter.Close();
}

std::string BuildStatisticsSuccessStatus(const StatisticsSnapshot& summary) {
    return FormatStatisticsSummary(summary)
         + "\nCSV-лог сохранён в output/latest_stats.csv"
         + "\nJSON-сводка сохранена в output/latest_stats_summary.json";
}

void CloseAndDeleteReader(ReadOnlyStream<double>* reader) {
    if (reader != nullptr) {
        delete reader;
    }
}

void ApplyCreateManualSequence(LabState& state, std::string& status, TerminalUi* ui) {
    SetSequence(state, MakeManualFiniteSequence(ui), "конечная последовательность");
    status = "Создана конечная LazySequence.\n" + BuildLengthStatus(state);
}

void ApplyCreateArithmeticSequence(LabState& state, std::string& status, TerminalUi* ui) {
    SetSequence(state, MakeArithmeticSequence(ui), "арифметическая прогрессия");
    status = "Создана арифметическая прогрессия.\n" + BuildLengthStatus(state);
}

void ApplyCreateFibonacciSequence(LabState& state, std::string& status, TerminalUi* ui) {
    SetSequence(state, MakeFibonacciSequence(ui), "последовательность Фибоначчи");
    status = "Создана последовательность Фибоначчи.\n" + BuildLengthStatus(state);
}

void ApplyCreateRandomWalkSequence(LabState& state, std::string& status, TerminalUi* ui) {
    SetSequence(state, MakeRandomWalkSequence(ui), "random walk");
    status = "Создан random walk.\n" + BuildLengthStatus(state);
}

void ApplyCreateSensorSequence(LabState& state, std::string& status, TerminalUi* ui) {
    SetSequence(state, MakeSensorSequence(ui), "псевдо-датчик с выбросами");
    status = "Создан псевдо-датчик с выбросами.\n" + BuildLengthStatus(state);
}

void ApplyShowPrefix(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    std::size_t count = ReadSize("Сколько элементов вывести: ", ui);
    status = "Первые " + std::to_string(count) + " элементов: " + FormatSequencePrefix(state.sequence, count);
    status += "\n" + BuildLengthStatus(state);
}

void ApplyGetByIndex(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    int index = ReadInt("Индекс: ", ui);
    status = "sequence[" + std::to_string(index) + "] = " + FormatDouble(state.sequence.Get(index), 6);
    status += "\n" + BuildLengthStatus(state);
}

void ApplyGetByOrdinal(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    Ordinal index = ReadOrdinal(ui);
    status = "sequence[" + index.ToString() + "] = " + FormatDouble(state.sequence.Get(index), 6);
    status += "\n" + BuildLengthStatus(state);
}

void ApplyShowLength(LabState& state, std::string& status) {
    RequireSequence(state);
    status = BuildLengthStatus(state);
}

void ApplyPrepend(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    double value = ReadDouble("Значение для вставки в начало: ", ui);
    state.sequence = state.sequence.Prepend(value);
    status = "Prepend выполнен.\n" + BuildLengthStatus(state);
}

void ApplyInsertAt(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    int index = ReadInt("Позиция вставки: ", ui);
    double value = ReadDouble("Значение: ", ui);
    state.sequence = state.sequence.InsertAt(value, index);
    status = "InsertAt выполнен.\n" + BuildLengthStatus(state);
}

void ApplyConcat(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    ArraySequence<double> tail = ParseNumbersLine(
        ReadText("Введите хвост через пробел/запятую: ", ui)
    );
    DoubleLazySequence suffix(&tail);
    state.sequence = state.sequence.Concat(suffix);
    status = "Concat выполнен.\n" + BuildLengthStatus(state);
}

void ApplyMap(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    double factor = ReadDouble("Множитель: ", ui);
    state.sequence = state.sequence.Map<double>([factor](double value) {
        return value * factor;
    });
    status = "Map выполнен: x * " + FormatDouble(factor, 6) + ".\n" + BuildLengthStatus(state);
}

void ApplyWhere(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    double threshold = ReadDouble("Порог threshold: ", ui);
    state.sequence = state.sequence.Where([threshold](double value) {
        return value >= threshold;
    });
    status = "Where выполнен: x >= " + FormatDouble(threshold, 6) + ".\n" + BuildLengthStatus(state);
}

void ApplyZip(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    ArraySequence<double> other = ParseNumbersLine(
        ReadText("Введите вторую последовательность через пробел/запятую: ", ui)
    );
    LazySequence<Tuple<double, double>> zipped = state.sequence.Zip(other);
    std::size_t count = ReadSize("Сколько пар показать: ", ui);
    status = "Zip, первые " + std::to_string(count) + " пар: " + FormatTuplePrefix(zipped, count);
}

void ApplySubsequence(LabState& state, std::string& status, TerminalUi* ui) {
    RequireSequence(state);
    int left = ReadInt("startIndex: ", ui);
    int right = ReadInt("endIndex: ", ui);
    state.sequence = state.sequence.GetSubsequence(left, right);
    state.sourceName = "подпоследовательность";
    status = "Подпоследовательность сохранена как текущая.\n" + BuildLengthStatus(state);
}

void ApplyStreamPreview(LabState& state, std::string& status, TerminalUi* ui) {
    EnsureOutputDirectories();
    ReadOnlyStream<double>* reader = CreateReadStreamFromUser(state, ui);

    try {
        std::size_t previewCount = ReadSize("Сколько элементов прочитать: ", ui);
        reader->Open();

        std::string values = "[";
        std::size_t read = 0;
        while (read < previewCount && !reader->IsEndOfStream()) {
            if (read > 0) {
                values += ", ";
            }
            values += FormatDouble(reader->Read());
            read++;
        }
        values += "]";

        status = "Прочитано элементов: " + std::to_string(read)
                 + "\nЗначения: " + values
                 + "\nПозиция потока: " + std::to_string(reader->GetPosition());
        state.lastStreamInfo = status;
        reader->Close();
        CloseAndDeleteReader(reader);
    } catch (...) {
        CloseAndDeleteReader(reader);
        throw;
    }
}

void ApplyStreamDump(LabState& state, std::string& status, TerminalUi* ui) {
    EnsureOutputDirectories();
    ReadOnlyStream<double>* reader = CreateReadStreamFromUser(state, ui);

    try {
        std::size_t copyCount = ReadSize("Сколько элементов скопировать в память и файл: ", ui);
        MemoryWriteStream<double> memoryWriter;
        FileWriteStream<double> fileWriter("output/stream_dump.txt", SerializeDouble);

        reader->Open();
        memoryWriter.Open();
        fileWriter.Open();

        std::size_t copied = Pump(*reader, memoryWriter, copyCount);
        for (int index = 0; index < memoryWriter.GetItems().GetLength(); index++) {
            fileWriter.Write(memoryWriter.GetItems().Get(index));
        }

        reader->Close();
        fileWriter.Close();
        status = "Скопировано элементов: " + std::to_string(copied)
                 + "\nДамп записан в output/stream_dump.txt";
        state.lastStreamInfo = status;
        CloseAndDeleteReader(reader);
    } catch (...) {
        CloseAndDeleteReader(reader);
        throw;
    }
}

void ApplyOnlineStatistics(LabState& state, std::string& status, TerminalUi* ui) {
    EnsureOutputDirectories();
    ReadOnlyStream<double>* reader = CreateReadStreamFromUser(state, ui);

    try {
        StatisticsRunSettings settings = ReadStatisticsRunSettings(ui);
        OnlineStatistics stats(settings.windowSize, settings.anomalyThreshold);
        FileWriteStream<std::string> csvWriter(
            "output/latest_stats.csv",
            [](const std::string& line) { return line; }
        );

        reader->Open();
        csvWriter.Open();
        csvWriter.Write("index,value,mean,median,min,max,variance,stddev,window_average,anomaly,zscore,anomaly_count");

        std::size_t processed = ProcessStatisticsRows(*reader, stats, csvWriter, settings.maxItems);

        reader->Close();
        csvWriter.Close();

        if (processed == 0) {
            status = "Поток оказался пустым.";
            state.lastStatisticsInfo = status;
            CloseAndDeleteReader(reader);
            return;
        }

        StatisticsSnapshot summary = stats.GetSnapshot();
        WriteStatisticsSummaryJson(summary);
        status = BuildStatisticsSuccessStatus(summary);
        state.lastStatisticsInfo = status;
        CloseAndDeleteReader(reader);
    } catch (...) {
        CloseAndDeleteReader(reader);
        throw;
    }
}

void RunTestsFromMenu(TerminalUi* ui) {
    if (ui != nullptr && ui->IsInteractive()) {
        ui->SuspendForStdIo();
        RunAllTests();
        std::cout << "\nНажмите Enter, чтобы вернуться в интерфейс...";
        std::string line;
        std::getline(std::cin, line);
        ui->ResumeFromStdIo();
        return;
    }

    RunAllTests();
}

bool HandleCreationChoice(int choice, LabState& state, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 1:
            ApplyCreateManualSequence(state, status, ui);
            return true;
        case 2:
            ApplyCreateArithmeticSequence(state, status, ui);
            return true;
        case 3:
            ApplyCreateFibonacciSequence(state, status, ui);
            return true;
        case 4:
            ApplyCreateRandomWalkSequence(state, status, ui);
            return true;
        case 5:
            ApplyCreateSensorSequence(state, status, ui);
            return true;
        default:
            return false;
    }
}

bool HandleLazyChoice(int choice, LabState& state, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 6:
            ApplyShowPrefix(state, status, ui);
            return true;
        case 7:
            ApplyGetByIndex(state, status, ui);
            return true;
        case 8:
            ApplyGetByOrdinal(state, status, ui);
            return true;
        case 9:
            ApplyShowLength(state, status);
            return true;
        case 10:
            ApplyPrepend(state, status, ui);
            return true;
        case 11:
            ApplyInsertAt(state, status, ui);
            return true;
        case 12:
            ApplyConcat(state, status, ui);
            return true;
        case 13:
            ApplyMap(state, status, ui);
            return true;
        case 14:
            ApplyWhere(state, status, ui);
            return true;
        case 15:
            ApplyZip(state, status, ui);
            return true;
        case 16:
            ApplySubsequence(state, status, ui);
            return true;
        default:
            return false;
    }
}

bool HandleStreamChoice(int choice, LabState& state, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 17:
            ApplyStreamPreview(state, status, ui);
            return true;
        case 18:
            ApplyStreamDump(state, status, ui);
            return true;
        case 19:
            ApplyOnlineStatistics(state, status, ui);
            return true;
        default:
            return false;
    }
}

bool HandleTestChoice(int choice, std::string& status, TerminalUi* ui) {
    if (choice == 20) {
        RunTestsFromMenu(ui);
        status = "Модульные тесты выполнены.";
        return true;
    }
    return false;
}

bool ExecuteChoice(int choice, LabState& state, std::string& status, TerminalUi* ui) {
    if (choice == 0) {
        return false;
    }
    if (HandleCreationChoice(choice, state, status, ui)) {
        return true;
    }
    if (HandleLazyChoice(choice, state, status, ui)) {
        return true;
    }
    if (HandleStreamChoice(choice, state, status, ui)) {
        return true;
    }
    if (HandleTestChoice(choice, status, ui)) {
        return true;
    }

    status = "Неверный выбор. Попробуйте снова.";
    return true;
}

bool HandleInteractiveKey(
    Key key,
    TerminalUi& ui,
    const std::vector<MenuItem>& items,
    int& selectedIndex,
    LabState& state,
    std::string& status
) {
    if (key == Key::Up) {
        selectedIndex = MoveSelection(items, selectedIndex, -1);
        ui.RenderMenu(items, selectedIndex, state, status);
        return true;
    }
    if (key == Key::Down) {
        selectedIndex = MoveSelection(items, selectedIndex, 1);
        ui.RenderMenu(items, selectedIndex, state, status);
        return true;
    }
    if (key == Key::Resize) {
        ui.RenderMenu(items, selectedIndex, state, status);
        return true;
    }
    if (key == Key::Quit) {
        return false;
    }
    if (key == Key::Enter) {
        const int choice = items[static_cast<std::size_t>(selectedIndex)].id;
        if (!ExecuteChoice(choice, state, status, &ui)) {
            return false;
        }
        ui.RenderMenu(items, selectedIndex, state, status);
    }
    return true;
}

void RenderInteractiveError(
    TerminalUi& ui,
    const std::vector<MenuItem>& items,
    int selectedIndex,
    const LabState& state,
    std::string& status,
    const std::string& message
) {
    status = "Ошибка: " + message;
    ui.RenderMenu(items, selectedIndex, state, status);
}

int RunInteractiveInterface() {
    TerminalUi ui;
    if (!ui.IsInteractive()) {
        return -1;
    }

    LabState state;
    std::string status = "Выберите действие стрелками и нажмите Enter.";
    const std::vector<MenuItem> items = BuildMenuItems();
    int selectedIndex = FindFirstSelectable(items);

    ui.RenderMenu(items, selectedIndex, state, status);

    while (true) {
        try {
            if (!HandleInteractiveKey(ui.ReadKey(), ui, items, selectedIndex, state, status)) {
                return 0;
            }
        } catch (const Exception& error) {
            RenderInteractiveError(ui, items, selectedIndex, state, status, error.what());
        } catch (const std::exception& error) {
            RenderInteractiveError(ui, items, selectedIndex, state, status, error.what());
        } catch (...) {
            RenderInteractiveError(ui, items, selectedIndex, state, status, "неизвестное исключение");
        }
    }
}

int RunNumericInterface() {
    LabState state;
    std::string status;

    std::cout << "\nЛабораторная работа №4 - LazySequence, Streams и онлайн-статистика" << std::endl;
    std::cout << "================================================================" << std::endl;

    while (true) {
        ShowNumericMenu(state);

        try {
            const int choice = ReadInt("");
            if (!ExecuteChoice(choice, state, status, nullptr)) {
                std::cout << "\nВыход из программы..." << std::endl;
                return 0;
            }

            if (!status.empty()) {
                std::cout << status << std::endl;
            }
        } catch (const InputError&) {
            std::cout << "\nВвод завершён. Выход из программы..." << std::endl;
            return 0;
        } catch (const Exception& error) {
            std::cout << "Ошибка: " << error.what() << std::endl;
        } catch (const std::exception& error) {
            std::cout << "Ошибка: " << error.what() << std::endl;
        } catch (...) {
            std::cout << "Ошибка: неизвестное исключение" << std::endl;
        }
    }
}

}  // namespace

int main() {
    setlocale(LC_ALL, "");
    EnsureOutputDirectories();

    const int interactiveResult = RunInteractiveInterface();
    if (interactiveResult != -1) {
        return interactiveResult;
    }

    return RunNumericInterface();
}
