CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -Wextra -I./include
LDLIBS = -lncurses
VALGRIND = valgrind
VALGRIND_SUPPRESSIONS = valgrind.supp
VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --suppressions=$(VALGRIND_SUPPRESSIONS)

SRC_DIR = src
BUILD_DIR = build
INC_DIR = include

MAIN_TARGET = $(BUILD_DIR)/lab4
TEST_TARGET = $(BUILD_DIR)/lab4_tests

MAIN_OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/tests.o
TEST_OBJECTS = $(BUILD_DIR)/tests.o $(BUILD_DIR)/tests_main.o
OBJECTS = $(sort $(MAIN_OBJECTS) $(TEST_OBJECTS))
DEPFILES = $(OBJECTS:.o=.d)

all: $(BUILD_DIR) $(MAIN_TARGET) $(TEST_TARGET)

$(BUILD_DIR):
	@echo "Создание директории $(BUILD_DIR)..."
	@mkdir -p $(BUILD_DIR)

$(MAIN_TARGET): $(MAIN_OBJECTS)
	@echo "Линковка $(MAIN_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TARGET): $(TEST_OBJECTS)
	@echo "Линковка $(TEST_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Компиляция $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(MAIN_TARGET)
	@echo "Запуск полноэкранного интерфейса..."
	./$(MAIN_TARGET)

test: $(TEST_TARGET)
	@echo "Запуск тестов..."
	./$(TEST_TARGET)

leaks: $(TEST_TARGET)
	@echo "Проверка утечек памяти через leaks..."
	MallocStackLogging=1 leaks --atExit -- ./$(TEST_TARGET)

valgrind: $(TEST_TARGET)
	@echo "Проверка утечек памяти через Valgrind..."
	$(VALGRIND) $(VALGRIND_FLAGS) ./$(TEST_TARGET)

clean:
	@echo "Очистка..."
	rm -rf $(BUILD_DIR)

rebuild: clean all

help:
	@echo "Доступные команды:"
	@echo "  make        - Собрать основную программу и тесты"
	@echo "  make run    - Запустить полноэкранный интерфейс"
	@echo "  make test   - Запустить модульные тесты"
	@echo "  make leaks  - Проверить тесты через macOS leaks"
	@echo "  make valgrind - Проверить тесты через Valgrind"
	@echo "  make clean  - Удалить build/"
	@echo "  make rebuild- Пересобрать всё заново"

-include $(DEPFILES)

.PHONY: all run test leaks valgrind clean rebuild help
