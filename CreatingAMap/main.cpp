#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <sstream>

const int TILE_SIZE = 4;
const int MAP_WIDTH = 194;
const int MAP_HEIGHT = 194;

struct Agent {
    sf::CircleShape shape;
    sf::Vector2f position;
    sf::Vector2f velocity;

    Agent(float x, float y) {
        shape.setRadius(TILE_SIZE / 2);
        shape.setFillColor(sf::Color::Red);
        position.x = x;
        position.y = y;
        shape.setPosition(position);

        // Rastgele hýz
        velocity.x = (std::rand() % 5 - 2) / 2.0f;
        velocity.y = (std::rand() % 5 - 2) / 2.0f;
        if (velocity.x == 0 && velocity.y == 0) {
            velocity.x = 1.0f;
        }
    }

    void update(const std::vector<std::vector<bool>>& collisionMap) {
        // Yeni pozisyonu hesapla
        sf::Vector2f newPos = position + velocity;

        // Harita sýnýrlarýný kontrol et
        if (newPos.x < 0 || newPos.x >= MAP_WIDTH * TILE_SIZE ||
            newPos.y < 0 || newPos.y >= MAP_HEIGHT * TILE_SIZE) {
            velocity = -velocity;
            return;
        }

        // Çarpýþma kontrolü
        int tileX = static_cast<int>(newPos.x / TILE_SIZE);
        int tileY = static_cast<int>(newPos.y / TILE_SIZE);

        if (tileX >= 0 && tileX < MAP_WIDTH && tileY >= 0 && tileY < MAP_HEIGHT) {
            if (!collisionMap[tileY][tileX]) {
                position = newPos;
            }
            else {
                // Engelle karþýlaþtý, yön deðiþtir
                velocity = -velocity;

                // Alternatif olarak rastgele yeni yön verebiliriz
                velocity.x = (std::rand() % 5 - 2) / 2.0f;
                velocity.y = (std::rand() % 5 - 2) / 2.0f;
            }
        }

        shape.setPosition(position);
    }
};

bool loadMapFromFile(const std::string& filename,
    std::vector<std::vector<bool>>& collisionMap,
    std::vector<sf::RectangleShape>& tiles) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Dosya acilamadi: " << filename << std::endl;
        return false;
    }

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < MAP_HEIGHT) {
        for (int x = 0; x < line.size() && x < MAP_WIDTH; ++x) {
            char c = line[x];

            sf::RectangleShape tile(sf::Vector2f(TILE_SIZE, TILE_SIZE));
            tile.setPosition(x * TILE_SIZE, y * TILE_SIZE);

            if (c == '@') {
                tile.setFillColor(sf::Color::Blue);
                collisionMap[y][x] = true;
            }
            else if (c == 'T') {
                tile.setFillColor(sf::Color::Green);
                collisionMap[y][x] = false; // Yeþil alanlar geçilebilir
            }
            else {
                tile.setFillColor(sf::Color::White);
                collisionMap[y][x] = false;
            }

            tiles.push_back(tile);
        }
        y++;
    }

    // Eksik satýrlarý varsayýlan deðerlerle doldur
    while (y < MAP_HEIGHT) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            sf::RectangleShape tile(sf::Vector2f(TILE_SIZE, TILE_SIZE));
            tile.setPosition(x * TILE_SIZE, y * TILE_SIZE);
            tile.setFillColor(sf::Color::White);
            collisionMap[y][x] = false;
            tiles.push_back(tile);
        }
        y++;
    }

    return true;
}

int main() {
    std::srand(std::time(nullptr));

    // Pencere oluþtur
    sf::RenderWindow window(sf::VideoMode(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), "SFML Map with Agents");

    // Haritayý yükle
    std::vector<std::vector<bool>> collisionMap(MAP_HEIGHT, std::vector<bool>(MAP_WIDTH, false));
    std::vector<sf::RectangleShape> tiles;

    if (!loadMapFromFile("map.txt", collisionMap, tiles)) {
        std::cerr << "Harita yuklenirken hata olustu. Program kapatiliyor." << std::endl;
        return 1;
    }

    // Ajanlarý oluþtur
    std::vector<Agent> agents;
    for (int i = 0; i < 50; ++i) {
        // Rastgele geçerli bir pozisyon bul
        int x, y;
        do {
            x = std::rand() % MAP_WIDTH;
            y = std::rand() % MAP_HEIGHT;
        } while (collisionMap[y][x]);

        agents.emplace_back(x * TILE_SIZE, y * TILE_SIZE);
    }

    // Ana döngü
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Zamaný kontrol et (60 FPS için)
        if (clock.getElapsedTime().asMilliseconds() < 16) {
            continue;
        }
        clock.restart();

        // Ajanlarý güncelle
        for (auto& agent : agents) {
            agent.update(collisionMap);
        }

        // Çizim
        window.clear();

        // Haritayý çiz
        for (const auto& tile : tiles) {
            window.draw(tile);
        }

        // Ajanlarý çiz
        for (const auto& agent : agents) {
            window.draw(agent.shape);
        }

        window.display();
    }

    return 0;
}