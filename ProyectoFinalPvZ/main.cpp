#define SFML_DYNAMIC
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <iostream>
#include <string>

#pragma pack(push, 1)
struct Entidad {
    uint8_t  id;
    uint8_t  vida;
    uint8_t  dano;
    uint8_t  estado;
    uint8_t  filaY;
    uint8_t  padding;
    uint16_t posX;
    uint32_t reloj;
};
#pragma pack(pop)

extern "C" {
    void InicializarJuego();
    void ActualizarJuego();

    void AparecerZombie(uint32_t fila, uint32_t coordenadaX, uint32_t tipo);
    void AparecerZombieColgado();

    void IntentarColocarPlanta(uint32_t fila, uint32_t columna, uint32_t tipo);

    extern Entidad defensa[45];
    extern Entidad horda[30];
    extern Entidad disparos[50];

    extern uint32_t soles;
}

sf::Color colorPorID(uint8_t id) {
    switch (id) {
    case 1: return sf::Color(255, 220, 0);
    case 2: return sf::Color(0, 180, 0);
    case 3: return sf::Color(150, 100, 50);
    case 4: return sf::Color(180, 0, 180);
    case 5: return sf::Color(220, 0, 0);

    case 6: return sf::Color(100, 120, 100);
    case 7: return sf::Color(200, 120, 20);
    case 8: return sf::Color(180, 180, 180);
    case 9: return sf::Color(120, 80, 160);
    case 10: return sf::Color(80, 80, 80);

    case 20: return sf::Color(80, 255, 80);

    default: return sf::Color::White;
    }
}

std::string nombrePlanta(uint32_t id) {
    switch (id) {
    case 1: return "Girasol";
    case 2: return "Lanzaguisantes";
    case 3: return "Nuez";
    case 4: return "Carnivora";
    case 5: return "Cereza";
    default: return "Desconocida";
    }
}

//FALTAN NIVELES Y VISUAL

int main() {

    InicializarJuego();

    uint32_t plantaSeleccionada = 2;

    AparecerZombie(0, 790, 6);
    AparecerZombie(2, 790, 7);
    AparecerZombie(4, 790, 8);

    sf::RenderWindow ventana(
        sf::VideoMode(800, 600),
        "PvZ ASM/C++ - Partida funcional"
    );

    ventana.setFramerateLimit(60);

    sf::Font fuente;

    if (!fuente.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        std::cout << "No se pudo cargar la fuente\n";
        return -1;
    }

    sf::Text textoSoles;
    textoSoles.setFont(fuente);
    textoSoles.setCharacterSize(24);
    textoSoles.setFillColor(sf::Color::Yellow);
    textoSoles.setPosition(500.f, 10.f);

    sf::Text textoSeleccion;
    textoSeleccion.setFont(fuente);
    textoSeleccion.setCharacterSize(18);
    textoSeleccion.setFillColor(sf::Color::White);
    textoSeleccion.setPosition(10.f, 555.f);

    sf::Text textoAyuda;
    textoAyuda.setFont(fuente);
    textoAyuda.setCharacterSize(16);
    textoAyuda.setFillColor(sf::Color::White);
    textoAyuda.setPosition(500.f, 555.f);
    textoAyuda.setString("C = Zombie colgado | 6,7,8,9 = zombies");

    sf::Text simboloColgado;
    simboloColgado.setFont(fuente);
    simboloColgado.setCharacterSize(28);
    simboloColgado.setFillColor(sf::Color::White);
    simboloColgado.setString("!");

    sf::CircleShape figuraPlanta(22.f);
    figuraPlanta.setOrigin(22.f, 22.f);

    sf::CircleShape figuraZombie(20.f);
    figuraZombie.setOrigin(20.f, 20.f);

    sf::CircleShape figuraDisparo(8.f);
    figuraDisparo.setOrigin(8.f, 8.f);

    sf::RectangleShape cuadroBarra(sf::Vector2f(70.f, 40.f));

    while (ventana.isOpen()) {

        sf::Event evento;

        while (ventana.pollEvent(evento)) {

            if (evento.type == sf::Event::Closed) {
                ventana.close();
            }

            if (evento.type == sf::Event::KeyPressed) {

                if (evento.key.code == sf::Keyboard::C) {
                    AparecerZombieColgado();
                }

                if (evento.key.code == sf::Keyboard::Num6) {
                    AparecerZombie(0, 790, 6);
                }

                if (evento.key.code == sf::Keyboard::Num7) {
                    AparecerZombie(1, 790, 7);
                }

                if (evento.key.code == sf::Keyboard::Num8) {
                    AparecerZombie(2, 790, 8);
                }

                if (evento.key.code == sf::Keyboard::Num9) {
                    AparecerZombie(3, 790, 9);
                }
            }

            if (evento.type == sf::Event::MouseButtonPressed &&
                evento.mouseButton.button == sf::Mouse::Left) {

                int mouseX = evento.mouseButton.x;
                int mouseY = evento.mouseButton.y;

                if (mouseY >= 0 && mouseY < 50) {

                    int opcion = mouseX / 80;

                    if (opcion >= 0 && opcion < 5) {
                        plantaSeleccionada = static_cast<uint32_t>(opcion + 1);
                    }
                }
                else if (mouseY >= 50 &&
                    mouseY < 550 &&
                    mouseX >= 0 &&
                    mouseX < 720) {

                    uint32_t fila = static_cast<uint32_t>((mouseY - 50) / 100);
                    uint32_t columna = static_cast<uint32_t>(mouseX / 80);

                    if (fila < 5 && columna < 9) {
                        IntentarColocarPlanta(
                            fila,
                            columna,
                            plantaSeleccionada
                        );
                    }
                }
            }
        }

        ActualizarJuego();

        textoSoles.setString(
            "Soles: " + std::to_string(soles)
        );

        textoSeleccion.setString(
            "Seleccion: " + nombrePlanta(plantaSeleccionada)
        );

        ventana.clear(sf::Color(34, 139, 34));

        for (int i = 0; i < 5; i++) {

            uint8_t id = static_cast<uint8_t>(i + 1);

            cuadroBarra.setPosition(
                10.f + i * 80.f,
                5.f
            );

            cuadroBarra.setFillColor(
                colorPorID(id)
            );

            if (plantaSeleccionada == id) {

                cuadroBarra.setOutlineThickness(4.f);
                cuadroBarra.setOutlineColor(sf::Color::White);
            }
            else {

                cuadroBarra.setOutlineThickness(1.f);
                cuadroBarra.setOutlineColor(sf::Color::Black);
            }

            ventana.draw(cuadroBarra);
        }

        for (int i = 1; i < 5; i++) {

            sf::Vertex linea[] = {

                sf::Vertex(
                    sf::Vector2f(0.f, 50.f + (i * 100.f)),
                    sf::Color(50, 180, 50)
                ),

                sf::Vertex(
                    sf::Vector2f(800.f, 50.f + (i * 100.f)),
                    sf::Color(50, 180, 50)
                )
            };

            ventana.draw(linea, 2, sf::Lines);
        }

        for (int i = 1; i < 9; i++) {

            sf::Vertex linea[] = {

                sf::Vertex(
                    sf::Vector2f(i * 80.f, 50.f),
                    sf::Color(50, 180, 50)
                ),

                sf::Vertex(
                    sf::Vector2f(i * 80.f, 550.f),
                    sf::Color(50, 180, 50)
                )
            };

            ventana.draw(linea, 2, sf::Lines);
        }

        for (int i = 0; i < 45; i++) {

            if (defensa[i].id >= 1 &&
                defensa[i].id <= 5) {

                figuraPlanta.setFillColor(
                    colorPorID(defensa[i].id)
                );

                figuraPlanta.setPosition(
                    static_cast<float>(defensa[i].posX),
                    100.f + defensa[i].filaY * 100.f
                );

                ventana.draw(figuraPlanta);
            }
        }

        for (int i = 0; i < 50; i++) {

            if (disparos[i].id == 20) {

                figuraDisparo.setFillColor(
                    colorPorID(disparos[i].id)
                );

                figuraDisparo.setPosition(
                    static_cast<float>(disparos[i].posX),
                    100.f + disparos[i].filaY * 100.f
                );

                ventana.draw(figuraDisparo);
            }
        }

        for (int i = 0; i < 30; i++) {

            if (horda[i].id >= 6 &&
                horda[i].id <= 10) {

                figuraZombie.setFillColor(
                    colorPorID(horda[i].id)
                );

                float x =
                    static_cast<float>(horda[i].posX);

                float y =
                    100.f + horda[i].filaY * 100.f;

                if (horda[i].id == 10) {
                    y -= 35.f;
                }

                figuraZombie.setPosition(x, y);

                ventana.draw(figuraZombie);

                if (horda[i].id == 10) {

                    simboloColgado.setPosition(
                        x - 5.f,
                        y - 45.f
                    );

                    ventana.draw(simboloColgado);
                }
            }
        }

        ventana.draw(textoSoles);
        ventana.draw(textoSeleccion);
        ventana.draw(textoAyuda);

        ventana.display();
    }

    return 0;
}