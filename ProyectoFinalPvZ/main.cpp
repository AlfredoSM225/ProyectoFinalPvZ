#define SFML_DYNAMIC
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <ctime>

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
    void ConfigurarNivel(uint32_t nivel);
    void ActualizarNivel();

    extern Entidad defensa[45];
    extern Entidad horda[30];
    extern Entidad disparos[50];

    extern uint32_t soles;
    extern uint32_t modoPrueba;
    extern uint32_t nivelActual;
    extern uint32_t contadorSpawn;
    extern uint32_t hordaFinalActiva;
    extern uint32_t nivelTerminado;
    extern uint32_t juegoTerminado;
}

struct Nivel {
    bool plantasDisponibles[5];

    int clasicos;
    int cono;
    int periodico;
    int jack;
    int colgado;

    int hordaClasicos;
    int hordaCono;
    int hordaPeriodico;
    int hordaJack;
    int hordaColgado;
};

Nivel niveles[5];

// ═══════════════════════════════════════════════════════════════
//  CONSTANTES DE LAYOUT
//  Toda la geometría de la grilla en un solo lugar
// ═══════════════════════════════════════════════════════════════
static const float BARRA_H = 50.f;   // alto de la barra de seeds
static const float CELDA_W = 80.f;   // ancho de cada columna
static const float CELDA_H = 100.f;  // alto de cada fila
static const float PLANTA_TAM = 80.f;   // tamaño target del sprite en campo
// (escala por ALTURA, no lado mayor)
static const float SEED_TAM = 42.f;   // tamaño target de seed en barra
static const float GUISANTE_TAM = 26.f; // tamaño del proyectil

static const uint32_t ID_PALA = 99;

// Centro X/Y de una celda (col, fila) en coordenadas de pantalla
static inline float celdaCX(int col) { return col * CELDA_W + CELDA_W * 0.5f; }
static inline float celdaCY(int fil) { return BARRA_H + fil * CELDA_H + CELDA_H * 0.5f; }

void InicializarNiveles() {
    niveles[0] = {
        { true, true, false, false, false },
        12, 0, 0, 0, 0,
        5, 0, 0, 0, 0
    };

    niveles[1] = {
        { true, true, false, true, false },
        14, 6, 0, 0, 0,
        4, 3, 0, 0, 0
    };

    niveles[2] = {
        { true, true, true, false, false },
        12, 8, 6, 0, 0,
        3, 4, 4, 0, 0
    };

    niveles[3] = {
        { true, true, true, false, true },
        10, 8, 8, 5, 0,
        0, 4, 5, 4, 0
    };

    niveles[4] = {
        { true, true, true, false, true },
        10, 10, 10, 6, 6,
        5, 5, 6, 4, 4
    };
}

bool PlantaDisponible(const Nivel& nivel, uint32_t idPlanta) {
    if (idPlanta < 1 || idPlanta > 5) return false;
    return nivel.plantasDisponibles[idPlanta - 1];
}

// ═══════════════════════════════════════════════════════════════
//  GESTOR DE TEXTURAS
// ═══════════════════════════════════════════════════════════════
struct GestorTexturas {

    sf::Texture girasol;
    sf::Texture lnzGst;
    sf::Texture nuez_[3];     // [0]=sana  [1]=media  [2]=critica
    sf::Texture cherry_[2];   // [0]=normal [1]=explosion
    sf::Texture chomper_[3];  // [0]=idle  [1]=atacando [2]=masticando
    sf::Texture guisante;

    sf::Texture seedGirasol;
    sf::Texture seedLnzGst;
    sf::Texture seedNuez;
    sf::Texture seedCereza;
    sf::Texture seedChomper;

    sf::Texture fallback;

    // ── Carga ───────────────────────────────────────────────────
    void cargar() {
        // Fallback magenta 32x32 — si aparece, falta un PNG
        sf::Image img; img.create(32, 32, sf::Color(255, 0, 255));
        fallback.loadFromImage(img);

        auto tryLoad = [&](sf::Texture& t, const std::string& path) {
            if (!t.loadFromFile(path)) {
                std::cout << "[WARN] Falta: " << path << "\n";
                t = fallback;
            }
            t.setSmooth(true);
            };

        // Campo
        tryLoad(girasol, "Sprites/Plantas/Girasol.png");
        tryLoad(lnzGst, "Sprites/Plantas/LnzGst.png");
        tryLoad(nuez_[0], "Sprites/Plantas/Nuez_1.png");   // vida alta  (estado 0)
        tryLoad(nuez_[1], "Sprites/Plantas/Nuez_2.png");   // vida media (estado 1)
        tryLoad(nuez_[2], "Sprites/Plantas/Nuez_3.png");   // vida baja  (estado 2)
        tryLoad(cherry_[0], "Sprites/Plantas/Cherry_1.png");
        tryLoad(cherry_[1], "Sprites/Plantas/Cherry_2.png");
        tryLoad(chomper_[0], "Sprites/Plantas/Chomper_1.png");
        tryLoad(chomper_[1], "Sprites/Plantas/Chomper_2.png");
        tryLoad(chomper_[2], "Sprites/Plantas/Chomper_3.png");

        // Proyectil
        tryLoad(guisante, "Sprites/Plantas/LnzGstShoot.png");

        // Seeds
        tryLoad(seedGirasol, "Sprites/Plantas/GirasolSeed.png");
        tryLoad(seedLnzGst, "Sprites/Plantas/LnzGstSeed.png");
        tryLoad(seedNuez, "Sprites/Plantas/NuezSeed.png");
        tryLoad(seedCereza, "Sprites/Plantas/CherrySeed.png");
        tryLoad(seedChomper, "Sprites/Plantas/ChomperSeed.png");
    }

    // ── Textura de campo ────────────────────────────────────────
    //  Para la Nuez: SOLO usamos 'vida' (0-100 según ASM).
    //  Para el resto: usamos 'estado' directamente.
    sf::Texture& texturaCampo(uint8_t id, uint8_t estado, uint8_t vida) {
        switch (id) {
        case 1: return girasol;
        case 2: return lnzGst;

        case 3: {
            // Nuez: deriva EXCLUSIVAMENTE de vida para evitar
            // que 'estado' incorrecto muestre sprite dañado.
            // Thresholds sobre escala 0-100:
            //   ≥ 67  → Nuez_1 (sana)
            //   34-66 → Nuez_2 (media)
            //   ≤ 33  → Nuez_3 (crítica)
            if (vida >= 20) return nuez_[0];
            else if (vida >= 10) return nuez_[1];
            else                 return nuez_[2];
        }

        case 4: {
            // Chomper: estado 0=idle, 1=atacando, 2=masticando
            uint8_t fase = (estado < 3) ? estado : 0;
            return chomper_[fase];
        }

        case 5: {
            // Cereza: estado 1 = explosión
            return (estado == 1) ? cherry_[1] : cherry_[0];
        }

        default: return fallback;
        }
    }

    sf::Texture& texturaSeed(uint32_t id) {
        switch (id) {
        case 1: return seedGirasol;
        case 2: return seedLnzGst;
        case 3: return seedNuez;
        case 4: return seedChomper;
        case 5: return seedCereza;
        default: return fallback;
        }
    }
};


// Escala manteniendo proporción, usando la ALTURA como referencia
void escalarPorAltura(sf::Sprite& spr, float targetH) {
    sf::FloatRect b = spr.getLocalBounds();
    if (b.height <= 0.f) return;
    float esc = targetH / b.height;
    spr.setScale(esc, esc);
}

// Escala por lado mayor (para seeds y proyectiles con canvas cuadrado)
void escalarPorMayor(sf::Sprite& spr, float target) {
    sf::FloatRect b = spr.getLocalBounds();
    float mayor = std::max(b.width, b.height);
    if (mayor <= 0.f) return;
    spr.setScale(target / mayor, target / mayor);
}

// Pone el origen en el centro del bounding box completo
void centrarOrigen(sf::Sprite& spr) {
    sf::FloatRect b = spr.getLocalBounds();
    spr.setOrigin(b.width * 0.5f, b.height * 0.5f);
}

// ═══════════════════════════════════════════════════════════════
//  Utilidades de color (fallback zombies + debug)
// ═══════════════════════════════════════════════════════════════
sf::Color colorPorID(uint8_t id) {
    switch (id) {
    case 6:  return sf::Color(100, 120, 100);
    case 7:  return sf::Color(200, 120, 20);
    case 8:  return sf::Color(180, 180, 180);
    case 9:  return sf::Color(120, 80, 160);
    case 10: return sf::Color(80, 80, 80);
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
    case 99: return "Pala";
    default: return "Desconocida";
    }
}

// ═══════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════
int main() {

    srand(static_cast<unsigned int>(time(nullptr)));

    InicializarNiveles();
    ConfigurarNivel(99);

    Nivel nivelEnJuego = niveles[0];

    uint32_t plantaSeleccionada = 2;
    int contadorCambioNivel = 0;

    GestorTexturas tex;
    tex.cargar();

    sf::RenderWindow ventana(sf::VideoMode(800, 600), "PvZ ASM/C++ - Con Sprites y Niveles");
    ventana.setFramerateLimit(60);

    sf::Font fuente;
    if (!fuente.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        std::cout << "No se pudo cargar la fuente\n";
        return -1;
    }

    // ── HUD ───────────────────────────────────────────────────
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
    textoAyuda.setCharacterSize(15);
    textoAyuda.setFillColor(sf::Color(200, 200, 200));
    textoAyuda.setPosition(400.f, 558.f);

    sf::Text textoNivel;
    textoNivel.setFont(fuente);
    textoNivel.setCharacterSize(18);
    textoNivel.setFillColor(sf::Color::White);
    textoNivel.setPosition(500.f, 35.f);

    sf::Text textoHorda;
    textoHorda.setFont(fuente);
    textoHorda.setCharacterSize(28);
    textoHorda.setFillColor(sf::Color::Red);
    textoHorda.setPosition(300.f, 8.f);

    sf::Text textoGameOver;
    textoGameOver.setFont(fuente);
    textoGameOver.setCharacterSize(42);
    textoGameOver.setFillColor(sf::Color::Red);
    textoGameOver.setString("GAME OVER");
    textoGameOver.setPosition(280.f, 250.f);

    sf::Text simboloColgado;
    simboloColgado.setFont(fuente);
    simboloColgado.setCharacterSize(28);
    simboloColgado.setFillColor(sf::Color::White);
    simboloColgado.setString("!");

    // ── Sprites reutilizables ─────────────────────────────────
    sf::Sprite sprPlanta;
    sf::Sprite sprDisparo;

    // ── Zombies: figuras de fallback ──────────────────────────
    sf::CircleShape figZombie(22.f);
    figZombie.setOrigin(22.f, 22.f);

    // ── Barra de seeds ────────────────────────────────────────
    sf::RectangleShape panelBarra(sf::Vector2f(800.f, BARRA_H));
    panelBarra.setFillColor(sf::Color(20, 20, 20, 200));
    panelBarra.setPosition(0.f, 0.f);

    sf::RectangleShape celdaSeed(sf::Vector2f(CELDA_W - 6.f, BARRA_H - 6.f));

    // ═══════════════════════════════════════════════════════════
    //  GAME LOOP
    // ═══════════════════════════════════════════════════════════
    while (ventana.isOpen()) {

        sf::Event evento;
        while (ventana.pollEvent(evento)) {

            if (evento.type == sf::Event::Closed)
                ventana.close();

            if (evento.type == sf::Event::KeyPressed) {
                switch (evento.key.code) {

                case sf::Keyboard::T:
                    ConfigurarNivel(99);
                    nivelEnJuego = niveles[0];
                    plantaSeleccionada = 2;
                    break;

                case sf::Keyboard::F1:
                    ConfigurarNivel(0);
                    nivelEnJuego = niveles[0];
                    plantaSeleccionada = 2;
                    break;

                case sf::Keyboard::F2:
                    ConfigurarNivel(1);
                    nivelEnJuego = niveles[1];
                    plantaSeleccionada = 2;
                    break;

                case sf::Keyboard::F3:
                    ConfigurarNivel(2);
                    nivelEnJuego = niveles[2];
                    plantaSeleccionada = 2;
                    break;

                case sf::Keyboard::F4:
                    ConfigurarNivel(3);
                    nivelEnJuego = niveles[3];
                    plantaSeleccionada = 2;
                    break;

                case sf::Keyboard::F5:
                    ConfigurarNivel(4);
                    nivelEnJuego = niveles[4];
                    plantaSeleccionada = 2;
                    break;

                case sf::Keyboard::C:
                    if (modoPrueba != 0)
                        AparecerZombieColgado();
                    break;

                case sf::Keyboard::Num6:
                    if (modoPrueba != 0)
                        AparecerZombie(rand() % 5, 790, 6);
                    break;

                case sf::Keyboard::Num7:
                    if (modoPrueba != 0)
                        AparecerZombie(rand() % 5, 790, 7);
                    break;

                case sf::Keyboard::Num8:
                    if (modoPrueba != 0)
                        AparecerZombie(rand() % 5, 790, 8);
                    break;

                case sf::Keyboard::Num9:
                    if (modoPrueba != 0)
                        AparecerZombie(rand() % 5, 790, 9);
                    break;

                default:
                    break;
                }
            }

            if (evento.type == sf::Event::MouseButtonPressed &&
                evento.mouseButton.button == sf::Mouse::Left) {

                int mx = evento.mouseButton.x;
                int my = evento.mouseButton.y;

                if (my >= 0 && my < (int)BARRA_H) {
                    int op = mx / (int)CELDA_W;

                    if (op >= 0 && op < 6) {
                        if (op == 5) {
                            plantaSeleccionada = ID_PALA;
                        }
                        else {
                            uint32_t posiblePlanta = (uint32_t)(op + 1);

                            if (modoPrueba != 0 || PlantaDisponible(nivelEnJuego, posiblePlanta)) {
                                plantaSeleccionada = posiblePlanta;
                            }
                        }
                    }
                }
                else if (my >= (int)BARRA_H && my < 550 && mx >= 0 && mx < 720) {
                    uint32_t fila = (uint32_t)((my - (int)BARRA_H) / (int)CELDA_H);
                    uint32_t columna = (uint32_t)(mx / (int)CELDA_W);

                    if (fila < 5 && columna < 9) {
                        if (modoPrueba != 0 || plantaSeleccionada == ID_PALA || PlantaDisponible(nivelEnJuego, plantaSeleccionada)) {
                            IntentarColocarPlanta(fila, columna, plantaSeleccionada);
                        }
                    }
                }
            }
        }

        if (nivelTerminado == 0 && juegoTerminado == 0) {
            ActualizarJuego();
            ActualizarNivel();
            contadorCambioNivel = 0;
        }
        else {

            contadorCambioNivel++;

            if (juegoTerminado != 0) {

                if (contadorCambioNivel >= 180) {

                    contadorCambioNivel = 0;

                    ConfigurarNivel(nivelActual);
                    nivelEnJuego = niveles[nivelActual];
                    plantaSeleccionada = 2;
                }
            }

            else if (modoPrueba == 0) {

                if (contadorCambioNivel >= 180) {

                    contadorCambioNivel = 0;

                    if (nivelActual < 4) {

                        ConfigurarNivel(nivelActual + 1);
                        nivelEnJuego = niveles[nivelActual];
                        plantaSeleccionada = 2;
                    }
                }
            }
        }

        textoSoles.setString("Soles: " + std::to_string(soles));
        textoSeleccion.setString("Sel: " + nombrePlanta(plantaSeleccionada));
        textoAyuda.setString("T=Prueba | F1-F5=Niveles | 6-9/C=Spawn manual en prueba");

        if (modoPrueba != 0) {
            textoNivel.setString("Modo prueba");
        }
        else {
            textoNivel.setString("Nivel: " + std::to_string(nivelActual + 1));
        }

        if (hordaFinalActiva != 0 && modoPrueba == 0 && nivelTerminado == 0) {
            textoHorda.setString("HORDA FINAL");
        }
        else if (nivelTerminado != 0) {
            if (modoPrueba == 0 && nivelActual == 4) {
                textoHorda.setString("JUEGO COMPLETADO");
            }
            else {
                textoHorda.setString("NIVEL COMPLETADO");
            }
        }
        else {
            textoHorda.setString("");
        }

        // ────────────────────────────────────────────────────────
        ventana.clear(sf::Color(34, 139, 34));

        // ── Grilla ──────────────────────────────────────────────
        for (int i = 1; i < 5; i++) {
            sf::Vertex h[] = {
                sf::Vertex(sf::Vector2f(0.f, BARRA_H + i * CELDA_H), sf::Color(50,180,50)),
                sf::Vertex(sf::Vector2f(800.f, BARRA_H + i * CELDA_H), sf::Color(50,180,50))
            };
            ventana.draw(h, 2, sf::Lines);
        }

        for (int i = 1; i < 9; i++) {
            sf::Vertex v[] = {
                sf::Vertex(sf::Vector2f(i * CELDA_W, BARRA_H), sf::Color(50,180,50)),
                sf::Vertex(sf::Vector2f(i * CELDA_W, 550.f),   sf::Color(50,180,50))
            };
            ventana.draw(v, 2, sf::Lines);
        }

        // ── Barra de seeds ──────────────────────────────────────
        ventana.draw(panelBarra);

        for (int i = 0; i < 6; i++) {
            uint32_t idSeed = (i == 5) ? ID_PALA : (uint32_t)(i + 1);
            bool disponible = (idSeed == ID_PALA) ? true : ((modoPrueba != 0) || PlantaDisponible(nivelEnJuego, idSeed));

            float cx = i * CELDA_W + CELDA_W * 0.5f;
            float cy = BARRA_H * 0.5f;

            // Fondo de celda
            celdaSeed.setPosition(i * CELDA_W + 3.f, 3.f);

            if (!disponible) {
                celdaSeed.setFillColor(sf::Color(20, 20, 20, 220));
                celdaSeed.setOutlineThickness(1.f);
                celdaSeed.setOutlineColor(sf::Color(40, 40, 40));
            }
            else if (plantaSeleccionada == idSeed) {
                celdaSeed.setFillColor(sf::Color(80, 70, 0, 200));
                celdaSeed.setOutlineThickness(2.f);
                celdaSeed.setOutlineColor(sf::Color(255, 220, 0));
            }
            else {
                celdaSeed.setFillColor(sf::Color(40, 40, 40, 180));
                celdaSeed.setOutlineThickness(1.f);
                celdaSeed.setOutlineColor(sf::Color(70, 70, 70));
            }

            ventana.draw(celdaSeed);

            if (idSeed == ID_PALA) {
                sf::Text textoPala;
                textoPala.setFont(fuente);
                textoPala.setCharacterSize(18);
                textoPala.setFillColor(sf::Color::White);
                textoPala.setString("Pala");
                textoPala.setPosition(i * CELDA_W + 18.f, 15.f);
                ventana.draw(textoPala);
            }
            else {
                // Sprite seed centrado en celda
                sprPlanta.setTexture(tex.texturaSeed(idSeed));
                escalarPorMayor(sprPlanta, SEED_TAM);
                centrarOrigen(sprPlanta);
                sprPlanta.setPosition(cx, cy);

                if (disponible) {
                    sprPlanta.setColor(sf::Color::White);
                }
                else {
                    sprPlanta.setColor(sf::Color(80, 80, 80, 180));
                }

                ventana.draw(sprPlanta);
            }
        }

        // ── Plantas en campo ────────────────────────────────────
        for (int i = 0; i < 45; i++) {
            uint8_t id = defensa[i].id;
            if (id < 1 || id > 5) continue;

            sf::Texture& t = tex.texturaCampo(id, defensa[i].estado, defensa[i].vida);

            sprPlanta.setTexture(t);
            escalarPorAltura(sprPlanta, PLANTA_TAM);
            centrarOrigen(sprPlanta);
            sprPlanta.setColor(sf::Color::White);

            float px = static_cast<float>(defensa[i].posX);
            float py = celdaCY(defensa[i].filaY);

            sprPlanta.setPosition(px, py);
            ventana.draw(sprPlanta);
        }

        // ── Proyectiles (guisantes) ─────────────────────────────
        sprDisparo.setTexture(tex.guisante);
        escalarPorMayor(sprDisparo, GUISANTE_TAM);
        centrarOrigen(sprDisparo);
        sprDisparo.setColor(sf::Color::White);

        for (int i = 0; i < 50; i++) {
            if (disparos[i].id != 20) continue;

            sprDisparo.setPosition(
                static_cast<float>(disparos[i].posX),
                celdaCY(disparos[i].filaY)
            );

            ventana.draw(sprDisparo);
        }

        // ── Zombies (formas de color — agregar sprites aquí cuando tengas los PNGs)
        for (int i = 0; i < 30; i++) {
            uint8_t id = horda[i].id;
            if (id < 6 || id > 10) continue;

            figZombie.setFillColor(colorPorID(id));

            float x = static_cast<float>(horda[i].posX);
            float y = celdaCY(horda[i].filaY);

            if (id == 10) y -= 35.f;

            figZombie.setPosition(x, y);
            ventana.draw(figZombie);

            if (id == 10) {
                simboloColgado.setPosition(x - 5.f, y - 50.f);
                ventana.draw(simboloColgado);
            }
        }

        // ── HUD ─────────────────────────────────────────────────
        ventana.draw(textoSoles);
        ventana.draw(textoSeleccion);
        ventana.draw(textoAyuda);
        ventana.draw(textoNivel);
        ventana.draw(textoHorda);

        if (juegoTerminado != 0) {
            ventana.draw(textoGameOver);
        }

        ventana.display();
    }

    return 0;
}