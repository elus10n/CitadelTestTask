#include <gtest/gtest.h>
#include "aggregator.h" 

// 1. Проверка поведения при абсолютно пустых входных данных
TEST(AggregatorSimpleTest, HandleEmptyData) {
    Aggregator aggregator;
    WorkerOutput empty_input;
    
    AggregatorOutput result = aggregator.aggregate(empty_input);
    
    // Результат должен быть пустым, программа не должна упасть
    EXPECT_TRUE(result.empty());
}

// 2. Базовый поиск минимума и максимума
TEST(AggregatorSimpleTest, BasicMinMaxExtraction) {
    Aggregator aggregator;
    
    // Формируем входные данные вручную
    WorkerOutput input;
    input["Sensor1"]["Temp"] = {
        ExtractionResult("file1.txt", 10.5, "10.5"),
        ExtractionResult("file2.txt", 42.0, "42.0"),
        ExtractionResult("file3.txt", -5.0, "-5.0"),
        ExtractionResult("file4.txt", 20.0, "20.0")
    };
    
    AggregatorOutput result = aggregator.aggregate(input);
    
    // Проверяем наличие ключей
    ASSERT_EQ(result.count("Sensor1"), 1);
    ASSERT_EQ(result["Sensor1"].count("Temp"), 1);
    
    // Проверяем корректность min и max
    auto& stats = result["Sensor1"]["Temp"];
    EXPECT_DOUBLE_EQ(stats.min.value, -5.0);
    EXPECT_EQ(stats.min.filename, "file3.txt");
    
    EXPECT_DOUBLE_EQ(stats.max.value, 42.0);
    EXPECT_EQ(stats.max.filename, "file2.txt");
}

// 3. Поведение, когда в правиле всего одно значение
TEST(AggregatorSimpleTest, SingleValueRule) {
    Aggregator aggregator;
    
    WorkerOutput input;
    input["Sensor_Speed"]["Net"] = {
        ExtractionResult("log.txt", 100.0, "100 Mbit")
    };
    
    AggregatorOutput result = aggregator.aggregate(input);
    
    auto& stats = result["Sensor_Speed"]["Net"];
    
    // Максимум и минимум должны указывать на один и тот же элемент
    EXPECT_DOUBLE_EQ(stats.min.value, 100.0);
    EXPECT_DOUBLE_EQ(stats.max.value, 100.0);
    EXPECT_EQ(stats.min.filename, "log.txt");
    EXPECT_EQ(stats.max.filename, "log.txt");
}

// 4. Проверка работы с пустым вектором внутри правила (отказоустойчивость)
TEST(AggregatorErrorHandlingTest, EmptyVectorInRule) {
    Aggregator aggregator;
    
    int callback_calls = 0;
    aggregator.setCallback([&](const std::string& err) {
        callback_calls++;
    });

    WorkerOutput input;
    // Создаем ключ, но оставляем вектор пустым
    input["BadSensor"]["BadRule"] = std::vector<ExtractionResult>(); 
    
    AggregatorOutput result = aggregator.aggregate(input);
    
    auto& stats = result["BadSensor"]["BadRule"];
    
    // Проверяем, что сработала наша заглушка "NO_DATA", которую ты добавил в cpp
    EXPECT_EQ(stats.min.filename, "NO_DATA");
    EXPECT_EQ(stats.max.filename, "NO_DATA");
    
    // Коллбэк должен был вызваться
    EXPECT_EQ(callback_calls, 1);
}

// 5. Тестирование массива одинаковых значений (проверка стабильности std::minmax_element)
TEST(AggregatorDeepTest, IdenticalValues) {
    Aggregator aggregator;
    
    WorkerOutput input;
    input["BoolSensor"]["State"] = {
        ExtractionResult("f1.txt", 1.0, "включен"),
        ExtractionResult("f2.txt", 1.0, "ON"),
        ExtractionResult("f3.txt", 1.0, "1")
    };
    
    AggregatorOutput result = aggregator.aggregate(input);
    auto& stats = result["BoolSensor"]["State"];
    
    EXPECT_DOUBLE_EQ(stats.min.value, 1.0);
    EXPECT_DOUBLE_EQ(stats.max.value, 1.0);
    // std::minmax_element обычно возвращает первый элемент как min, и последний как max при равенстве
    EXPECT_EQ(stats.min.filename, "f1.txt"); 
}

// 6. Сложная структура: несколько сенсоров и правил одновременно
TEST(AggregatorDeepTest, ComplexStructure) {
    Aggregator aggregator;
    
    WorkerOutput input;
    // Сенсор 1
    input["S1"]["Temp"] = {
        ExtractionResult("f1.txt", 10.0, "10"),
        ExtractionResult("f2.txt", 20.0, "20")
    };
    input["S1"]["Press"] = {
        ExtractionResult("f1.txt", 700.0, "700"),
        ExtractionResult("f2.txt", 750.0, "750")
    };
    // Сенсор 2
    input["S2"]["Temp"] = {
        ExtractionResult("f1.txt", 100.0, "100"),
        ExtractionResult("f2.txt", 50.0, "50")
    };
    
    AggregatorOutput result = aggregator.aggregate(input);
    
    // Проверка независимости агрегации
    EXPECT_DOUBLE_EQ(result["S1"]["Temp"].max.value, 20.0);
    EXPECT_DOUBLE_EQ(result["S1"]["Press"].max.value, 750.0);
    
    EXPECT_DOUBLE_EQ(result["S2"]["Temp"].min.value, 50.0);
    EXPECT_DOUBLE_EQ(result["S2"]["Temp"].max.value, 100.0);
}

// 7. Работа с очень большими и очень маленькими числами (научная нотация)
TEST(AggregatorDeepTest, ExtremeValues) {
    Aggregator aggregator;
    
    WorkerOutput input;
    input["Science"]["Radiation"] = {
        ExtractionResult("f1.txt", 1e-9, "0.000000001"),
        ExtractionResult("f2.txt", 1e9, "1000000000"),
        ExtractionResult("f3.txt", 0.5, "0.5") // Добавим среднее для надежности
    };
    
    AggregatorOutput result = aggregator.aggregate(input);
    
    // 1. Сначала проверяем, что данные вообще дошли до аггрегатора
    ASSERT_EQ(result.count("Science"), 1);
    ASSERT_EQ(result["Science"].count("Radiation"), 1);

    auto& stats = result["Science"]["Radiation"];
    
    // 2. Используем EXPECT_NEAR для работы с очень маленькими числами
    // Погрешность 1e-12 будет достаточно точной для 1e-9
    EXPECT_NEAR(stats.min.value, 1e-9, 1e-12);
    EXPECT_NEAR(stats.max.value, 1e9, 1.0); // Для миллиарда погрешность в 1.0 допустима
}

// 8. Стресс-тест на большом количестве данных
TEST(AggregatorDeepTest, LargeDataStress) {
    Aggregator aggregator;
    WorkerOutput input;
    
    std::vector<ExtractionResult> large_vector;
    large_vector.reserve(100000);
    
    for(int i = 0; i < 100000; ++i) {
        // Значения от 0 до 99999
        large_vector.emplace_back("file" + std::to_string(i), static_cast<double>(i), "val");
    }
    // Добавим экстремумы в середину
    large_vector[50000] = ExtractionResult("min_file.txt", -9999.0, "-9999");
    large_vector[50001] = ExtractionResult("max_file.txt", 999999.0, "999999");
    
    input["StressSensor"]["Rule"] = std::move(large_vector);
    
    AggregatorOutput result = aggregator.aggregate(input);
    
    auto& stats = result["StressSensor"]["Rule"];
    EXPECT_DOUBLE_EQ(stats.min.value, -9999.0);
    EXPECT_EQ(stats.min.filename, "min_file.txt");
    
    EXPECT_DOUBLE_EQ(stats.max.value, 999999.0);
    EXPECT_EQ(stats.max.filename, "max_file.txt");
}