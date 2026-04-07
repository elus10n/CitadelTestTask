#include "Orchestrator.h"
#include "Logger.h"
#include <iostream>
#include <vector>
#include <filesystem>

int main(int argc, char** argv) 
{
    setlocale(LC_ALL, "ru_RU.UTF-8");

    std::string configPath = (argc > 1) ? argv[1] : "example/example.json";
    std::string dataDir = (argc > 2) ? argv[2] : "example/data/";
    std::string reportPath = (argc > 3) ? argv[3] : "example/report.txt";

    std::vector<std::string> filePaths;
    try 
    {
        for (const auto& entry : std::filesystem::directory_iterator(dataDir)) 
            if (entry.is_regular_file()) 
                filePaths.push_back(entry.path().string());
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
        return 1;
    }

    Orchestrator orch;
    orch.setCallback(Logger::getCallback(std::cerr));
    
    orch.launchParser(configPath, filePaths, reportPath);

    return 0;
}