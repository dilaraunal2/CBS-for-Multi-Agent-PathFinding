# Multi-Agent Path Finding Simulation

A visualization and comparison tool for Multi-Agent Path Finding (MAPF) algorithms, featuring two different pathfinding approaches with real-time simulation.

## 📋 Features

- **Two Algorithm Support:**
  - **CBS (Conflict-Based Search):** Conflict-based search algorithm
  - **ICTS (Increasing Cost Tree Search):** Increasing cost tree search algorithm

- **Visual Simulation:**
  - Real-time agent movement
  - Colorful agent and target representations
  - Multiple map size support

- **Performance Measurement:**
  - Simulation completion time tracking
  - Automatic result logging
  - Algorithm comparison metrics

- **Map Management:**
  - 3 different map templates
  - Custom map format (.txt)
  - Save and load agent positions

## 🚀 Installation

### Requirements

- C++ compiler (C++11 or higher)
- SFML 2.5+ library
- CMake (optional)

### SFML Installation

**Windows:**
```bash
# Using vcpkg
vcpkg install sfml

# Or download SFML manually:
# https://www.sfml-dev.org/download.php
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install libsfml-dev
```

**macOS:**
```bash
brew install sfml
```

### Building the Project

```bash
# Navigate to project folder
cd multi-agent-pathfinding

# Compile
g++ -std=c++11 main.cpp -o mapf_simulation -lsfml-graphics -lsfml-window -lsfml-system

# Or using CMake
mkdir build && cd build
cmake ..
make
```

## 📖 Usage

### Running the Program

```bash
./mapf_simulation
```

### Interface Guide

1. **Algorithm Selection:**
   - Click on CBS or ICTS button
   - Selected algorithm will be highlighted in green

2. **Map Selection:**
   - Choose one of the three map previews
   - Each map has different size and difficulty

3. **Watching the Simulation:**
   - Agents automatically move towards their targets
   - Elapsed time is shown in the top-left corner
   - Click "Back" button to return to menu

## 📁 Project Structure

```
multi-agent-pathfinding/
├── main.cpp                 # Main program file
├── map.txt                  # Map 1 (den520d - 256x257)
├── map2.txt                 # Map 2 (ost003d - 194x194)
├── map3.txt                 # Map 3 (brc202d - 530x481)
├── den520d.png              # Map 1 preview
├── ost003d.png              # Map 2 preview
├── brc202d.png              # Map 3 preview
├── arial.ttf                # Font file
├── simulation_results.txt   # Results log
└── README.md
```

## 🗺️ Map Format

Map files (.txt) contain the following characters:

- `@` - Obstacle (impassable area)
- `.` - Empty space (passable)
- `T` - Tree/Obstacle

### Example Map:

```
@@@@@@@@@@
@........@
@.@@.@@..@
@........@
@@@@@@@@@@
```

## 🤖 Agent Positions

Agent positions for each map are saved in `[map_name]_positions.txt`:

```
5                           # Number of agents
0 5 10 50 100              # ID startX startY targetX targetY
1 8 12 45 98
...
```

## 📊 Results Log

Simulation results are saved in CSV format in `simulation_results.txt`:

```csv
MapName,Algorithm,CompletionTime,NumAgents
den520d,CBS,12.3456,5
den520d,ICTS,11.8923,5
ost003d,CBS,8.7654,5
```

## 🧮 Algorithms

### CBS (Conflict-Based Search)

CBS is a two-level algorithm that detects conflicts and replans agent paths to resolve them:

1. **High Level:** Finds conflicts and adds constraints
2. **Low Level:** Finds paths for each agent respecting constraints

**Advantages:**
- Produces optimal solutions
- Strong conflict management

### ICTS (Increasing Cost Tree Search)

ICTS searches for conflict-free solutions by incrementally increasing individual agent path costs:

1. Starts with minimum individual costs
2. Increases costs if conflicts exist
3. Continues until finding a conflict-free solution

**Advantages:**
- Faster in certain scenarios
- Cost optimization

## ⚙️ Configuration

### Window Size

```cpp
const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 900;
```

### Number of Agents

Modifiable in `loadMapFromFile()` function:

```cpp
for (int i = 0; i < 5; ++i) {  // Change 5 to desired number
    // Agent creation code
}
```

### Agent Colors

```cpp
std::vector<sf::Color> agentColors = {
    sf::Color::Red, sf::Color::Green, sf::Color::Blue,
    // Add more colors as needed
};
```

## 🐛 Known Issues

- Performance may decrease on very large maps
- ICTS algorithm falls back to CBS in complex scenarios
- Program won't run if font file (arial.ttf) is missing

## 🔮 Future Improvements

- [ ] Class-based architecture (modular structure)
- [ ] A* algorithm optimization
- [ ] More algorithm support (EECBS, PBS)
- [ ] Custom map editor
- [ ] Dynamic agent count adjustment
- [ ] Statistics and graph panel
- [ ] Replay feature



## 👥 Contributing

1. Fork this repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📧 Contact

Feel free to open an issue for questions or suggestions.

## 🙏 Acknowledgments

- SFML library developers
- [MovingAI](https://movingai.com/benchmarks/) benchmark set for map files
- Multi-Agent Path Finding researchers

## 📚 Academic Background

This project is based on the seminal paper on Conflict-Based Search:

**Sharon, G., Stern, R., Felner, A., & Sturtevant, N. R. (2015).** *Conflict-based search for optimal multi-agent pathfinding.* Artificial Intelligence, 219, 40-66.

### Key Concepts from the Paper

**CBS Algorithm:**
- Two-level algorithm: high-level constraint tree search and low-level single-agent pathfinding
- Guarantees optimal solutions for multi-agent pathfinding
- Generates constraints only when conflicts are detected
- More efficient than traditional approaches in many scenarios

**Algorithm Performance:**
- Outperforms A* based approaches on many benchmarks
- Scalable to dozens of agents in complex environments
- Particularly effective in structured environments

⭐ If you like this project, don't forget to give it a star!
IMAGES FROM THE PROJECT
<img width="1919" height="1030" alt="image" src="https://github.com/user-attachments/assets/15c5bea4-8fd0-45d2-85b5-dbd1e4715861" />
<img width="1920" height="1021" alt="image" src="https://github.com/user-attachments/assets/0309408f-0473-49c5-9bbe-163f1c7c0661" />
