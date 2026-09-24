#include <SFML/Graphics.hpp>
#include "GameEngine.hpp"

class SFMLCanvas : public ICanvas {
private:
    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite;

public:
    SFMLCanvas() 
        : m_image(sf::Vector2u(GameEngine::VIRTUAL_WIDTH, GameEngine::VIRTUAL_HEIGHT), sf::Color::Black),
          m_texture(sf::Vector2u(GameEngine::VIRTUAL_WIDTH, GameEngine::VIRTUAL_HEIGHT)),
          m_sprite(m_texture) // Bind texture directly in member initializer list
    {
    }

    int GetWidth() const override { return GameEngine::VIRTUAL_WIDTH; }
    int GetHeight() const override { return GameEngine::VIRTUAL_HEIGHT; }

    void DrawPixel(int16_t x, int16_t y, Color565 color) override {
        if (x < 0 || x >= GameEngine::VIRTUAL_WIDTH || y < 0 || y >= GameEngine::VIRTUAL_HEIGHT) return;

        uint8_t r = (color.value >> 11) & 0x1F;
        uint8_t g = (color.value >> 5) & 0x3F;
        uint8_t b = color.value & 0x1F;
        
        r = (r * 527 + 23) >> 6;
        g = (g * 259 + 33) >> 6;
        b = (b * 527 + 23) >> 6;

        m_image.setPixel(sf::Vector2u(static_cast<uint32_t>(x), static_cast<uint32_t>(y)), sf::Color(r, g, b));
    }

    void Clear(Color565 color) override {
        DrawRect(0, 0, GameEngine::VIRTUAL_WIDTH, GameEngine::VIRTUAL_HEIGHT, color);
    }

    void Present(sf::RenderWindow& window) {
        m_texture.update(m_image);
        m_sprite.setScale(sf::Vector2f(3.0f, 3.0f)); 
        window.draw(m_sprite);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode({720, 720}), "MCU Dungeon Crawler - Desktop Simulator");
    window.setFramerateLimit(60);

    SFMLCanvas canvas;
    GameEngine engine;
    engine.Init(12345, 4);

    sf::Clock clock;

    while (window.isOpen()) {
        // SFML 3 pollEvent replacement loop
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        InputState input = {};
        input.confirmPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter);
        input.upPressed      = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
        input.downPressed    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
        input.leftPressed    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
        input.rightPressed   = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);

        float dt = clock.restart().asSeconds();
        engine.Update(input, dt);

        window.clear();
        engine.Render(canvas);
        canvas.Present(window);
        window.display();
    }

    return 0;
}