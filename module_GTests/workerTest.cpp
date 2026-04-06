#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "Worker.h"

// 1. Проверка поведения при пустом списке файлов
TEST(WorkerSimpleTest, HandleEmptyFilePathsList) {
    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules = {{"R1", {"R1", RuleType::VALUE, ".*"}}};
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"R1"}}};

    Worker worker(sensors, rules, extractors);
    
    std::vector<std::string> paths; // Пустой список
    WorkerOutput out = worker.processFiles(paths);

    EXPECT_TRUE(out.empty());
    EXPECT_FALSE(worker.hasFatal());
}

// 2. Проверка поведения при передаче несуществующего файла
TEST(WorkerSimpleTest, HandleMissingFile) {
    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules = {{"R1", {"R1", RuleType::VALUE, ".*"}}};
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"R1"}}};

    Worker worker(sensors, rules, extractors);
    
    WorkerOutput out = worker.processFiles({"definitely_no_such_file_worker.txt"});

    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(worker.hasWarnings()); // Должен сработать коллбэк на невозможность открыть файл
}

// 3. Базовое извлечение обычного значения (VALUE)
TEST(WorkerExtractionTest, BasicValueExtraction) {
    std::string test_file = "basic_value.txt";
    std::ofstream out(test_file);
    out << "Sensor 1:\n";
    out << "    Temp: 42.5\n";
    out.close();

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules;
    rules.emplace("Temp", Rule{"Temp", RuleType::VALUE, "Temp:\\s*(.*)"});
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"Temp"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    ASSERT_EQ(result["S1"]["Temp"].size(), 1);
    EXPECT_DOUBLE_EQ(result["S1"]["Temp"][0].value, 42.5);
    EXPECT_EQ(result["S1"]["Temp"][0].filename, test_file);

    std::remove(test_file.c_str());
}

// 4. Проверка логики типа BOOL и функции trim (\r, пробелы, табы)
TEST(WorkerExtractionTest, BoolTypeWithWhitespaceTrimming) {
    std::string test_file = "bool_trim.txt";
    std::ofstream out(test_file);
    out << "Sensor 1:\n";
    // Имитируем грязные данные: пробелы до, пробелы после, \r как в Windows
    out << "State:  ON  \r\n"; 
    out << "State:\tOFF\t\n";
    out.close();

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules;
    rules.emplace("State", Rule{"State", RuleType::BOOL, "State:(.*)", "ON", "OFF"});
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"State"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    ASSERT_EQ(result["S1"]["State"].size(), 2);
    EXPECT_DOUBLE_EQ(result["S1"]["State"][0].value, 1.0); // "  ON  \r" должно очиститься до "ON"
    EXPECT_DOUBLE_EQ(result["S1"]["State"][1].value, 0.0); // "\tOFF\t" должно очиститься до "OFF"
    EXPECT_FALSE(worker.hasWarnings());

    std::remove(test_file.c_str());
}

// 5. Проверка типа SPEED и конвертации единиц измерения (строго по ТЗ)
TEST(WorkerExtractionTest, SpeedUnitConversions) {
    std::string test_file = "speed_conv.txt";
    std::ofstream out(test_file);
    out << "Network:\n";
    out << "Spd: 1.5 Gbit/s\n";
    out << "Spd: 500 Mbit/s\n";
    out << "Spd: 10 Kbit/s\n";
    out << "Spd: 8 bit/s\n";
    out.close();

    std::vector<Sensor> sensors = {{"NET", "Network"}};
    std::map<std::string, Rule> rules;
    
    // Регулярка теперь один-в-один как в ТЗ: (число) (единица)/s
    rules.emplace("Spd", Rule{"Spd", RuleType::SPEED, "Spd: (.*) (.*)/s"});
    
    std::map<std::string, std::vector<std::string>> extractors = {{"NET", {"Spd"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    ASSERT_EQ(result["NET"]["Spd"].size(), 4);
    EXPECT_DOUBLE_EQ(result["NET"]["Spd"][0].value, 1.5 * 1e9);
    EXPECT_DOUBLE_EQ(result["NET"]["Spd"][1].value, 500 * 1e6);
    EXPECT_DOUBLE_EQ(result["NET"]["Spd"][2].value, 10 * 1e3);
    EXPECT_DOUBLE_EQ(result["NET"]["Spd"][3].value, 8.0);

    std::remove(test_file.c_str());
}

// 7. Проверка на неизвестное состояние BOOL (ни true_repr, ни false_repr)
TEST(WorkerErrorHandlingTest, UnknownBoolState) {
    std::string test_file = "bad_bool_state.txt";
    std::ofstream out(test_file);
    out << "Sensor 1:\n";
    out << "State: BROKEN\n"; 
    out.close();

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules;
    rules.emplace("State", Rule{"State", RuleType::BOOL, "State: (.*)", "ON", "OFF"});
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"State"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    EXPECT_TRUE(worker.hasWarnings());
    ASSERT_EQ(result["S1"]["State"].size(), 1);
    EXPECT_DOUBLE_EQ(result["S1"]["State"][0].value, 0.0); 

    std::remove(test_file.c_str());
}

// 8. Проверка переключения контекста сенсоров в одном файле
TEST(WorkerDeepTest, MultipleSensorsContextSwitch) {
    std::string test_file = "context.txt";
    std::ofstream out(test_file);
    out << "Sensor 1:\n";
    out << "Temp: 10\n";
    out << "Sensor 2:\n";
    out << "Temp: 20\n";
    out << "Sensor 1:\n"; // Возврат к первому
    out << "Temp: 30\n";
    out.close();

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}, {"S2", "Sensor 2"}};
    std::map<std::string, Rule> rules;
    rules.emplace("Temp", Rule{"Temp", RuleType::VALUE, "Temp: (\\d+)"});
    std::map<std::string, std::vector<std::string>> extractors = {
        {"S1", {"Temp"}}, 
        {"S2", {"Temp"}}
    };

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    ASSERT_EQ(result["S1"]["Temp"].size(), 2);
    EXPECT_DOUBLE_EQ(result["S1"]["Temp"][0].value, 10.0);
    EXPECT_DOUBLE_EQ(result["S1"]["Temp"][1].value, 30.0);

    ASSERT_EQ(result["S2"]["Temp"].size(), 1);
    EXPECT_DOUBLE_EQ(result["S2"]["Temp"][0].value, 20.0);

    std::remove(test_file.c_str());
}

// 9. Стресс-тест многопоточности: слияние данных из нескольких файлов
TEST(WorkerConcurrencyTest, MergeMultipleFiles) {
    std::vector<std::string> files = {"f1.txt", "f2.txt", "f3.txt"};
    for (int i = 0; i < 3; ++i) {
        std::ofstream out(files[i]);
        out << "Sensor 1:\n";
        out << "Temp: " << (i + 1) * 10 << "\n"; // 10, 20, 30
        out.close();
    }

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules;
    rules.emplace("Temp", Rule{"Temp", RuleType::VALUE, "Temp: (\\d+)"});
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"Temp"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles(files);

    // Должно быть 3 записи. Так как потоки асинхронны, порядок может быть любым
    ASSERT_EQ(result["S1"]["Temp"].size(), 3);
    
    // Проверим сумму, чтобы не зависеть от порядка слияния векторов
    double sum = 0;
    for (const auto& res : result["S1"]["Temp"]) sum += res.value;
    EXPECT_DOUBLE_EQ(sum, 60.0); // 10 + 20 + 30

    for (const auto& f : files) std::remove(f.c_str());
}

// 10. Проверка работы с нераспознанными строками (мусором/комментариями)
TEST(WorkerResilienceTest, HandleGarbageStrings) {
    std::string test_file = "garbage.txt";
    std::ofstream out(test_file);
    out << "// Начало лога\n";
    out << "Some random text that makes no sense\n";
    out << "Sensor 1:\n";
    out << "Temp: 10\n";
    out << "Another random text inside context\n";
    out.close();

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules;
    rules.emplace("Temp", Rule{"Temp", RuleType::VALUE, "Temp: (\\d+)"});
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"Temp"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    // Из-за случайного текста Worker должен взвести варнинги, но не упасть
    EXPECT_TRUE(worker.hasWarnings());
    
    // Валидное значение всё равно должно быть извлечено
    ASSERT_EQ(result["S1"]["Temp"].size(), 1);
    EXPECT_DOUBLE_EQ(result["S1"]["Temp"][0].value, 10.0);

    std::remove(test_file.c_str());
}

// 11. Проверка поддержки научной нотации (1.2e-3)
// Важно, так как std::stod это поддерживает, и это может встретиться в логах
TEST(WorkerDeepTest, ScientificNotationSupport) {
    std::string test_file = "scientific.txt";
    std::ofstream out(test_file);
    out << "Sensor 1:\n";
    out << "Val: 1.23e2\n";  // 123
    out << "Val: 5.0e-1\n";  // 0.5
    out.close();

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules;
    rules.emplace("Val", Rule{"Val", RuleType::VALUE, "Val: (.*)"});
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"Val"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    ASSERT_EQ(result["S1"]["Val"].size(), 2);
    EXPECT_DOUBLE_EQ(result["S1"]["Val"][0].value, 123.0);
    EXPECT_DOUBLE_EQ(result["S1"]["Val"][1].value, 0.5);

    std::remove(test_file.c_str());
}

// 12. Проверка работы с кириллицей (UTF-8)
// В ТЗ указано "Датчик 1", поэтому Worker должен корректно находить контекст на русском
TEST(WorkerDeepTest, UTF8RussianContextSupport) {
    std::string test_file = "russian_utf8.txt";
    std::ofstream out(test_file);
    out << "Датчик Температуры:\n";
    out << "Значение: 36.6\n";
    out.close();

    // В коде используется std::string, который хранит UTF-8. std::regex обычно с этим справляется.
    std::vector<Sensor> sensors = {{"T_SENS", "Датчик Температуры"}};
    std::map<std::string, Rule> rules;
    rules.emplace("val", Rule{"val", RuleType::VALUE, "Значение: (.*)"});
    std::map<std::string, std::vector<std::string>> extractors = {{"T_SENS", {"val"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    ASSERT_EQ(result["T_SENS"]["val"].size(), 1);
    EXPECT_DOUBLE_EQ(result["T_SENS"]["val"][0].value, 36.6);

    std::remove(test_file.c_str());
}

// 14. Проверка пустых строк и строк из одних пробелов между данными
// Проверяет устойчивость цикла чтения line-by-line
TEST(WorkerResilienceTest, EmptyAndWhitespaceLines) {
    std::string test_file = "whitespaces.txt";
    std::ofstream out(test_file);
    out << "\n"; // Пустая строка в начале
    out << "Sensor 1:    \n";
    out << "   \n"; // Строка с пробелами
    out << "\t\t\n"; // Строка с табами
    out << "Val: 100\n";
    out << "\n\n";
    out.close();

    std::vector<Sensor> sensors = {{"S1", "Sensor 1"}};
    std::map<std::string, Rule> rules;
    rules.emplace("Val", Rule{"Val", RuleType::VALUE, "Val: (.*)"});
    std::map<std::string, std::vector<std::string>> extractors = {{"S1", {"Val"}}};

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    ASSERT_EQ(result["S1"]["Val"].size(), 1);
    EXPECT_DOUBLE_EQ(result["S1"]["Val"][0].value, 100.0);

    std::remove(test_file.c_str());
}

// 15. Проверка "жадного" переключения контекста
// Если имя одного датчика является подстрокой другого
TEST(WorkerDeepTest, SubstringSensorNames) {
    std::string test_file = "substrings.txt";
    std::ofstream out(test_file);
    out << "Sensor:\n"; // Короткое имя
    out << "Val: 10\n";
    out << "Sensor Extended:\n"; // Длинное имя, содержащее короткое
    out << "Val: 20\n";
    out.close();

    std::vector<Sensor> sensors = {
        {"S1", "Sensor"}, 
        {"S2", "Sensor Extended"}
    };
    std::map<std::string, Rule> rules;
    rules.emplace("Val", Rule{"Val", RuleType::VALUE, "Val: (\\d+)"});
    std::map<std::string, std::vector<std::string>> extractors = {
        {"S1", {"Val"}}, 
        {"S2", {"Val"}}
    };

    Worker worker(sensors, rules, extractors);
    WorkerOutput result = worker.processFiles({test_file});

    // Если логика поиска контекста правильная (ищет полное совпадение или по флагу), 
    // данные не должны перемешаться.
    EXPECT_DOUBLE_EQ(result["S1"]["Val"][0].value, 10.0);
    EXPECT_DOUBLE_EQ(result["S2"]["Val"][0].value, 20.0);

    std::remove(test_file.c_str());
}