#include <SFML/Graphics.hpp>
#include <vector>
#include <queue>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <algorithm>

static constexpr int GRID_SIZE = 40;
static constexpr int CELL_SIZE = 16;
static constexpr int WINDOW_SIZE = GRID_SIZE * CELL_SIZE;
static constexpr int INF = std::numeric_limits<int>::max();

static constexpr int dx[4] = { 1, -1,  0, 0 };
static constexpr int dy[4] = { 0,  0,  1, -1 };

inline int idx(int x, int y) { return y * GRID_SIZE + x; }

int main()
{
    std::srand(unsigned(std::time(nullptr)));

    std::vector<int> grid(GRID_SIZE * GRID_SIZE, 0);
    for (int i = 0, N = GRID_SIZE * GRID_SIZE / 3; i < N; ++i) {
        int x = std::rand() % GRID_SIZE;
        int y = std::rand() % GRID_SIZE;
        grid[idx(x, y)] = 1;
    }

    sf::RenderWindow window(sf::VideoMode({ WINDOW_SIZE, WINDOW_SIZE }), "Path", sf::Style::Default);
    window.setFramerateLimit(60);

    std::vector<sf::Vector2i> picks;
    while (window.isOpen() && picks.size() < 2)
    {
        while (auto ev = window.pollEvent())
        {
            if (ev->is<sf::Event::Closed>())
                window.close();

            if (auto* buttonpress = ev->getIf<sf::Event::MouseButtonPressed>())
            {
                
                sf::Vector2f wp = window.mapPixelToCoords({ buttonpress->position.x, buttonpress->position.y });
                int gx = std::clamp(int(wp.x / CELL_SIZE), 0, GRID_SIZE - 1);
                int gy = std::clamp(int(wp.y / CELL_SIZE), 0, GRID_SIZE - 1);
                picks.emplace_back(gx, gy);
                grid[idx(gx, gy)] = 0;
                
            }
        }

        window.clear();
        sf::RectangleShape cell({ CELL_SIZE - 1.0f, CELL_SIZE - 1.0f });
        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                int i = idx(x, y);
                if (picks.size() > 0 && sf::Vector2i(x, y) == picks[0])
                    cell.setFillColor(sf::Color::Green);
                else if (picks.size() > 1 && sf::Vector2i(x, y) == picks[1])
                    cell.setFillColor(sf::Color::Blue);
                else if (grid[i] == 1)
                    cell.setFillColor({ 50,50,50 });
                else
                    cell.setFillColor(sf::Color::White);

                cell.setPosition(sf::Vector2f{ float(x * CELL_SIZE), float(y * CELL_SIZE) });
                window.draw(cell);
            }
        }
        window.display();
    }

    if (picks.size() < 2)
        return 0;
    sf::Vector2i A = picks[0], B = picks[1];

    int total = GRID_SIZE * GRID_SIZE;
    std::vector<int> dist(total, INF), prev(total, -1);
    dist[idx(A.x, A.y)] = 0;

    using Node = std::pair<int, int>; 
    auto cmp = [](auto& a, auto& b) { return a.first > b.first; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
    pq.push({ 0, idx(A.x,A.y) });

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u]) continue;
        if (u == idx(B.x, B.y)) break;

        int ux = u % GRID_SIZE, uy = u / GRID_SIZE;
        for (int k = 0; k < 4; ++k) {
            int vx = ux + dx[k], vy = uy + dy[k];
            if (vx < 0 || vy < 0 || vx >= GRID_SIZE || vy >= GRID_SIZE) continue;
            int v = idx(vx, vy);
            if (grid[v] == 1) continue;
            if (dist[v] > d + 1) {
                dist[v] = d + 1;
                prev[v] = u;
                pq.push({ dist[v], v });
            }
        }
    }

    std::vector<bool> onPath(total, false);
    for (int cur = idx(B.x, B.y); cur != -1; cur = prev[cur])
        onPath[cur] = true;

    sf::RectangleShape cell({ CELL_SIZE - 1.0f, CELL_SIZE - 1.0f });
    while (window.isOpen())
    {
        while (auto ev = window.pollEvent())
            if (ev->is<sf::Event::Closed>())
                window.close();

        window.clear();
        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                int i = idx(x, y);
                if (sf::Vector2i(x, y) == A)
                    cell.setFillColor(sf::Color::Green);
                else if (sf::Vector2i(x, y) == B)
                    cell.setFillColor(sf::Color::Blue);
                else if (grid[i] == 1)
                    cell.setFillColor({ 50,50,50 });
                else if (onPath[i])
                    cell.setFillColor(sf::Color::Yellow);
                else
                    cell.setFillColor(sf::Color::White);

                cell.setPosition(sf::Vector2f{ float(x * CELL_SIZE), float(y * CELL_SIZE) });
                window.draw(cell);
            }
        }
        window.display();
    }

    return 0;
}

