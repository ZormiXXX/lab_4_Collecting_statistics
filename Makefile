CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I./include

SRC_DIR = src
BUILD_DIR = build

MAIN_TARGET = $(BUILD_DIR)/lab4
TEST_TARGET = $(BUILD_DIR)/lab4_tests

MAIN_OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/tests.o
TEST_OBJECTS = $(BUILD_DIR)/tests.o $(BUILD_DIR)/tests_main.o

all: $(BUILD_DIR) $(MAIN_TARGET) $(TEST_TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(MAIN_TARGET): $(MAIN_OBJECTS)
	@echo "Линковка $(MAIN_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET): $(TEST_OBJECTS)
	@echo "Линковка $(TEST_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Компиляция $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(MAIN_TARGET)
	@echo "Запуск лабораторной работы №4..."
	./$(MAIN_TARGET)

test: $(TEST_TARGET)
	@echo "Запуск модульных тестов..."
	./$(TEST_TARGET)

clean:
	@echo "Очистка..."
	rm -rf $(BUILD_DIR)

rebuild: clean all

help:
	@echo "Доступные команды:"
	@echo "  make        - собрать приложение и тесты"
	@echo "  make run    - запустить интерактивный CLI"
	@echo "  make test   - прогнать модульные тесты"
	@echo "  make clean  - удалить build/"
	@echo "  make rebuild- пересобрать проект"

.PHONY: all run test clean rebuild help
