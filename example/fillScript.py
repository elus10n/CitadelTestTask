import os
import random

current_dir = os.path.dirname(os.path.abspath(__file__))
data_path = os.path.join(current_dir, "data")

if not os.path.exists(data_path):
    os.makedirs(data_path)

states = ["включен", "выключен", "завис", "unknown"]
units = ["bit", "Kbit", "Mbit", "Gbit", "ERR/s"]

for i in range(1, 101):
    content = ""
    num_entries = random.randint(15, 30)
    
    for j in range(num_entries):
        sensor_type = random.randint(1, 2)
        content += f"Датчик {sensor_type}:\n"
        
        speed = str(round(random.uniform(0.0, 999.9), 1))
        unit = random.choice(units)
        content += f"Скорость: {speed} {unit}/s\n"
        
        if sensor_type == 1:
            temp = str(round(random.uniform(0.0, 85.0), 1)) 
            content += f"Температура: {temp}\n\n"
        else:
            state = random.choice(states)
            content += f"Состояние: {state}\n\n"
            
    file_full_path = os.path.join(data_path, f"file{i}.txt")
    with open(file_full_path, "w", encoding="utf-8") as f:
        f.write(content)