// main.cpp
// main.cpp
#include <SFML/Graphics.hpp>
#include <vector>
#include <queue>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>

static constexpr int GRID_SIZE = 40;
static constexpr int INF = std::numeric_limits<int>::max();
static constexpr int dx[4] = { 1, -1,  0,  0 };
static constexpr int dy[4] = { 0,  0,  1, -1 };
inline int idx(int x, int y) { return y * GRID_SIZE + x; }

enum class State { Picking, Searching, Animating, Error };

int main()
{
    std::srand(unsigned(std::time(nullptr)));

    std::vector<int> grid(GRID_SIZE * GRID_SIZE, 0);
    for (int i = 0, N = GRID_SIZE * GRID_SIZE / 3; i < N; ++i) {
        int x = std::rand() % GRID_SIZE;
        int y = std::rand() % GRID_SIZE;
        grid[idx(x, y)] = 1;
    }

    sf::RenderWindow window(sf::VideoMode(
        { 800, 800 }), "Dijkstra Animated (SFML 3.0.0)", sf::Style::Default
    );
    window.setFramerateLimit(60);

    sf::View view{ sf::FloatRect{sf::Vector2f{0.f,0.f}, sf::Vector2f{float(GRID_SIZE), float(GRID_SIZE)}} };
    view.setCenter(sf::Vector2f{ GRID_SIZE / 2.f, GRID_SIZE / 2.f });
    window.setView(view);

    sf::Texture floorTex, wallTex, pathTex, charTex;
    sf::Font    font;
    if (!floorTex.loadFromFile("floor.png") ||
        !wallTex.loadFromFile("wall.png") ||
        !pathTex.loadFromFile("path.png") ||
        !charTex.loadFromFile("man.png") ||
        !font.openFromFile("font.ttf"))
    {
        std::cerr << "Resource load failed\n";
        return 1;
    }

    sf::Sprite floorSprite(floorTex);
    floorSprite.setScale(sf::Vector2f{
        1.f / float(floorTex.getSize().x),
        1.f / float(floorTex.getSize().y) }
    );
    sf::Sprite wallSprite(wallTex);
    wallSprite.setScale(sf::Vector2f{
        1.f / float(wallTex.getSize().x),
        1.f / float(wallTex.getSize().y) }
    );
    sf::Sprite pathSprite(pathTex);
    pathSprite.setScale(sf::Vector2f{
        1.f / float(pathTex.getSize().x),
        1.f / float(pathTex.getSize().y) }
    );
    sf::Sprite character(charTex);
    character.setOrigin(sf::Vector2f{
        charTex.getSize().x / 2.f,
        charTex.getSize().y / 2.f }
    );
    character.setScale(sf::Vector2f{
        1.f / float(charTex.getSize().x),
        1.f / float(charTex.getSize().y) }
    );

    sf::Text fpsText{font, "", 12 };
    fpsText.setFillColor(sf::Color::White);
    sf::Clock frameClock;

    State state;
    state = State::Picking;
    std::vector<sf::Vector2i> picks;  
    std::vector<int> dist, prev, path;
    int startIdx = 0, goalIdx = 0;
    int pathIdx = 0;
    sf::Clock animClock;
    const float SEGMENT_TIME = 0.1f;

    while (window.isOpen())
    {
        while (auto ev = window.pollEvent())
        {
            if (ev->is<sf::Event::Closed>())
            {
                window.close(); break;
            }

            if (auto* rs = ev->getIf<sf::Event::Resized>())
            {
                float w = float(rs->size.x), h = float(rs->size.y);
                float wr = w / h, vr = 1.f; 
                float vpX = 0, vpY = 0, vpW = 1, vpH = 1;
                if (wr > vr) {
                    float s = vr / wr; vpX = (1 - s) / 2; vpW = s;
                }
                else {
                    float s = wr / vr; vpY = (1 - s) / 2; vpH = s;
                }
                view.setViewport({ sf::Vector2f{vpX,vpY},sf::Vector2f{vpW,vpH} });
                view.setCenter(sf::Vector2f{ GRID_SIZE / 2.f, GRID_SIZE / 2.f });
                window.setView(view);
            }

            if (state == State::Picking &&
                ev->is<sf::Event::MouseButtonPressed>())
            {
                if (auto* mb = ev->getIf<sf::Event::MouseButtonPressed>())
                {
                    if (mb->button == sf::Mouse::Button::Left)
                    {
                        sf::Vector2i pixel = sf::Mouse::getPosition(window);
                        sf::Vector2f world = window.mapPixelToCoords(pixel, view);
                        int gx = std::clamp(int(world.x), 0, GRID_SIZE - 1);
                        int gy = std::clamp(int(world.y), 0, GRID_SIZE - 1);
                        picks.emplace_back(gx, gy);
                        grid[idx(gx, gy)] = 0; 
                        if (picks.size() == 2)
                            state = State::Searching;
                    }
                }
            }

            if (state == State::Error &&
                (ev->is<sf::Event::MouseButtonPressed>() ||
                    ev->is<sf::Event::KeyPressed>()))
            {
                picks.clear();
                state = State::Picking;
            }
        }

        if (state == State::Searching)
        {
            int total = GRID_SIZE * GRID_SIZE;
            dist.assign(total, INF);
            prev.assign(total, -1);

            startIdx = idx(picks[0].x, picks[0].y);
            goalIdx = idx(picks[1].x, picks[1].y);
            dist[startIdx] = 0;

            using Node = std::pair<int, int>;
            auto cmp = [](auto& a, auto& b) { return a.first > b.first; };
            std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
            pq.push({ 0, startIdx });

            while (!pq.empty())
            {
                auto [d, u] = pq.top(); pq.pop();
                if (d > dist[u]) continue;
                if (u == goalIdx) break;

                int ux = u % GRID_SIZE, uy = u / GRID_SIZE;
                for (int k = 0; k < 4; ++k)
                {
                    int vx = ux + dx[k], vy = uy + dy[k];
                    if (vx < 0 || vy < 0 || vx >= GRID_SIZE || vy >= GRID_SIZE) continue;
                    int v = idx(vx, vy);
                    if (grid[v] == 1) continue;
                    if (dist[v] > d + 1)
                    {
                        dist[v] = d + 1;
                        prev[v] = u;
                        pq.push({ dist[v], v });
                    }
                }
            }

            if (dist[goalIdx] == INF)
            {
                std::cout << "No path! Click or press any key to retry.\n";
                state = State::Error;
            }
            else
            {
                path.clear();
                for (int u = goalIdx; u != -1; u = prev[u])
                    path.push_back(u);
                std::reverse(path.begin(), path.end());

                pathIdx = 0;
                animClock.restart();
                state = State::Animating;
            }
        }

        if (state == State::Animating)
        {
            float t = animClock.getElapsedTime().asSeconds() / SEGMENT_TIME;
            if (t >= 1.f && pathIdx + 1 < (int)path.size())
            {
                pathIdx++;
                animClock.restart();
                t = 0.f;
            }

            int ci = path[pathIdx];
            sf::Vector2f p0{ float(ci % GRID_SIZE) + 0.5f,
                             float(ci / GRID_SIZE) + 0.5f };

            if (pathIdx + 1 < (int)path.size())
            {
                int ni = path[pathIdx + 1];
                sf::Vector2f p1{ float(ni % GRID_SIZE) + 0.5f,
                                 float(ni / GRID_SIZE) + 0.5f };
                character.setPosition(p0 + (p1 - p0) * std::min(t, 1.f));
            }
            else
            {
                character.setPosition(p0);
            }

            view.setCenter(character.getPosition());
            window.setView(view);
        }

        window.clear();
        window.setView(view);

        for (int y = 0; y < GRID_SIZE; ++y)
        {
            for (int x = 0; x < GRID_SIZE; ++x)
            {
                int i = idx(x, y);
                if (grid[i] == 1)
                {
                    wallSprite.setPosition(sf::Vector2f{ float(x), float(y) });
                    window.draw(wallSprite);
                }
                else
                {
                    floorSprite.setPosition(sf::Vector2f{ float(x), float(y) });
                    window.draw(floorSprite);
                }

                if (state == State::Animating)
                {
                    for (int k = 0; k <= pathIdx && k < (int)path.size(); ++k)
                    {
                        if (path[k] == i)
                        {
                            pathSprite.setPosition(sf::Vector2f{ float(x), float(y) });
                            window.draw(pathSprite);
                            break;
                        }
                    }
                }

                if (state != State::Error && picks.size() > 0 &&
                    picks[0] == sf::Vector2i(x, y))
                {
                    sf::RectangleShape r({ 1.f,1.f });
                    r.setPosition(sf::Vector2f{ float(x), float(y) });
                    r.setFillColor({ 0,255,0,150 });
                    window.draw(r);
                }
                if (state != State::Error && picks.size() > 1 &&
                    picks[1] == sf::Vector2i(x, y))
                {
                    sf::RectangleShape r({ 1.f,1.f });
                    r.setPosition(sf::Vector2f{ float(x), float(y) });
                    r.setFillColor({ 0,0,255,150 });
                    window.draw(r);
                }
            }
        }

        if (state == State::Animating)
            window.draw(character);

        if (state == State::Error)
        {
            sf::RectangleShape over({ float(GRID_SIZE), float(GRID_SIZE) });
            over.setFillColor({ 255,0,0,100 });
            window.draw(over);
        }

        window.setView(window.getDefaultView());
        float ft = frameClock.restart().asSeconds();
        fpsText.setString("FPS: " + std::to_string(int(1.f / ft)));
        fpsText.setPosition(sf::Vector2f{ 5, 5 });
        window.draw(fpsText);

        window.display();
    }

    return 0;
}

