#include "ParticleSystem.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

float fmod(float a, float b)
{
    return a - b * (int)(a / b);
}

// get hue color bar
sf::Color getColor(float value, sf::Color min, sf::Color max, double luminosity) {
    float hue = fmod(value, 255);
    float ratio = hue / 255;
    sf::Color color;

    color.r = min.r + ratio * (max.r - min.r);
    color.g = min.g + ratio * (max.g - min.g);
    color.b = min.b + ratio * (max.b - min.b);
    color.a = luminosity;
    return color;
}

int main() {

    // initialisation de la fenêtre
    sf::RenderWindow window(sf::VideoMode(1200, 1200), "N-Body Simulation");
    sf::CircleShape shape(1.0f);
    // framerate
    window.setFramerateLimit(60);
    shape.setFillColor(sf::Color::White);

    // initialisation des particules
    ParticleSystem ps(NUM_PARTICLES);
    double luminosity = 100.0;

    // boucle principale
    int n = 0;
    float zoom = 8;
    float size = 100;
    float dt = 10.0;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.key.code == sf::Keyboard::Add)
                luminosity += 1;
            if (event.key.code == sf::Keyboard::Subtract)
                luminosity -= 1;
            if (event.key.code == sf::Keyboard::Up)
                dt *= 1.01;
            if (event.key.code == sf::Keyboard::Down)
                dt *= 0.99;
            if (event.key.code == sf::Keyboard::P)
                zoom *= 1.1;
            if (event.key.code == sf::Keyboard::M)
                zoom *= 0.9;
            luminosity = luminosity < 0 ? 0 : luminosity;
            luminosity = luminosity > 255 ? 255 : luminosity;
        }
        ps.update(dt);

        // affichage des particules
        window.clear();
        // for (int i = 0; i < NUM_PARTICLES; i++) {
        //     // printf("x: %f, y: %f\n", particle.position.x, particle.position.y);
        //     shape.setPosition((particles[i].getPosition().x + 50) * 8, (particles[i].getPosition().y + 50) * 8);
        //     window.draw(shape);
        // }

        // if (n++ % 10 == 0) {
            for (int i = 0; i < NUM_PARTICLES; i += 1) {
                Particle particle = ps.getParticles()[i];
                if (particle.getMass() > 100) {
                    shape.setFillColor(sf::Color::Red);
                    shape.setRadius(10);
                } else {
                    shape.setFillColor(getColor(particle.getPressure().norm(), sf::Color::Blue, sf::Color::Red, luminosity));
                    shape.setRadius(1);
                }
                shape.setOrigin((sf::Vector2f){shape.getRadius(), shape.getRadius()});
                shape.setPosition((particle.getPosition().x * zoom + window.getSize().x / 2), (particle.getPosition().y * zoom + window.getSize().y / 2));
                window.draw(shape);
            }
        window.display();
        // }
    }

    return 0;
}
