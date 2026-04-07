#!/bin/bash
set -e

echo "GTESTS"
cd build && ./unit_tests

echo -e "Example from TechTask"
./main_app example/example.json example/data/ example/example.txt

echo -e "Stress test (100 files, same cfg)"
python3 example/fillScript.py
./main_app example/example.json example/data/ example/stress.txt

echo -e ""
echo "Example output"
cat example/example.txt

echo -e ""
echo "Stress output"
cat example/stress.txt