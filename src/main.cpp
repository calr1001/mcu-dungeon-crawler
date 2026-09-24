// src/main.cpp
#include <SFML/Graphics.hpp>
#include "GameEngine.hpp"

class SFMLCanvas : public ICanvas {
private:
    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite;

public:
    SFMLCanvas() {
        m_image.create(GameEngine::VIRTUAL_WIDTH, GameEngine::VIRTUAL_HEIGHT, sf::Color::Black);
        m_texture.create(GameEngine::VIRTUAL_WIDTH, GameEngine::VIRTUAL_HEIGHT);
        m_sprite.setTexture(m_texture);
    }

    int GetWidth() const override { return GameEngine::VIRTUAL_WIDTH; }
    int GetHeight() const override { return GameEngine::VIRTUAL_HEIGHT; }

    void DrawPixel(int16_t x, int16_t y, Color565 color) override {
        if (x < 0 || x >= GameEngine::VIRTUAL_WIDTH || y < 0 || y >= GameEngine::VIRTUAL_HEIGHT) return;

        // Unpack RGB565 to RGB888 for SFML rendering
        uint8_t r = (color.value >> 11) & 0x1F;
        uint8_t g = (color.value >> 5) & 0x3F;
        uint8_t b = color.value & 0x1F;
        
        r = (r * 527 + 23) >> 6;
        g = (g * 259 + 33) >> 6;
        b = (b * 527 + 23) >> 6;

        m_image.setPixel(x, y, sf::Color(r, g, b));
    }

    void Clear(Color565 color) override {
        DrawRect(0, 0, GameEngine::VIRTUAL_WIDTH, GameEngine::VIRTUAL_HEIGHT, color);
    }

    void Present(sf::RenderWindow& window) {
        m_texture.update(m_image);
        // Scale 240x240 up 3x to 720x720 for comfortable viewing on modern monitors
        m_sprite.setScale(3.0f, 3.0f); 
        window.draw(m_sprite);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(720, 720), "MCU Dungeon Crawler - Desktop Simulator");
    window.setFramerateLimit(60);

    SFMLCanvas canvas;
    GameEngine engine;
    engine.Init(12345, 4); // Seed, Party Size = 4

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
        }

        InputState input = {};
        input.confirmPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Enter);
        input.upPressed      = sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        input.downPressed    = sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        input.leftPressed    = sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        input.rightPressed   = sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right);

        float dt = clock.restart().asSeconds();
        engine.Update(input, dt);

        window.clear();
        engine.Render(canvas);
        canvas.Present(window);
        window.display();
    }

    return 0;
}