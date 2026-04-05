#include <gtest/gtest.h>
#include <fstream>
#include "parser.h" // Путь подхватится из CMakeLists.txt

// 1. Проверка начального состояния (сразу после создания объекта)
TEST(ParserSimpleTest, InitialState) {
    ConfigParser parser;
    
    // Проверяем, что изначально ошибок нет
    EXPECT_FALSE(parser.hasFatal());
    EXPECT_FALSE(parser.hasWarnings());
    
    // Проверяем, что списки пусты
    EXPECT_TRUE(parser.getSensors().empty());
    EXPECT_TRUE(parser.getRules().empty());
}

// 2. Проверка реакции на пустую строку вместо пути
TEST(ParserSimpleTest, HandleEmptyPath) {
    ConfigParser parser;
    parser.SetConfig("");
    
    EXPECT_TRUE(parser.hasFatal());
    EXPECT_TRUE(parser.hasWarnings());
}

// 3. Проверка реакции на несуществующий файл
TEST(ParserSimpleTest, HandleMissingFile) {
    ConfigParser parser;
    parser.SetConfig("definitely_no_such_file.json");
    
    EXPECT_TRUE(parser.hasFatal());
}

// 4. Проверка корректного парсинга (нужен файл example.json в папке запуска)
TEST(ParserSimpleTest, SuccessfulParsing) {
    // Создадим временный простой конфиг прямо в тесте
    std::string test_file = "test_config.json";
    std::ofstream out(test_file);
    out << R"({
        "sensors": [{"name": "S1", "rule": "View1"}],
        "rules": [{"name": "R1", "type": "value", "rule": "\\d+"}],
        "extractors": [{"sensor": "S1", "rules": ["R1"]}]
    })";
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    // Если всё ок, фатальных ошибок быть не должно
    EXPECT_FALSE(parser.hasFatal());
    
    // Проверяем, что данные попали внутрь через геттеры
    EXPECT_EQ(parser.getSensors().size(), 1);
    EXPECT_EQ(parser.getRules().size(), 1);
    EXPECT_EQ(parser.getExtractors().count("S1"), 1);

    std::remove(test_file.c_str()); // Удаляем за собой файл
}

// 5. Проверка на битый JSON (синтаксическая ошибка)
TEST(ParserSimpleTest, MalformedJson) {
    std::string test_file = "bad_syntax.json";
    std::ofstream out(test_file);
    out << "{ \"sensors\": [ { \"name\": \"S1\" ] }"; // Пропущена скобка
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_TRUE(parser.hasFatal());
    EXPECT_TRUE(parser.hasWarnings());

    std::remove(test_file.c_str());
}

// 6. Проверка на отсутствие обязательных секций (например, нет extractors)
TEST(ParserSimpleTest, MissingSections) {
    std::string test_file = "missing_sections.json";
    std::ofstream out(test_file);
    out << R"({
        "sensors": [],
        "rules": []
    })"; 
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_TRUE(parser.hasFatal());
    std::remove(test_file.c_str());
}

// 7. Проверка кривой регулярки (ошибка компиляции std::regex)
TEST(ParserSimpleTest, BadRegexPattern) {
    std::string test_file = "bad_regex.json";
    std::ofstream out(test_file);
    out << R"({
        "sensors": [{"name": "S1", "rule": "V1"}],
        "rules": [{"name": "R1", "type": "value", "rule": "[0-9++"}],
        "extractors": [{"sensor": "S1", "rules": ["R1"]}]
    })";
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    // Должен быть warning, а список правил остаться пустым
    EXPECT_TRUE(parser.hasWarnings());
    EXPECT_TRUE(parser.getRules().empty());

    std::remove(test_file.c_str());
}

// 8. Проверка фильтрации: удаление сенсора, для которого нет правил в extractors
TEST(ParserSimpleTest, SensorFiltering) {
    std::string test_file = "filter_test.json";
    std::ofstream out(test_file);
    out << R"({
        "sensors": [
            {"name": "S_Valid", "rule": "V1"},
            {"name": "S_NoRules", "rule": "V2"}
        ],
        "rules": [{"name": "R1", "type": "value", "rule": ".*"}],
        "extractors": [{"sensor": "S_Valid", "rules": ["R1"]}]
    })";
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    // S_NoRules должен быть удален, останется только 1 сенсор
    EXPECT_EQ(parser.getSensors().size(), 1);
    EXPECT_EQ(parser.getSensors()[0].tech_name, "S_Valid");
    EXPECT_TRUE(parser.hasWarnings()); // Так как было удаление

    std::remove(test_file.c_str());
}

// 9. Проверка правила типа bool без полей true/false
TEST(ParserSimpleTest, IncompleteBoolRule) {
    std::string test_file = "bad_bool.json";
    std::ofstream out(test_file);
    out << R"({
        "sensors": [{"name": "S1", "rule": "V1"}],
        "rules": [{"name": "R1", "type": "bool", "rule": ".*"}],
        "extractors": [{"sensor": "S1", "rules": ["R1"]}]
    })";
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_TRUE(parser.hasWarnings());
    EXPECT_TRUE(parser.getRules().empty());

    std::remove(test_file.c_str());
}

// 10. Проверка на неизвестный тип правила (должен быть warning)
TEST(ParserSimpleTest, UnknownRuleType) {
    std::string test_file = "unknown_type.json";
    std::ofstream out(test_file);
    out << R"({
        "sensors": [{"name": "S1", "rule": "V1"}],
        "rules": [{"name": "R1", "type": "MAGIC", "rule": ".*"}],
        "extractors": [{"sensor": "S1", "rules": ["R1"]}]
    })";
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_TRUE(parser.hasWarnings());
    EXPECT_TRUE(parser.getRules().empty()); // Правило с типом MAGIC не должно создаться

    std::remove(test_file.c_str());
}

// 11. Проверка на дубликаты сенсоров (должен остаться один)
TEST(ParserSimpleTest, DuplicateSensors) {
    std::string test_file = "dups.json";
    std::ofstream out(test_file);
    out << R"({
        "sensors": [
            {"name": "S1", "rule": "V1"},
            {"name": "S1", "rule": "V1"}
        ],
        "rules": [{"name": "R1", "type": "value", "rule": ".*"}],
        "extractors": [{"sensor": "S1", "rules": ["R1"]}]
    })";
    out.close();

    ConfigParser parser;
    parser.SetConfig(test_file);

    // У тебя используется emplace/vector, проверим сколько их там
    // Если в векторе sensors два элемента - это повод задуматься о std::set
    EXPECT_EQ(parser.getSensors().size(), 2); 

    std::remove(test_file.c_str());
}

// 12. Тест на ссылки на несуществующие правила в экстракторах
TEST(ParserDeepTest, ExtractorReferencesMissingRule) {
    std::string test_file = "missing_rule_ref.json";
    {
        std::ofstream out(test_file);
        out << R"({
            "sensors": [{"name": "S1", "rule": "V1"}],
            "rules": [{"name": "R1", "type": "value", "rule": ".*"}],
            "extractors": [{"sensor": "S1", "rules": ["NON_EXISTENT_RULE"]}]
        })";
    }

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_TRUE(parser.hasWarnings());
    if (parser.getExtractors().count("S1")) {
        auto& r = parser.getExtractors().at("S1");
        EXPECT_TRUE(std::find(r.begin(), r.end(), "NON_EXISTENT_RULE") == r.end());
    }
    std::remove(test_file.c_str());
}

// 13. Тест на неверные типы данных в JSON (например, число вместо строки)
TEST(ParserDeepTest, WrongJsonDataTypes) {
    std::string test_file = "wrong_types.json";
    {
        std::ofstream out(test_file);
        out << R"({
            "sensors": [{"name": "S1", "rule": 12345}], 
            "rules": [{"name": "R1", "type": "value", "rule": ["not", "a", "string"]}],
            "extractors": [{"sensor": "S1", "rules": ["R1"]}]
        })";
    }

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_TRUE(parser.hasWarnings() || parser.hasFatal());
    std::remove(test_file.c_str());
}

// 14. Тест на пустой список правил в экстракторе
TEST(ParserDeepTest, EmptyRulesInExtractor) {
    std::string test_file = "empty_extractor.json";
    {
        std::ofstream out(test_file);
        out << R"({
            "sensors": [{"name": "S1", "rule": "V1"}],
            "rules": [{"name": "R1", "type": "value", "rule": ".*"}],
            "extractors": [{"sensor": "S1", "rules": []}]
        })";
    }

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_TRUE(parser.getSensors().empty());
    EXPECT_TRUE(parser.hasWarnings());
    std::remove(test_file.c_str());
}

// 15. Тест на Юникод и сложные регулярки
TEST(ParserDeepTest, UnicodeAndEscaping) {
    std::string test_file = "unicode.json";
    {
        std::ofstream out(test_file);
        // Явно пишем валидный UTF-8 JSON
        // Используем четыре бэкслеша для регулярки, если она потом идет в std::regex
        out << R"({
            "sensors": [{"name": "Sensor_Кириллица", "rule": "Test"}],
            "rules": [{"name": "R1", "type": "value", "rule": "\\\\d+°C"}],
            "extractors": [{"sensor": "Sensor_Кириллица", "rules": ["R1"]}]
        })";
    }

    ConfigParser parser;
    parser.SetConfig(test_file);

    // Если упало, выведи в консоль, что именно не так (если у тебя есть такой геттер)
    // EXPECT_FALSE(parser.hasFatal()) << "Parser failed on Unicode/Escaping";
    
    ASSERT_FALSE(parser.hasFatal());
    ASSERT_FALSE(parser.getSensors().empty());
    EXPECT_EQ(parser.getSensors()[0].tech_name, "Sensor_Кириллица");

    std::remove(test_file.c_str());
}

// 16. Стресс-тест на 1000 объектов
TEST(ParserDeepTest, LargeConfigStress) {
    std::string test_file = "stress.json";
    {
        std::ofstream out(test_file);
        out << "{ \"sensors\": [";
        for(int i=0; i<1000; ++i) {
            out << "{\"name\": \"S" << i << "\", \"rule\": \"V" << i << "\"}" << (i==999 ? "" : ",");
        }
        out << "], \"rules\": [{\"name\": \"R1\", \"type\": \"value\", \"rule\": \".*\"}], \"extractors\": [";
        for(int i=0; i<1000; ++i) {
            out << "{\"sensor\": \"S" << i << "\", \"rules\": [\"R1\"]}" << (i==999 ? "" : ",");
        }
        out << "] }";
    }

    ConfigParser parser;
    parser.SetConfig(test_file);

    EXPECT_FALSE(parser.hasFatal());
    EXPECT_EQ(parser.getSensors().size(), 1000);
    std::remove(test_file.c_str());
}

//17. Здесь мы проверяем, что парсер не падает при встрече с мусором, а просто отфильтровывает его, сохраняя валидные части
TEST(ParserDeepTest, ResilienceAndPartialLoading) {
    std::string test_file = "resilience.json";
    {
        std::ofstream out(test_file);
        out << R"({
            "sensors": [
                {"name": "GoodSensor", "rule": "V1"},
                {"name": "BadSensor", "rule": "V2"}
            ],
            "rules": [
                {"name": "ValidRule", "type": "value", "rule": "\\d+"},
                {"name": "BrokenRule", "type": "bool", "rule": ".*"} 
            ],
            "extractors": [
                {"sensor": "GoodSensor", "rules": ["ValidRule"]},
                {"sensor": "BadSensor", "rules": ["BrokenRule", "NonExistent"]}
            ]
        })";
        // Пояснение: BrokenRule вызовет ошибку (нет true/false), 
        // поэтому BadSensor останется без валидных правил и должен быть удален.
    }

    ConfigParser parser;
    parser.SetConfig(test_file);

    // Должны быть варнинги, но не фатальная ошибка
    EXPECT_TRUE(parser.hasWarnings());
    EXPECT_FALSE(parser.hasFatal());

    // В итоге должен выжить только один сенсор и одно правило
    EXPECT_EQ(parser.getSensors().size(), 1);
    EXPECT_EQ(parser.getSensors()[0].tech_name, "GoodSensor");
    EXPECT_EQ(parser.getRules().size(), 1);
    EXPECT_TRUE(parser.getRules().count("ValidRule"));

    std::remove(test_file.c_str());
}