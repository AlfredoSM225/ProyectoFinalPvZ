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

// ── Estados (mismos valores que el ASM) ──────────────────────
static const uint8_t ESTADO_INACTIVO = 0;
static const uint8_t ESTADO_CAMINANDO = 1;
static const uint8_t ESTADO_ATACANDO = 2;
static const uint8_t ESTADO_QUIETO = 3;

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
// ═══════════════════════════════════════════════════════════════
static const float BARRA_H = 50.f;
static const float CELDA_W = 80.f;
static const float CELDA_H = 100.f;
static const float PLANTA_TAM = 80.f;
static const float ZOMBIE_TAM = 110.f;  // zombies mas altos que las plantas
static const float SEED_TAM = 60.f;
static const float GUISANTE_TAM = 26.f;

static const uint32_t ID_PALA = 99;

static inline float celdaCX(int col) { return col * CELDA_W + CELDA_W * 0.5f; }
static inline float celdaCY(int fil) { return BARRA_H + fil * CELDA_H + CELDA_H * 0.7f; }  // plantas
static inline float celdaCYZombie(int fil) { return BARRA_H + fil * CELDA_H + CELDA_H * 0.4f; }  // zombies

void InicializarNiveles() {
    niveles[0] = {
        { false, true, false, false, false },
        12, 0, 0, 0, 0,
        5, 0, 0, 0, 0
    };

    niveles[1] = {
        { true, true, false, false, false },
        14, 6, 0, 0, 0,
        4, 3, 0, 0, 0
    };

    niveles[2] = {
        { true, true, false, true, false },
        12, 8, 6, 0, 0,
        3, 4, 4, 0, 0
    };

    niveles[3] = {
        { true, true, true, true, false },
        10, 8, 8, 5, 0,
        0, 4, 5, 4, 0
    };

    niveles[4] = {
        { true, true, true, true, true },
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

    // ── Plantas ──────────────────────────────────────────────
    sf::Texture girasol;
    sf::Texture lnzGst;
    sf::Texture nuez_[3];     // [0]=sana  [1]=media  [2]=critica
    sf::Texture cherry_[2];   // [0]=normal [1]=explosion
    sf::Texture chomper_[3];  // [0]=idle  [1]=atacando [2]=cooldown
    sf::Texture guisante;

    sf::Texture seedGirasol;
    sf::Texture seedLnzGst;
    sf::Texture seedNuez;
    sf::Texture seedCereza;
    sf::Texture seedChomper;
    sf::Texture pala;

    // ── Zombies ──────────────────────────────────────────────
    sf::Texture zmb_[2];        // clasico: [0]=walk1  [1]=walk2
    sf::Texture zmbAtk_[2];     // clasico: [0]=atk1   [1]=atk2
    sf::Texture zmbDead_;       // clasico: muerto

    sf::Texture zmbCono_[2];    // cono: walk
    sf::Texture zmbConoAtk_[2]; // cono: atk
    sf::Texture zmbConoDead_;   // cono: muerto

    sf::Texture zmbJack_[2];    // jack: walk
    sf::Texture zmbJackAtk_[2]; // jack: atk
    sf::Texture zmbJackDead_;   // jack: muerto

    sf::Texture zmpPer_[2];     // periodico: walk normal (vida > 15)
    sf::Texture zmpPerAtk_[2];  // periodico: atk
    sf::Texture zmpPerDead_;    // periodico: muerto
    sf::Texture zmpPerNP_[2];   // periodico: walk rapido sin periodico (vida <= 15)

    sf::Texture zomC_[3];       // colgado: [0]=ZomC_1 [1]=ZomC_2 [2]=ZomC_3
    sf::Texture cuerda_;        // cuerda del colgado

    sf::Texture fallback;

    // ── Carga ───────────────────────────────────────────────────
    void cargar() {
        // Fallback magenta 32x32
        sf::Image img; img.create(32, 32, sf::Color(255, 0, 255));
        fallback.loadFromImage(img);

        auto tryLoad = [&](sf::Texture& t, const std::string& path) {
            if (!t.loadFromFile(path)) {
                std::cout << "[WARN] Falta: " << path << "\n";
                t = fallback;
            }
            t.setSmooth(false);
            };

        // ── Plantas ─────────────────────────────────────────
        tryLoad(girasol, "Sprites/Plantas/Girasol.png");
        tryLoad(lnzGst, "Sprites/Plantas/LnzGst.png");
        tryLoad(nuez_[0], "Sprites/Plantas/Nuez_1.png");
        tryLoad(nuez_[1], "Sprites/Plantas/Nuez_2.png");
        tryLoad(nuez_[2], "Sprites/Plantas/Nuez_3.png");
        tryLoad(cherry_[0], "Sprites/Plantas/Cherry_1.png");
        tryLoad(cherry_[1], "Sprites/Plantas/Cherry_2.png");
        tryLoad(chomper_[0], "Sprites/Plantas/Chomper_1.png");
        tryLoad(chomper_[1], "Sprites/Plantas/Chomper_2.png");
        tryLoad(chomper_[2], "Sprites/Plantas/Chomper_3.png");
        tryLoad(guisante, "Sprites/Plantas/LnzGstShoot.png");
        tryLoad(seedGirasol, "Sprites/Plantas/GirasolSeed.png");
        tryLoad(seedLnzGst, "Sprites/Plantas/LnzGstSeed.png");
        tryLoad(seedNuez, "Sprites/Plantas/NuezSeed.png");
        tryLoad(seedCereza, "Sprites/Plantas/CherrySeed.png");
        tryLoad(seedChomper, "Sprites/Plantas/ChomperSeed.png");
        tryLoad(pala, "Sprites/Plantas/Pala.png");

        // ── Zombies ─────────────────────────────────────────
        // Clasico
        tryLoad(zmb_[0], "Sprites/Zombies/Zmb_1.png");
        tryLoad(zmb_[1], "Sprites/Zombies/Zmb_2.png");
        tryLoad(zmbAtk_[0], "Sprites/Zombies/ZmbATK_1.png");
        tryLoad(zmbAtk_[1], "Sprites/Zombies/ZmbATK_2.png");
        tryLoad(zmbDead_, "Sprites/Zombies/Zmb_Dead.png");

        // Cono
        tryLoad(zmbCono_[0], "Sprites/Zombies/ZmbCono_1.png");
        tryLoad(zmbCono_[1], "Sprites/Zombies/ZmbCono_2.png");
        tryLoad(zmbConoAtk_[0], "Sprites/Zombies/ZmbConoATK_1.png");
        tryLoad(zmbConoAtk_[1], "Sprites/Zombies/ZmbConoATK_2.png");
        tryLoad(zmbConoDead_, "Sprites/Zombies/ZmbCono_Dead.png");

        // Jack
        tryLoad(zmbJack_[0], "Sprites/Zombies/ZmbJack_1.png");
        tryLoad(zmbJack_[1], "Sprites/Zombies/ZmbJack_2.png");
        tryLoad(zmbJackAtk_[0], "Sprites/Zombies/ZmbJackATK_1.png");
        tryLoad(zmbJackAtk_[1], "Sprites/Zombies/ZmbJackATK_2.png");
        tryLoad(zmbJackDead_, "Sprites/Zombies/ZmbJack_Dead.png");

        // Periodico
        tryLoad(zmpPer_[0], "Sprites/Zombies/ZmpPer_1.png");
        tryLoad(zmpPer_[1], "Sprites/Zombies/ZmpPer_2.png");
        tryLoad(zmpPerAtk_[0], "Sprites/Zombies/ZmpPerATK_1.png");
        tryLoad(zmpPerAtk_[1], "Sprites/Zombies/ZmpPerATK_2.png");
        tryLoad(zmpPerDead_, "Sprites/Zombies/ZmpPer_Dead.png");
        tryLoad(zmpPerNP_[0], "Sprites/Zombies/ZmpPerNP_1.png");
        tryLoad(zmpPerNP_[1], "Sprites/Zombies/ZmpPerNP_2.png");

        // Colgado
        tryLoad(zomC_[0], "Sprites/Zombies/ZomC_1.png");
        tryLoad(zomC_[1], "Sprites/Zombies/ZomC_2.png");
        tryLoad(zomC_[2], "Sprites/Zombies/ZomC_3.png");
        tryLoad(cuerda_, "Sprites/Zombies/Cuerda.png");
    }

    // ── Textura de campo (plantas) ───────────────────────────
    sf::Texture& texturaCampo(uint8_t id, uint8_t estado, uint8_t vida, uint32_t reloj) {
        switch (id) {
        case 1: return girasol;
        case 2: return lnzGst;

        case 3: {
            if (vida >= 20) return nuez_[0];
            else if (vida >= 10) return nuez_[1];
            else                 return nuez_[2];
        }

        case 4: {
            if (estado == ESTADO_ATACANDO) {
                return (reloj > 110) ? chomper_[1] : chomper_[2];
            }
            return chomper_[0];
        }

        case 5: {
            return (estado == ESTADO_ATACANDO) ? cherry_[1] : cherry_[0];
        }

        default: return fallback;
        }
    }

    // ── Textura de zombie ────────────────────────────────────
    // animFrame: contador global que sube cada tick en el game loop,
    // independiente del reloj de logica del ASM.
    sf::Texture& texturaZombie(uint8_t id, uint8_t estado, uint8_t vida, uint32_t animFrame) {
        int frame = (animFrame / 15) % 2;

        switch (id) {
        case 6: // Clasico
            if (estado == ESTADO_ATACANDO) return zmbAtk_[frame];
            return zmb_[frame];

        case 7: // Cono
            if (estado == ESTADO_ATACANDO) return zmbConoAtk_[frame];
            return zmbCono_[frame];

        case 8: // Periodico
            if (estado == ESTADO_ATACANDO) return zmpPerAtk_[frame];
            if (vida <= 15) return zmpPerNP_[frame]; // rapido, sin periodico
            return zmpPer_[frame];

        case 9: // Jack
            if (estado == ESTADO_ATACANDO) return zmbJackAtk_[frame];
            return zmbJack_[frame];

        case 10: // Colgado — 3 frames de ataque
            return zomC_[(animFrame / 10) % 3];

        default: return fallback;
        }
    }

    // Devuelve el sprite de muerte segun tipo de zombie.
    // Para Jack: si murio explotando (estado==ATACANDO) muestra el sprite de ataque,
    // si murio por daño normal muestra el _Dead.
    sf::Texture& texturaMuerto(uint8_t id, uint8_t estadoAlMorir) {
        switch (id) {
        case 6:  return zmbDead_;
        case 7:  return zmbConoDead_;
        case 8:  return zmpPerDead_;
        case 9:
            // Jack exploto → sprite atk_2 (explosion); mato una planta → _Dead
            return (estadoAlMorir == ESTADO_ATACANDO) ? zmbJackAtk_[1] : zmbJackDead_;
        case 10: return zomC_[2];
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


// Escala manteniendo proporcion, usando la ALTURA como referencia
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

// Pone el origen en el centro del bounding box local
void centrarOrigen(sf::Sprite& spr) {
    sf::FloatRect b = spr.getLocalBounds();
    spr.setOrigin(b.left + b.width * 0.5f, b.top + b.height * 0.5f);
}

// ═══════════════════════════════════════════════════════════════
//  Utilidades de debug
// ═══════════════════════════════════════════════════════════════
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
    uint32_t animFrameZombie = 0;   // contador de animacion independiente del ASM

    // ── Registro de zombies muertos (animacion de muerte en C++) ──
    struct ZombieMuerto {
        uint8_t  id;
        uint8_t  estadoAlMorir;  // estado del zombie en el tick que murio
        float    x, y;
        int      framesTTL;
        bool     jackExploto;    // true si Jack murio explotando al tocar una planta (vida>0)
    };
    static const int MAX_MUERTOS = 30;
    ZombieMuerto muertos[MAX_MUERTOS] = {};

    // Snapshot del frame anterior para detectar zombies que desaparecieron
    struct SnapZombie {
        uint8_t  id;
        uint8_t  estado;         // estado en el tick anterior
        uint16_t posX;
        uint8_t  filaY;
        uint8_t  vida;
    };
    SnapZombie snapAnterior[30] = {};

    // Snapshot de plantas para detectar las que desaparecen por Jack
    struct SnapPlanta {
        uint8_t  id;
        uint8_t  estado;
        uint8_t  vida;
        uint8_t  filaY;
        uint16_t posX;
    };
    SnapPlanta snapPlantaAnterior[45] = {};

    // Plantas "fantasma": se muestran durante la animacion de explosion de Jack
    struct PlantaFantasma {
        uint8_t  id;
        uint8_t  estado;
        uint8_t  vida;
        float    x, y;
        int      framesTTL;
    };
    static const int MAX_FANTASMAS = 10;
    PlantaFantasma plantasFantasma[MAX_FANTASMAS] = {};

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

    // ── Sprites reutilizables ─────────────────────────────────
    sf::Sprite sprPlanta;
    sf::Sprite sprDisparo;
    sf::Sprite sprZombie;

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

        animFrameZombie++;

        // ── Detectar zombies que murieron este tick ──────────────
        // Un zombie "murió" si en el frame anterior tenia id>=6 y vida>0,
        // y ahora su slot tiene id=0 (LimpiarEntidad lo borró en el ASM).
        for (int i = 0; i < 30; i++) {
            bool vivoAntes = (snapAnterior[i].id >= 6 && snapAnterior[i].id <= 10
                && snapAnterior[i].vida > 0);
            bool muertoAhora = (horda[i].id == 0);

            if (vivoAntes && muertoAhora) {
                // Jack: vida>0 al morir = exploto al tocar planta;
                // vida==0 = lo mataron disparos/cereza => sprite _Dead.
                bool jackExploto = (snapAnterior[i].id == 9 && snapAnterior[i].vida > 0);

                // Buscar hueco en el array de muertos
                for (int m = 0; m < MAX_MUERTOS; m++) {
                    if (muertos[m].framesTTL <= 0) {
                        muertos[m].id = snapAnterior[i].id;
                        muertos[m].estadoAlMorir = snapAnterior[i].estado;
                        muertos[m].x = static_cast<float>(snapAnterior[i].posX);
                        muertos[m].y = celdaCYZombie(snapAnterior[i].filaY);
                        muertos[m].jackExploto = jackExploto;
                        muertos[m].framesTTL = jackExploto ? 50 : 40;
                        break;
                    }
                }

                // Si Jack exploto, buscar la planta que desaparecio en este mismo tick
                // y guardarla como fantasma para que siga visible durante la animacion
                if (jackExploto) {
                    for (int p = 0; p < 45; p++) {
                        bool plantaVivaAntes = (snapPlantaAnterior[p].id >= 1 && snapPlantaAnterior[p].id <= 5);
                        bool plantaMuertaAhora = (defensa[p].id == 0);
                        if (!plantaVivaAntes || !plantaMuertaAhora) continue;

                        // Verificar que esta planta estaba en la misma fila y cerca de Jack
                        if (snapPlantaAnterior[p].filaY != snapAnterior[i].filaY) continue;
                        int dx = (int)snapAnterior[i].posX - (int)snapPlantaAnterior[p].posX;
                        if (dx < -70 || dx > 70) continue;

                        // Guardar como fantasma
                        for (int f = 0; f < MAX_FANTASMAS; f++) {
                            if (plantasFantasma[f].framesTTL <= 0) {
                                plantasFantasma[f].id = snapPlantaAnterior[p].id;
                                plantasFantasma[f].estado = snapPlantaAnterior[p].estado;
                                plantasFantasma[f].vida = snapPlantaAnterior[p].vida;
                                plantasFantasma[f].x = static_cast<float>(snapPlantaAnterior[p].posX);
                                plantasFantasma[f].y = celdaCY(snapPlantaAnterior[p].filaY);
                                plantasFantasma[f].framesTTL = 40;
                                break;
                            }
                        }
                        break; // solo una planta por Jack
                    }
                }
            }

            // Actualizar snapshot para el proximo tick
            snapAnterior[i].id = horda[i].id;
            snapAnterior[i].estado = horda[i].estado;
            snapAnterior[i].posX = horda[i].posX;
            snapAnterior[i].filaY = horda[i].filaY;
            snapAnterior[i].vida = horda[i].vida;
        }

        // Actualizar snapshot de plantas
        for (int p = 0; p < 45; p++) {
            snapPlantaAnterior[p].id = defensa[p].id;
            snapPlantaAnterior[p].estado = defensa[p].estado;
            snapPlantaAnterior[p].vida = defensa[p].vida;
            snapPlantaAnterior[p].filaY = defensa[p].filaY;
            snapPlantaAnterior[p].posX = defensa[p].posX;
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
        ventana.clear(sf::Color(15, 10, 5));

        // ── Fondo estilo PvZ ──────────────────────────────────
        {
            static const sf::Color FILA_CLARA(106, 154, 55);
            static const sf::Color FILA_OSCURA(82, 120, 40);

            sf::RectangleShape filaRect(sf::Vector2f(800.f, CELDA_H));
            for (int fila = 0; fila < 5; fila++) {
                filaRect.setPosition(0.f, BARRA_H + fila * CELDA_H);
                filaRect.setFillColor((fila % 2 == 0) ? FILA_CLARA : FILA_OSCURA);
                ventana.draw(filaRect);
            }

            sf::RectangleShape zonaBaja(sf::Vector2f(800.f, 600.f - (BARRA_H + 5 * CELDA_H)));
            zonaBaja.setPosition(0.f, BARRA_H + 5 * CELDA_H);
            zonaBaja.setFillColor(FILA_OSCURA);
            ventana.draw(zonaBaja);

            for (int col = 1; col < 9; col++) {
                sf::RectangleShape linea(sf::Vector2f(1.f, 5 * CELDA_H));
                linea.setPosition(col * CELDA_W, BARRA_H);
                linea.setFillColor(sf::Color(0, 0, 0, 40));
                ventana.draw(linea);
            }
        }

        // ── Barra de seeds ──────────────────────────────────────
        ventana.draw(panelBarra);

        for (int i = 0; i < 6; i++) {
            uint32_t idSeed = (i == 5) ? ID_PALA : (uint32_t)(i + 1);
            bool disponible = (idSeed == ID_PALA) ? true : ((modoPrueba != 0) || PlantaDisponible(nivelEnJuego, idSeed));

            float cx = i * CELDA_W + CELDA_W * 0.5f;
            float cy = BARRA_H * 0.5f;

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
                sprPlanta = sf::Sprite();
                sprPlanta.setTexture(tex.pala);
                escalarPorMayor(sprPlanta, SEED_TAM);
                centrarOrigen(sprPlanta);
                sprPlanta.setPosition(cx, cy);
                sprPlanta.setColor(sf::Color::White);
                ventana.draw(sprPlanta);
            }
            else {
                sprPlanta.setTexture(tex.texturaSeed(idSeed));
                escalarPorMayor(sprPlanta, SEED_TAM);
                centrarOrigen(sprPlanta);
                sprPlanta.setPosition(cx, cy);
                sprPlanta.setColor(disponible ? sf::Color::White : sf::Color(80, 80, 80, 180));
                ventana.draw(sprPlanta);
            }
        }

        // ── Plantas en campo ────────────────────────────────────
        for (int i = 0; i < 45; i++) {
            uint8_t id = defensa[i].id;
            if (id < 1 || id > 5) continue;

            sf::Texture& t = tex.texturaCampo(id, defensa[i].estado, defensa[i].vida, defensa[i].reloj);

            sprPlanta = sf::Sprite();
            sprPlanta.setTexture(t);
            escalarPorAltura(sprPlanta, PLANTA_TAM);
            centrarOrigen(sprPlanta);
            sprPlanta.setColor(sf::Color::White);

            float px = static_cast<float>(defensa[i].posX);
            float py = celdaCY(defensa[i].filaY);

            sprPlanta.setPosition(px, py);
            ventana.draw(sprPlanta);
        }

        // ── Plantas fantasma (se mantienen visibles durante la explosion de Jack) ──
        for (int f = 0; f < MAX_FANTASMAS; f++) {
            if (plantasFantasma[f].framesTTL <= 0) continue;
            plantasFantasma[f].framesTTL--;

            sf::Texture& tf = tex.texturaCampo(
                plantasFantasma[f].id,
                plantasFantasma[f].estado,
                plantasFantasma[f].vida,
                0);

            sprPlanta = sf::Sprite();
            sprPlanta.setTexture(tf);
            escalarPorAltura(sprPlanta, PLANTA_TAM);
            centrarOrigen(sprPlanta);
            sprPlanta.setColor(sf::Color::White);
            sprPlanta.setPosition(plantasFantasma[f].x, plantasFantasma[f].y);
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

        // ── Zombies ─────────────────────────────────────────────
        for (int i = 0; i < 30; i++) {
            uint8_t id = horda[i].id;
            if (id < 6 || id > 10) continue;

            float x = static_cast<float>(horda[i].posX);
            float y = celdaCYZombie(horda[i].filaY);

            // Colgado: dibuja la cuerda encima del zombie
            if (id == 10) {
                sprZombie = sf::Sprite();
                sprZombie.setTexture(tex.cuerda_);
                escalarPorAltura(sprZombie, ZOMBIE_TAM);
                centrarOrigen(sprZombie);
                sprZombie.setPosition(x, y - 55.f);
                ventana.draw(sprZombie);

                y -= 35.f;
            }

            sf::Texture& tz = tex.texturaZombie(
                id,
                horda[i].estado,
                horda[i].vida,
                animFrameZombie         // contador global, no reloj del ASM
            );

            sprZombie = sf::Sprite();
            sprZombie.setTexture(tz);
            escalarPorAltura(sprZombie, ZOMBIE_TAM);
            centrarOrigen(sprZombie);
            sprZombie.setPosition(x, y);
            ventana.draw(sprZombie);
        }

        // ── Zombies muertos (animacion de muerte gestionada en C++) ──
        // Los sprites de muerte tienen dimensiones distintas a los de walk,
        // por eso se escalan a ZOMBIE_TAM/1.5 y se baja el punto de anclaje
        // para que la base quede al nivel del suelo correcto.
        static const float ZOMBIE_MUERTO_TAM = ZOMBIE_TAM / 1.5f;

        for (int m = 0; m < MAX_MUERTOS; m++) {
            if (muertos[m].framesTTL <= 0) continue;

            muertos[m].framesTTL--;

            // Fade: alpha decrece de 255 a 0 en los ultimos 20 frames
            uint8_t alpha = (muertos[m].framesTTL < 20)
                ? (uint8_t)(muertos[m].framesTTL * 12)
                : 255;

            sf::Texture* ptd = nullptr;
            if (muertos[m].id == 9 && muertos[m].jackExploto) {
                // Explosion de Jack: primeros 25 frames = atk_1, luego = atk_2
                ptd = (muertos[m].framesTTL >= 25)
                    ? &tex.zmbJackAtk_[0]
                    : &tex.zmbJackAtk_[1];
            }
            else {
                ptd = &tex.texturaMuerto(muertos[m].id, muertos[m].estadoAlMorir);
            }

            bool esExplosionJack = (muertos[m].id == 9 && muertos[m].jackExploto);
            float tamMuerto = esExplosionJack ? ZOMBIE_TAM : ZOMBIE_MUERTO_TAM;
            float offsetY = esExplosionJack ? 0.f : ZOMBIE_MUERTO_TAM * 0.5f;

            sprZombie = sf::Sprite();
            sprZombie.setTexture(*ptd);
            escalarPorAltura(sprZombie, tamMuerto);
            centrarOrigen(sprZombie);
            sprZombie.setColor(sf::Color(255, 255, 255, alpha));
            sprZombie.setPosition(muertos[m].x, muertos[m].y + offsetY);
            ventana.draw(sprZombie);
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