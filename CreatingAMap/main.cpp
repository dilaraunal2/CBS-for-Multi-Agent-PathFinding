#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <memory>
#include <queue>
#include <unordered_map>
#include<unordered_set>
#include <algorithm>
#include <set>
#include <tuple>
#include <iomanip> 

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 900;

enum AppState { MENU, MAP_VIEW };
enum Algorithm { CBS, ICTS };

float calculateTileSize(int mapWidth, int mapHeight) {
    float tileWidth = static_cast<float>(WINDOW_WIDTH) / mapWidth;
    float tileHeight = static_cast<float>(WINDOW_HEIGHT) / mapHeight;
    return std::min(tileWidth, tileHeight);
}

struct Position {
    int x, y;
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(const Position& other) const {
        return std::tie(x, y) < std::tie(other.x, other.y);
    }
};

namespace std {
    template<>
    struct hash<Position> {
        size_t operator()(const Position& p) const {
            return p.x * 1812433253 + p.y;
        }
    };
}

struct PathNode {
    Position pos;
    int g, h;
    PathNode* parent;

    PathNode(Position p, int g, int h, PathNode* parent = nullptr)
        : pos(p), g(g), h(h), parent(parent) {}

    int f() const { return g + h; }
};

struct Agent {
    sf::CircleShape shape;
    Position currentPos;
    Position startPos;
    Position targetPos;
    std::vector<Position> path;
    size_t currentPathIndex;
    bool reachedTarget;
    float tileSize;
    sf::Color color;
    int id;

    Agent(int id, Position start, Position target, float tSize, sf::Color color)
        : id(id), startPos(start), currentPos(start), targetPos(target),
        tileSize(tSize), color(color), reachedTarget(false), currentPathIndex(0) {
        shape.setRadius(tileSize / 2.5f);
        shape.setFillColor(color);
        shape.setOutlineThickness(1);
        shape.setOutlineColor(sf::Color::Black);
    }

    void updatePosition(float offsetX, float offsetY) {
        shape.setPosition(offsetX + currentPos.x * tileSize + tileSize / 4,
            offsetY + currentPos.y * tileSize + tileSize / 4);
    }

    void moveAlongPath(const std::set<Position>& occupiedTargets) {
        if (reachedTarget || path.empty() || currentPathIndex >= path.size()) return;

        Position nextPos = path[currentPathIndex];

        // Eðer bir sonraki pozisyon baþka bir ajanýn hedef pozisyonuysa ve o ajan hedefine ulaþtýysa, bekle
        if (occupiedTargets.count(nextPos) > 0) {
            return; // Bu turda hareket etme, bekle
        }

        currentPos = nextPos;
        currentPathIndex++;

        if (currentPos == targetPos) {
            reachedTarget = true;
        }
    }
};

struct Target {
    sf::RectangleShape shape;
    Position pos;

    Target(Position p, float tileSize) : pos(p) {
        shape.setSize(sf::Vector2f(tileSize * 0.8f, tileSize * 0.8f));
        shape.setFillColor(sf::Color::Magenta);
        shape.setOutlineThickness(1);
        shape.setOutlineColor(sf::Color::Black);
    }

    void updatePosition(float offsetX, float offsetY, float tileSize) {
        shape.setPosition(offsetX + pos.x * tileSize + tileSize / 10,
            offsetY + pos.y * tileSize + tileSize / 10);
    }
};

struct MapData {
    std::vector<std::vector<bool>> collisionMap;
    std::vector<sf::RectangleShape> tiles;
    std::vector<Agent> agents;
    std::vector<Target> targets;
    int width, height;
    float tileSize;
    sf::Vector2f offset;
    sf::Clock timer;
    bool simulationRunning;
    bool allAgentsReached;
    float completionTime;
    Algorithm selectedAlgorithm;
    std::string mapName;
    std::set<Position> occupiedTargets; // Yeni: Ulaþýlmýþ hedef pozisyonlarý
};

// Artýk kullanmýyoruz, kaldýrýyoruz
// std::vector<std::vector<bool>> createDynamicCollisionMap(...) - REMOVED

std::vector<Position> findPath(const Position& start, const Position& target,
    const std::vector<std::vector<bool>>& collisionMap,
    int mapWidth, int mapHeight) {
    auto heuristic = [](const Position& a, const Position& b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
        };

    auto cmp = [](PathNode* a, PathNode* b) { return a->f() > b->f(); };
    std::priority_queue<PathNode*, std::vector<PathNode*>, decltype(cmp)> openSet(cmp);
    std::unordered_map<Position, PathNode*> allNodes;

    PathNode* startNode = new PathNode(start, 0, heuristic(start, target));
    openSet.push(startNode);
    allNodes[start] = startNode;

    const std::vector<Position> directions = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };

    while (!openSet.empty()) {
        PathNode* current = openSet.top();
        openSet.pop();

        if (current->pos == target) {
            std::vector<Position> path;
            while (current != nullptr) {
                path.push_back(current->pos);
                current = current->parent;
            }
            std::reverse(path.begin(), path.end());

            for (auto& pair : allNodes) {
                delete pair.second;
            }

            return path;
        }

        for (const auto& dir : directions) {
            Position neighbor = { current->pos.x + dir.x, current->pos.y + dir.y };

            if (neighbor.x < 0 || neighbor.x >= mapWidth ||
                neighbor.y < 0 || neighbor.y >= mapHeight ||
                collisionMap[neighbor.y][neighbor.x]) {
                continue;
            }

            int newG = current->g + 1;
            auto it = allNodes.find(neighbor);

            if (it == allNodes.end() || newG < it->second->g) {
                PathNode* neighborNode;
                if (it == allNodes.end()) {
                    neighborNode = new PathNode(neighbor, newG, heuristic(neighbor, target), current);
                    allNodes[neighbor] = neighborNode;
                    openSet.push(neighborNode);
                }
                else {
                    neighborNode = it->second;
                    neighborNode->g = newG;
                    neighborNode->parent = current;
                }
            }
        }
    }

    for (auto& pair : allNodes) {
        delete pair.second;
    }

    return {};
}


void findPathsWithCBS(std::vector<Agent>& agents, const std::vector<std::vector<bool>>& baseCollisionMap,
    int mapWidth, int mapHeight) {

    for (auto& agent : agents) {
        agent.path = findPath(agent.startPos, agent.targetPos, baseCollisionMap, mapWidth, mapHeight);
    }

    bool hasConflicts = true;
    int maxIterations = 1000; // ICTS için daha fazla iterasyon
    int iteration = 0;

    while (hasConflicts && iteration < maxIterations) {
        hasConflicts = false;

        for (size_t i = 0; i < agents.size(); ++i) {
            for (size_t j = i + 1; j < agents.size(); ++j) {
                size_t minLength = std::min(agents[i].path.size(), agents[j].path.size());

                for (size_t k = 0; k < minLength; ++k) {
                    if (agents[i].path[k] == agents[j].path[k]) {
                        hasConflicts = true;

                        auto tempCollisionMap = baseCollisionMap;
                        for (size_t m = 0; m < agents[i].path.size(); ++m) {
                            if (m < tempCollisionMap.size() && agents[i].path[m].x < tempCollisionMap[0].size()) {
                                tempCollisionMap[agents[i].path[m].y][agents[i].path[m].x] = true;
                            }
                        }

                        agents[j].path = findPath(agents[j].startPos, agents[j].targetPos,
                            tempCollisionMap, mapWidth, mapHeight);
                        break;
                    }

                    if (k > 0 && agents[i].path[k] == agents[j].path[k - 1] &&
                        agents[i].path[k - 1] == agents[j].path[k]) {
                        hasConflicts = true;

                        auto tempCollisionMap = baseCollisionMap;
                        for (size_t m = 0; m < agents[i].path.size(); ++m) {
                            if (m < tempCollisionMap.size() && agents[i].path[m].x < tempCollisionMap[0].size()) {
                                tempCollisionMap[agents[i].path[m].y][agents[i].path[m].x] = true;
                            }
                        }

                        agents[j].path = findPath(agents[j].startPos, agents[j].targetPos,
                            tempCollisionMap, mapWidth, mapHeight);
                        break;
                    }
                }

                if (hasConflicts) break;
            }
            if (hasConflicts) break;
        }

        iteration++;
    }
}

// ICTS Algorithm
struct ICTSNode {
    std::vector<int> costs;
    int totalCost;

    ICTSNode(const std::vector<int>& c) : costs(c) {
        totalCost = 0;
        for (int cost : costs) totalCost += cost;
    }

    bool operator<(const ICTSNode& other) const {
        return totalCost > other.totalCost; // For min-heap
    }
};

bool hasConflictsInPaths(const std::vector<std::vector<Position>>& paths) {
    size_t maxLength = 0;
    for (const auto& path : paths) {
        maxLength = std::max(maxLength, path.size());
    }

    // Check vertex conflicts and edge conflicts
    for (size_t t = 0; t < maxLength; ++t) {
        std::set<Position> occupiedPositions;

        for (size_t i = 0; i < paths.size(); ++i) {
            Position currentPos;
            if (t < paths[i].size()) {
                currentPos = paths[i][t];
            }
            else if (!paths[i].empty()) {
                currentPos = paths[i].back(); // Agent stays at goal
            }
            else {
                continue;
            }

            // Check vertex conflict
            if (occupiedPositions.count(currentPos)) {
                return true;
            }
            occupiedPositions.insert(currentPos);

            // Check edge conflict
            if (t > 0) {
                Position prevPos;
                if (t - 1 < paths[i].size()) {
                    prevPos = paths[i][t - 1];
                }
                else if (!paths[i].empty()) {
                    prevPos = paths[i].back();
                }
                else {
                    continue;
                }

                for (size_t j = 0; j < paths.size(); ++j) {
                    if (i == j) continue;

                    Position otherCurrent, otherPrev;
                    if (t < paths[j].size()) {
                        otherCurrent = paths[j][t];
                    }
                    else if (!paths[j].empty()) {
                        otherCurrent = paths[j].back();
                    }
                    else {
                        continue;
                    }

                    if (t - 1 < paths[j].size()) {
                        otherPrev = paths[j][t - 1];
                    }
                    else if (!paths[j].empty()) {
                        otherPrev = paths[j].back();
                    }
                    else {
                        continue;
                    }

                    // Edge conflict: agents swap positions
                    if (currentPos == otherPrev && prevPos == otherCurrent) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

std::vector<Position> findPathWithMaxCost(const Position& start, const Position& target,
    const std::vector<std::vector<bool>>& collisionMap,
    int mapWidth, int mapHeight, int maxCost) {

    // Eðer maxCost çok küçükse, minimum path'i döndür
    auto minPath = findPath(start, target, collisionMap, mapWidth, mapHeight);
    if (minPath.empty()) return {};

    int minCost = minPath.size() - 1;
    if (maxCost < minCost) return {}; // Ýmkansýz
    if (maxCost == minCost) return minPath; // Tam uyuyor

    // maxCost > minCost ise, path'i uzatmaya çalýþ
    auto extendedPath = minPath;
    int extraSteps = maxCost - minCost;

    // Basit uzatma: son pozisyonda kal
    for (int i = 0; i < extraSteps; i++) {
        extendedPath.push_back(target);
    }

    return extendedPath;
}

void findPathsWithICTS(std::vector<Agent>& agents, const std::vector<std::vector<bool>>& baseCollisionMap,
    int mapWidth, int mapHeight) {

    // Find minimum individual costs
    std::vector<int> minCosts(agents.size());
    for (size_t i = 0; i < agents.size(); ++i) {
        auto path = findPath(agents[i].startPos, agents[i].targetPos, baseCollisionMap, mapWidth, mapHeight);
        minCosts[i] = path.empty() ? 0 : path.size() - 1;
        std::cout << "Agent " << i << " min cost: " << minCosts[i] << std::endl; // Debug
    }

    std::priority_queue<ICTSNode> queue;
    queue.push(ICTSNode(minCosts));

    int maxIterations = 100;
    int iteration = 0;

    std::cout << "Starting ICTS iterations..." << std::endl; // Debug

    while (!queue.empty() && iteration < maxIterations) {
        ICTSNode current = queue.top();
        queue.pop();
        iteration++;

        std::cout << "ICTS iteration " << iteration << ", total cost: " << current.totalCost << std::endl; // Debug

        // Generate paths with current costs
        std::vector<std::vector<Position>> paths(agents.size());
        bool allPathsFound = true;

        for (size_t i = 0; i < agents.size(); ++i) {
            paths[i] = findPathWithMaxCost(agents[i].startPos, agents[i].targetPos,
                baseCollisionMap, mapWidth, mapHeight, current.costs[i]);
            if (paths[i].empty()) {
                std::cout << "Failed to find path for agent " << i << " with cost " << current.costs[i] << std::endl; // Debug
                allPathsFound = false;
                break;
            }
        }

        if (!allPathsFound) {
            std::cout << "Not all paths found, continuing..." << std::endl; // Debug
            continue;
        }

        // Check for conflicts
        if (!hasConflictsInPaths(paths)) {
            // Solution found!
            std::cout << "ICTS solution found after " << iteration << " iterations!" << std::endl; // Debug
            for (size_t i = 0; i < agents.size(); ++i) {
                agents[i].path = paths[i];
                std::cout << "Agent " << i << " path length: " << agents[i].path.size() << std::endl; // Debug
            }
            return;
        }

        std::cout << "Conflicts found, generating child nodes..." << std::endl; // Debug

        // Generate child nodes by increasing costs
        for (size_t i = 0; i < current.costs.size(); ++i) {
            auto newCosts = current.costs;
            newCosts[i]++;
            queue.push(ICTSNode(newCosts));
        }
    }

    std::cout << "ICTS failed after " << maxIterations << " iterations, falling back to CBS..." << std::endl; // Debug

    // Fallback to CBS if ICTS fails - FIX: Correct parameters
    findPathsWithCBS(agents, baseCollisionMap, mapWidth, mapHeight);
}

// Agent pozisyonlarýný dosyaya kaydet
void saveAgentPositions(const std::string& filename, const std::vector<Agent>& agents) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << agents.size() << std::endl;
        for (const auto& agent : agents) {
            file << agent.id << " "
                << agent.startPos.x << " " << agent.startPos.y << " "
                << agent.targetPos.x << " " << agent.targetPos.y << std::endl;
        }
        file.close();
        std::cout << "Agent positions saved to " << filename << std::endl;
    }
}

// Agent pozisyonlarýný dosyadan yükle
bool loadAgentPositions(const std::string& filename, std::vector<Agent>& agents,
    const std::vector<sf::Color>& agentColors, float tileSize) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    int numAgents;
    file >> numAgents;

    agents.clear();
    for (int i = 0; i < numAgents; i++) {
        int id, startX, startY, targetX, targetY;
        file >> id >> startX >> startY >> targetX >> targetY;

        Position start = { startX, startY };
        Position target = { targetX, targetY };

        agents.emplace_back(id, start, target, tileSize, agentColors[id % agentColors.size()]);
    }

    file.close();
    std::cout << "Agent positions loaded from " << filename << std::endl;
    return true;
}

void saveResultToFile(const std::string& mapName, Algorithm algorithm, float completionTime, int numAgents) {
    std::ofstream file("simulation_results.txt", std::ios::app);
    if (file.is_open()) {
        std::string algName = (algorithm == CBS) ? "CBS" : "ICTS";
        file << std::fixed << std::setprecision(4);
        file << mapName << "," << algName << "," << completionTime << "," << numAgents << std::endl;
        file.close();
        std::cout << "Result saved: " << mapName << " - " << algName << " - " << completionTime << "s" << std::endl;
    }
}

void initializeResultsFile() {
    // Check if file already exists
    std::ifstream checkFile("simulation_results.txt");
    bool fileExists = checkFile.good();
    checkFile.close();

    if (!fileExists) {
        // Only create header if file doesn't exist
        std::ofstream file("simulation_results.txt");
        if (file.is_open()) {
            file << "MapName,Algorithm,CompletionTime,NumAgents" << std::endl;
            file.close();
            std::cout << "Results file initialized with headers." << std::endl;
        }
    }
    else {
        std::cout << "Results file already exists, appending new results..." << std::endl;
    }
}

bool loadMapFromFile(const std::string& filename, MapData& mapData) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    mapData.collisionMap = std::vector<std::vector<bool>>(mapData.height, std::vector<bool>(mapData.width, false));
    mapData.tiles.clear();
    mapData.agents.clear();
    mapData.targets.clear();
    mapData.occupiedTargets.clear(); // Yeni: Ulaþýlmýþ hedefleri temizle

    size_t lastSlash = filename.find_last_of("/\\");
    size_t lastDot = filename.find_last_of(".");
    mapData.mapName = filename.substr(lastSlash + 1, lastDot - lastSlash - 1);

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < mapData.height) {
        for (int x = 0; x < line.size() && x < mapData.width; ++x) {
            char c = line[x];
            sf::RectangleShape tile(sf::Vector2f(mapData.tileSize, mapData.tileSize));
            tile.setPosition(mapData.offset.x + x * mapData.tileSize, mapData.offset.y + y * mapData.tileSize);
            if (c == '@') {
                tile.setFillColor(sf::Color::Black);
                mapData.collisionMap[y][x] = true;
            }
            else if (c == 'T') {
                tile.setFillColor(sf::Color::Green);
                mapData.collisionMap[y][x] = true;
            }
            else {
                tile.setFillColor(sf::Color::White);
                mapData.collisionMap[y][x] = false;
            }

            mapData.tiles.push_back(tile);
        }
        ++y;
    }

    while (y < mapData.height) {
        for (int x = 0; x < mapData.width; ++x) {
            sf::RectangleShape tile(sf::Vector2f(mapData.tileSize, mapData.tileSize));
            tile.setPosition(mapData.offset.x + x * mapData.tileSize, mapData.offset.y + y * mapData.tileSize);
            tile.setFillColor(sf::Color::White);
            mapData.tiles.push_back(tile);
        }
        ++y;
    }

    std::vector<sf::Color> agentColors = {
        sf::Color::Red, sf::Color::Green, sf::Color::Blue, sf::Color::Yellow,
        sf::Color::Cyan, sf::Color::Magenta, sf::Color(255, 165, 0),
        sf::Color(128, 0, 128), sf::Color(0, 128, 0), sf::Color(128, 128, 0)
    };

    // Agent pozisyonlarýný yüklemeye çalýþ
    std::string positionsFile = filename.substr(0, filename.find_last_of('.')) + "_positions.txt";

    if (!loadAgentPositions(positionsFile, mapData.agents, agentColors, mapData.tileSize)) {
        // Pozisyonlar dosyasý yoksa yeni pozisyonlar oluþtur
        std::cout << "No saved positions found, generating new positions..." << std::endl;

        for (int i = 0; i < 5; ++i) {
            Position start, target;
            int attempts = 0;

            do {
                start = { std::rand() % mapData.width, std::rand() % mapData.height };
                attempts++;
                if (attempts > 1000) break;
            } while (mapData.collisionMap[start.y][start.x]);

            attempts = 0;
            do {
                target = { std::rand() % mapData.width, std::rand() % mapData.height };
                attempts++;
                if (attempts > 1000) break;
            } while (mapData.collisionMap[target.y][target.x] ||
                (start.x == target.x && start.y == target.y));

            if (attempts <= 1000) {
                mapData.agents.emplace_back(i, start, target, mapData.tileSize, agentColors[i]);
            }
        }

        // Yeni oluþturulan pozisyonlarý kaydet
        saveAgentPositions(positionsFile, mapData.agents);
    }

    // Target'larý oluþtur
    for (const auto& agent : mapData.agents) {
        mapData.targets.emplace_back(agent.targetPos, mapData.tileSize);
    }

    // Use selected algorithm
    if (mapData.selectedAlgorithm == CBS) {
        std::cout << "Running CBS algorithm..." << std::endl;
        findPathsWithCBS(mapData.agents, mapData.collisionMap, mapData.width, mapData.height);
    }
    else {
        std::cout << "Running ICTS algorithm..." << std::endl;
        findPathsWithICTS(mapData.agents, mapData.collisionMap, mapData.width, mapData.height);
    }

    for (auto& agent : mapData.agents) {
        agent.updatePosition(mapData.offset.x, mapData.offset.y);
    }

    for (auto& target : mapData.targets) {
        target.updatePosition(mapData.offset.x, mapData.offset.y, mapData.tileSize);
    }

    mapData.simulationRunning = true;
    mapData.allAgentsReached = false;
    mapData.completionTime = 0.0f;
    mapData.timer.restart();

    return true;
}



int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    initializeResultsFile();
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "CBS & ICTS Multi-Agent Simulation");
    AppState state = MENU;
    Algorithm selectedAlgorithm = CBS;

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Failed to load font" << std::endl;
        return 1;
    }

   
    sf::Texture map1Texture, map2Texture, map3Texture;
    if (!map1Texture.loadFromFile("den520d.png") ||  
        !map2Texture.loadFromFile("ost003d.png") ||  
        !map3Texture.loadFromFile("brc202d.png")) {
        std::cerr << "Failed to load map preview images" << std::endl;
        return 1;
    }

    // Harita önizleme sprite'larý oluþtur
    sf::Sprite map1Sprite(map1Texture);
    sf::Sprite map2Sprite(map2Texture);
    sf::Sprite map3Sprite(map3Texture);

    // Boyutlarý ayarla ve konumlandýr
    float previewWidth = 300.f;
    float previewHeight = 200.f;

    map1Sprite.setPosition(550.f, 150.f);
    map1Sprite.setScale(previewWidth / map1Texture.getSize().x, previewHeight / map1Texture.getSize().y);

    map2Sprite.setPosition(550.f, 400.f);
    map2Sprite.setScale(previewWidth / map2Texture.getSize().x, previewHeight / map2Texture.getSize().y);

    map3Sprite.setPosition(550.f, 650.f);
    map3Sprite.setScale(previewWidth / map3Texture.getSize().x, previewHeight / map3Texture.getSize().y);

    // Buton arka planlarý
    sf::RectangleShape map1Bg(sf::Vector2f(320.f, 220.f));
    map1Bg.setPosition(540.f, 140.f);
    map1Bg.setFillColor(sf::Color(70, 70, 70));
    map1Bg.setOutlineThickness(2.f);
    map1Bg.setOutlineColor(sf::Color::White);

    sf::RectangleShape map2Bg(sf::Vector2f(320.f, 220.f));
    map2Bg.setPosition(540.f, 390.f);
    map2Bg.setFillColor(sf::Color(70, 70, 70));
    map2Bg.setOutlineThickness(2.f);
    map2Bg.setOutlineColor(sf::Color::White);

    sf::RectangleShape map3Bg(sf::Vector2f(320.f, 220.f));
    map3Bg.setPosition(540.f, 640.f);
    map3Bg.setFillColor(sf::Color(70, 70, 70));
    map3Bg.setOutlineThickness(2.f);
    map3Bg.setOutlineColor(sf::Color::White);

    // Algorithm selection buttons
    sf::RectangleShape cbsBtn(sf::Vector2f(120.f, 50.f));
    cbsBtn.setPosition(200.f, 300.f);
    cbsBtn.setFillColor(sf::Color::Green);
    cbsBtn.setOutlineThickness(2.f);
    cbsBtn.setOutlineColor(sf::Color::White);

    sf::RectangleShape ictsBtn(sf::Vector2f(120.f, 50.f));
    ictsBtn.setPosition(200.f, 370.f);
    ictsBtn.setFillColor(sf::Color(70, 70, 70));
    ictsBtn.setOutlineThickness(2.f);
    ictsBtn.setOutlineColor(sf::Color::White);

    // UI elemanlarý
    sf::Text title("Choose Algorithm and Map", font, 50);
    title.setPosition(WINDOW_WIDTH / 2 - title.getLocalBounds().width / 2, 50);
    title.setFillColor(sf::Color::White);

    sf::Text algorithmLabel("Algorithm:", font, 24);
    algorithmLabel.setPosition(200.f, 250.f);
    algorithmLabel.setFillColor(sf::Color::White);

    sf::Text cbsText("CBS", font, 20);
    cbsText.setPosition(240.f, 315.f);
    cbsText.setFillColor(sf::Color::White);

    sf::Text ictsText("ICTS", font, 20);
    ictsText.setPosition(235.f, 385.f);
    ictsText.setFillColor(sf::Color::White);

    sf::Text map1Btn("Map 1", font, 20);
    sf::Text map2Btn("Map 2", font, 20);
    sf::Text map3Btn("Map 3", font, 20);

    map1Btn.setPosition(550 + previewWidth / 2 - map1Btn.getLocalBounds().width / 2, 150 + previewHeight + 10);
    map2Btn.setPosition(550 + previewWidth / 2 - map2Btn.getLocalBounds().width / 2, 400 + previewHeight + 10);
    map3Btn.setPosition(550 + previewWidth / 2 - map3Btn.getLocalBounds().width / 2, 650 + previewHeight + 10);

    for (auto* t : { &map1Btn, &map2Btn, &map3Btn }) {
        t->setFillColor(sf::Color::White);
    }

    sf::Text backBtn("Geri", font, 24);
    backBtn.setPosition(20, 20);
    backBtn.setFillColor(sf::Color::Yellow);

    std::unique_ptr<MapData> currentMap;
    sf::Clock clock;
    sf::Text timerText("", font, 24);
    timerText.setPosition(20, 60);
    timerText.setFillColor(sf::Color::White);

    sf::Text algorithmText("", font, 20);
    algorithmText.setPosition(20, 100);
    algorithmText.setFillColor(sf::Color::Cyan);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                auto mousePos = sf::Mouse::getPosition(window);

                if (state == MENU) {
                    // Algorithm selection
                    if (cbsBtn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        selectedAlgorithm = CBS;
                        cbsBtn.setFillColor(sf::Color::Green);
                        ictsBtn.setFillColor(sf::Color(70, 70, 70));
                    }
                    else if (ictsBtn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        selectedAlgorithm = ICTS;
                        ictsBtn.setFillColor(sf::Color::Green);
                        cbsBtn.setFillColor(sf::Color(70, 70, 70));
                    }

                   
                    std::string filename;
                    int width = 0, height = 0;

                    if (map1Sprite.getGlobalBounds().contains(mousePos.x, mousePos.y) ||
                        map1Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        filename = "map.txt"; width = 256; height = 257;  
                    }
                    else if (map2Sprite.getGlobalBounds().contains(mousePos.x, mousePos.y) ||
                        map2Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        filename = "map2.txt"; width = 194; height = 194;   
                    }
                    else if (map3Sprite.getGlobalBounds().contains(mousePos.x, mousePos.y) ||
                        map3Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        filename = "map3.txt"; width = 530; height = 481;
                    }

                    if (!filename.empty()) {
                        currentMap = std::make_unique<MapData>();
                        currentMap->width = width;
                        currentMap->height = height;
                        currentMap->tileSize = calculateTileSize(width, height);
                        currentMap->selectedAlgorithm = selectedAlgorithm;
                        currentMap->offset = sf::Vector2f(
                            (WINDOW_WIDTH - width * currentMap->tileSize) / 2.0f,
                            (WINDOW_HEIGHT - height * currentMap->tileSize) / 2.0f
                        );
                        if (loadMapFromFile(filename, *currentMap)) {
                            state = MAP_VIEW;
                        }
                    }
                }
                else if (state == MAP_VIEW) {
                    if (backBtn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        state = MENU;
                    }
                }
            }
        }

        if (clock.getElapsedTime().asMilliseconds() < 100) continue;
        clock.restart();

        if (state == MAP_VIEW && currentMap && currentMap->simulationRunning) {
            bool allReached = true;

            // Önce ulaþýlmýþ hedefleri güncelle
            for (const auto& agent : currentMap->agents) {
                if (agent.reachedTarget) {
                    currentMap->occupiedTargets.insert(agent.targetPos);
                }
            }

            for (auto& agent : currentMap->agents) {
                if (!agent.reachedTarget) {
                    agent.moveAlongPath(currentMap->occupiedTargets);
                    agent.updatePosition(currentMap->offset.x, currentMap->offset.y);
                    allReached = false;
                }
            }

            if (allReached && !currentMap->allAgentsReached) {
                currentMap->simulationRunning = false;
                currentMap->allAgentsReached = true;
                currentMap->completionTime = currentMap->timer.getElapsedTime().asSeconds();

                // Save result to file
                saveResultToFile(currentMap->mapName, currentMap->selectedAlgorithm,
                    currentMap->completionTime, currentMap->agents.size());
            }
        }

        window.clear(sf::Color(50, 50, 50));

        if (state == MENU) {
            window.draw(title);

            // Algorithm selection
            window.draw(algorithmLabel);
            window.draw(cbsBtn);
            window.draw(ictsBtn);
            window.draw(cbsText);
            window.draw(ictsText);

            // Arka planlarý çiz
            window.draw(map1Bg);
            window.draw(map2Bg);
            window.draw(map3Bg);

            // Harita önizlemelerini çiz
            window.draw(map1Sprite);
            window.draw(map2Sprite);
            window.draw(map3Sprite);

            // Buton metinlerini çiz
            window.draw(map1Btn);
            window.draw(map2Btn);
            window.draw(map3Btn);
        }
        else if (state == MAP_VIEW && currentMap) {
            // Draw tiles
            for (const auto& tile : currentMap->tiles) window.draw(tile);

            // Draw targets
            for (const auto& target : currentMap->targets) window.draw(target.shape);

            // Draw agents
            for (const auto& agent : currentMap->agents) window.draw(agent.shape);

            // Draw back button
            window.draw(backBtn);

            // Draw algorithm info
            std::string algName = (currentMap->selectedAlgorithm == CBS) ? "CBS" : "ICTS";
            algorithmText.setString("Algorithm: " + algName);
            window.draw(algorithmText);

            // Draw timer
            if (currentMap->simulationRunning) {
                float seconds = currentMap->timer.getElapsedTime().asSeconds();
                timerText.setString("Time: " + std::to_string(seconds) + " s");
            }
            else if (currentMap->allAgentsReached) {
                timerText.setString("All agents reached to goal. Time: " +
                    std::to_string(currentMap->completionTime) + " s");
            }
            window.draw(timerText);
        }

        window.display();
    }

    return 0;
}