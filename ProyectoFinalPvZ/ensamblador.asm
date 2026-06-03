.386
.MODEL FLAT, C
.STACK 4096

PUBLIC InicializarJuego
PUBLIC ColocarPlanta
PUBLIC AparecerZombie
PUBLIC AparecerZombieColgado
PUBLIC ActualizarColgado
PUBLIC ActualizarJuego
PUBLIC CrearDisparo
PUBLIC ActualizarDisparos
PUBLIC AplicarInstakill
PUBLIC AplicarCereza
PUBLIC IntentarColocarLanzaguisantes
PUBLIC IntentarColocarPlanta
PUBLIC ActualizarEconomia
PUBLIC modoPrueba
PUBLIC nivelActual
PUBLIC contadorSpawn
PUBLIC hordaFinalActiva
PUBLIC nivelTerminado
PUBLIC ConfigurarNivel
PUBLIC ActualizarNivel
PUBLIC juegoTerminado

PUBLIC soles
PUBLIC defensa
PUBLIC horda
PUBLIC disparos

ID_GIRASOL          EQU 1
ID_LANZAGUISANTES   EQU 2
ID_NUEZ             EQU 3
ID_CARNIVORA        EQU 4
ID_CEREZA           EQU 5

ID_ZOMBIE_CLASICO   EQU 6
ID_ZOMBIE_CONO      EQU 7
ID_ZOMBIE_PERIODICO EQU 8
ID_ZOMBIE_JACK      EQU 9
ID_ZOMBIE_COLGADO   EQU 10

ID_PALA             EQU 99

ID_GUISANTE         EQU 20

ESTADO_INACTIVO     EQU 0
ESTADO_CAMINANDO    EQU 1
ESTADO_ATACANDO     EQU 2
ESTADO_QUIETO       EQU 3

Entidad STRUCT
    ID      BYTE 0
    Vida    BYTE 0
    Dano    BYTE 0
    Estado  BYTE 0
    FilaY   BYTE 0
    Padding BYTE 0
    PosX    WORD 0
    Reloj   DWORD 0
Entidad ENDS

.DATA

    soles DWORD 9999
    contadorSoles DWORD 0
    contadorOleada DWORD 0
    juegoTerminado DWORD 0

    defensa  Entidad 45 DUP (<>)
    horda    Entidad 30 DUP (<>)
    disparos Entidad 50 DUP (<>)

    modoPrueba        DWORD 1
    nivelActual       DWORD 0
    contadorSpawn     DWORD 0
    hordaFinalActiva  DWORD 0
    nivelTerminado    DWORD 0

    zClasico          DWORD 0
    zCono             DWORD 0
    zPeriodico        DWORD 0
    zJack             DWORD 0
    zColgado          DWORD 0

    hClasico          DWORD 0
    hCono             DWORD 0
    hPeriodico        DWORD 0
    hJack             DWORD 0
    hColgado          DWORD 0

.CODE

; Limpia una entidad apuntada por ESI. Se usa internamente para desactivar plantas, zombies y disparos sin repetir código.
LimpiarEntidad PROC
    MOV BYTE PTR [ESI].Entidad.ID, 0
    MOV BYTE PTR [ESI].Entidad.Vida, 0
    MOV BYTE PTR [ESI].Entidad.Dano, 0
    MOV BYTE PTR [ESI].Entidad.Estado, 0
    MOV BYTE PTR [ESI].Entidad.FilaY, 0
    MOV WORD PTR [ESI].Entidad.PosX, 0
    MOV DWORD PTR [ESI].Entidad.Reloj, 0
    RET
LimpiarEntidad ENDP

; Reinicia la partida limpiando plantas, zombies y disparos. También deja los soles altos para pruebas.
InicializarJuego PROC USES ESI ECX

    MOV DWORD PTR soles, 9999
    MOV DWORD PTR contadorSoles, 0
    MOV DWORD PTR contadorOleada, 0
    MOV DWORD PTR juegoTerminado, 0

    LEA ESI, defensa
    MOV ECX, 45

Inicializar_Defensa:
    CALL LimpiarEntidad
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ Inicializar_Defensa

    LEA ESI, horda
    MOV ECX, 30

Inicializar_Horda:
    CALL LimpiarEntidad
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ Inicializar_Horda

    LEA ESI, disparos
    MOV ECX, 50

Inicializar_Disparos:
    CALL LimpiarEntidad
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ Inicializar_Disparos

    RET
InicializarJuego ENDP

; Coloca una planta directamente en una celda sin descontar soles. Recibe fila, columna y tipo de planta.
ColocarPlanta PROC USES ESI EAX EBX, fila:DWORD, columna:DWORD, tipo:DWORD

    MOV EAX, fila
    MOV EBX, 9
    MUL EBX
    ADD EAX, columna

    MOV EBX, TYPE Entidad
    MUL EBX

    LEA ESI, defensa
    ADD ESI, EAX

    CMP BYTE PTR [ESI].Entidad.ID, 0
    JNE ColocarPlanta_Fin

    MOV AL, BYTE PTR tipo
    MOV BYTE PTR [ESI].Entidad.ID, AL

    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_QUIETO

    MOV AL, BYTE PTR fila
    MOV BYTE PTR [ESI].Entidad.FilaY, AL

    MOV EAX, columna
    MOV EBX, 80
    MUL EBX
    ADD EAX, 40
    MOV WORD PTR [ESI].Entidad.PosX, AX

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    CMP BYTE PTR [ESI].Entidad.ID, ID_GIRASOL
    JE ColocarPlanta_Girasol

    CMP BYTE PTR [ESI].Entidad.ID, ID_LANZAGUISANTES
    JE ColocarPlanta_Lanzaguisantes

    CMP BYTE PTR [ESI].Entidad.ID, ID_NUEZ
    JE ColocarPlanta_Nuez

    CMP BYTE PTR [ESI].Entidad.ID, ID_CARNIVORA
    JE ColocarPlanta_Carnivora

    CMP BYTE PTR [ESI].Entidad.ID, ID_CEREZA
    JE ColocarPlanta_Cereza

    JMP ColocarPlanta_Fin

ColocarPlanta_Girasol:
    MOV BYTE PTR [ESI].Entidad.Vida, 5
    MOV BYTE PTR [ESI].Entidad.Dano, 0
    JMP ColocarPlanta_Fin

ColocarPlanta_Lanzaguisantes:
    MOV BYTE PTR [ESI].Entidad.Vida, 5
    MOV BYTE PTR [ESI].Entidad.Dano, 1
    JMP ColocarPlanta_Fin

ColocarPlanta_Nuez:
    MOV BYTE PTR [ESI].Entidad.Vida, 30
    MOV BYTE PTR [ESI].Entidad.Dano, 0
    JMP ColocarPlanta_Fin

ColocarPlanta_Carnivora:
    MOV BYTE PTR [ESI].Entidad.Vida, 8
    MOV BYTE PTR [ESI].Entidad.Dano, 10
    JMP ColocarPlanta_Fin

ColocarPlanta_Cereza:
    MOV BYTE PTR [ESI].Entidad.Vida, 1
    MOV BYTE PTR [ESI].Entidad.Dano, 10

ColocarPlanta_Fin:
    RET
ColocarPlanta ENDP

; Intenta colocar una planta usando soles. Valida costo, celda vacía, posición y estadísticas de cada tipo.
IntentarColocarPlanta PROC USES EAX EBX ESI, fila:DWORD, columna:DWORD, tipo:DWORD

    MOV EAX, tipo

    CMP EAX, ID_PALA
    JE UsarPala

    CMP EAX, ID_GIRASOL
    JE IntentarPlanta_CostoGirasol

    CMP EAX, ID_LANZAGUISANTES
    JE IntentarPlanta_CostoLanzaguisantes

    CMP EAX, ID_NUEZ
    JE IntentarPlanta_CostoNuez

    CMP EAX, ID_CARNIVORA
    JE IntentarPlanta_CostoCarnivora

    CMP EAX, ID_CEREZA
    JE IntentarPlanta_CostoCereza

    JMP IntentarPlanta_Fin

UsarPala:
    MOV EAX, fila
    MOV EBX, 9
    MUL EBX
    ADD EAX, columna

    MOV EBX, TYPE Entidad
    MUL EBX

    LEA ESI, defensa
    ADD ESI, EAX

    CMP BYTE PTR [ESI].Entidad.ID, ID_GIRASOL
    JB IntentarPlanta_Fin

    CMP BYTE PTR [ESI].Entidad.ID, ID_CEREZA
    JA IntentarPlanta_Fin

    CALL LimpiarEntidad
    JMP IntentarPlanta_Fin

IntentarPlanta_CostoGirasol:
    MOV EBX, 50
    JMP IntentarPlanta_VerificarSoles

IntentarPlanta_CostoLanzaguisantes:
    MOV EBX, 100
    JMP IntentarPlanta_VerificarSoles

IntentarPlanta_CostoNuez:
    MOV EBX, 50
    JMP IntentarPlanta_VerificarSoles

IntentarPlanta_CostoCarnivora:
    MOV EBX, 150
    JMP IntentarPlanta_VerificarSoles

IntentarPlanta_CostoCereza:
    MOV EBX, 150

IntentarPlanta_VerificarSoles:
    CMP DWORD PTR soles, EBX
    JB IntentarPlanta_Fin

    MOV EAX, fila
    MOV EBX, 9
    MUL EBX
    ADD EAX, columna

    MOV EBX, TYPE Entidad
    MUL EBX

    LEA ESI, defensa
    ADD ESI, EAX

    CMP BYTE PTR [ESI].Entidad.ID, 0
    JNE IntentarPlanta_Fin

    MOV AL, BYTE PTR tipo
    MOV BYTE PTR [ESI].Entidad.ID, AL

    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_QUIETO

    MOV AL, BYTE PTR fila
    MOV BYTE PTR [ESI].Entidad.FilaY, AL

    MOV EAX, columna
    MOV EBX, 80
    MUL EBX
    ADD EAX, 40
    MOV WORD PTR [ESI].Entidad.PosX, AX

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    CMP BYTE PTR [ESI].Entidad.ID, ID_GIRASOL
    JE IntentarPlanta_CrearGirasol

    CMP BYTE PTR [ESI].Entidad.ID, ID_LANZAGUISANTES
    JE IntentarPlanta_CrearLanzaguisantes

    CMP BYTE PTR [ESI].Entidad.ID, ID_NUEZ
    JE IntentarPlanta_CrearNuez

    CMP BYTE PTR [ESI].Entidad.ID, ID_CARNIVORA
    JE IntentarPlanta_CrearCarnivora

    CMP BYTE PTR [ESI].Entidad.ID, ID_CEREZA
    JE IntentarPlanta_CrearCereza

    JMP IntentarPlanta_Fin

IntentarPlanta_CrearGirasol:
    SUB DWORD PTR soles, 50
    MOV BYTE PTR [ESI].Entidad.Vida, 5
    MOV BYTE PTR [ESI].Entidad.Dano, 0
    JMP IntentarPlanta_Fin

IntentarPlanta_CrearLanzaguisantes:
    SUB DWORD PTR soles, 100
    MOV BYTE PTR [ESI].Entidad.Vida, 5
    MOV BYTE PTR [ESI].Entidad.Dano, 1
    JMP IntentarPlanta_Fin

IntentarPlanta_CrearNuez:
    SUB DWORD PTR soles, 50
    MOV BYTE PTR [ESI].Entidad.Vida, 30
    MOV BYTE PTR [ESI].Entidad.Dano, 0
    JMP IntentarPlanta_Fin

IntentarPlanta_CrearCarnivora:
    SUB DWORD PTR soles, 150
    MOV BYTE PTR [ESI].Entidad.Vida, 8
    MOV BYTE PTR [ESI].Entidad.Dano, 10
    JMP IntentarPlanta_Fin

IntentarPlanta_CrearCereza:
    SUB DWORD PTR soles, 150
    MOV BYTE PTR [ESI].Entidad.Vida, 1
    MOV BYTE PTR [ESI].Entidad.Dano, 10

IntentarPlanta_Fin:
    RET
IntentarColocarPlanta ENDP

; Función de prueba para colocar únicamente lanzaguisantes. Se conserva por compatibilidad con versiones anteriores del C++.
IntentarColocarLanzaguisantes PROC fila:DWORD, columna:DWORD
    PUSH ID_LANZAGUISANTES
    PUSH columna
    PUSH fila
    CALL IntentarColocarPlanta
    ADD ESP, 12
    RET
IntentarColocarLanzaguisantes ENDP

; Crea un zombie en el primer espacio libre de la horda. Recibe fila, posición X inicial y tipo.
AparecerZombie PROC USES ESI ECX, fila:DWORD, coordenadaX:DWORD, tipo:DWORD

    LEA ESI, horda
    MOV ECX, 30

AparecerZombie_Buscar:
    CMP BYTE PTR [ESI].Entidad.ID, 0
    JE AparecerZombie_Crear

    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ AparecerZombie_Buscar

    RET

AparecerZombie_Crear:
    MOV AL, BYTE PTR tipo
    MOV BYTE PTR [ESI].Entidad.ID, AL

    MOV BYTE PTR [ESI].Entidad.Dano, 1
    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_CAMINANDO

    MOV AL, BYTE PTR fila
    MOV BYTE PTR [ESI].Entidad.FilaY, AL

    MOV AX, WORD PTR coordenadaX
    MOV WORD PTR [ESI].Entidad.PosX, AX

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_CLASICO
    JE AparecerZombie_Clasico

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_CONO
    JE AparecerZombie_Cono

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_PERIODICO
    JE AparecerZombie_Periodico

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_JACK
    JE AparecerZombie_Jack

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_COLGADO
    JE AparecerZombie_Colgado

    JMP AparecerZombie_Fin

AparecerZombie_Clasico:
    MOV BYTE PTR [ESI].Entidad.Vida, 10
    JMP AparecerZombie_Fin

AparecerZombie_Cono:
    MOV BYTE PTR [ESI].Entidad.Vida, 20
    JMP AparecerZombie_Fin

AparecerZombie_Periodico:
    MOV BYTE PTR [ESI].Entidad.Vida, 20
    JMP AparecerZombie_Fin

AparecerZombie_Jack:
    MOV BYTE PTR [ESI].Entidad.Vida, 12
    JMP AparecerZombie_Fin

AparecerZombie_Colgado:
    MOV BYTE PTR [ESI].Entidad.Vida, 10

AparecerZombie_Fin:
    RET
AparecerZombie ENDP

AparecerZombieColgado PROC USES ESI EDI EAX EBX ECX EDX

    INC DWORD PTR contadorOleada

    ; contar plantas ocupadas
    LEA ESI, defensa
    MOV ECX, 45
    XOR EDX, EDX

ContarPlantasColgado:
    CMP BYTE PTR [ESI].Entidad.ID, ID_GIRASOL
    JB NoContarColgado

    CMP BYTE PTR [ESI].Entidad.ID, ID_CEREZA
    JA NoContarColgado

    INC EDX

NoContarColgado:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ ContarPlantasColgado

    CMP EDX, 0
    JE FinColgado

    ; indice pseudo-aleatorio = (contadorOleada + contadorSoles) % cantidadPlantas
    MOV EAX, contadorOleada
    ADD EAX, contadorSoles
    MOV EBX, EDX
    XOR EDX, EDX
    DIV EBX

    ; EDX = planta elegida
    MOV EBX, EDX

    ; buscar la planta elegida
    LEA ESI, defensa
    MOV ECX, 45

BuscarPlantaAleatoriaColgado:
    CMP BYTE PTR [ESI].Entidad.ID, ID_GIRASOL
    JB SiguientePlantaAleatoria

    CMP BYTE PTR [ESI].Entidad.ID, ID_CEREZA
    JA SiguientePlantaAleatoria

    CMP EBX, 0
    JE PlantaElegidaColgado

    DEC EBX

SiguientePlantaAleatoria:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ BuscarPlantaAleatoriaColgado

    JMP FinColgado

PlantaElegidaColgado:
    LEA EDI, horda
    MOV ECX, 30

BuscarEspacioColgado:
    CMP BYTE PTR [EDI].Entidad.ID, 0
    JE CrearColgadoTemporal

    ADD EDI, TYPE Entidad
    DEC ECX
    JNZ BuscarEspacioColgado

    JMP FinColgado

CrearColgadoTemporal:
    MOV BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_COLGADO
    MOV BYTE PTR [EDI].Entidad.Vida, 10
    MOV BYTE PTR [EDI].Entidad.Dano, 0
    MOV BYTE PTR [EDI].Entidad.Estado, ESTADO_ATACANDO

    MOV AL, BYTE PTR [ESI].Entidad.FilaY
    MOV BYTE PTR [EDI].Entidad.FilaY, AL

    MOV AX, WORD PTR [ESI].Entidad.PosX
    MOV WORD PTR [EDI].Entidad.PosX, AX

    MOV DWORD PTR [EDI].Entidad.Reloj, 0

FinColgado:
    RET

AparecerZombieColgado ENDP

ActualizarColgado PROC USES ESI EDI ECX EDX EAX

    LEA ESI, horda
    MOV ECX, 30

BuscarColgadoActivo:

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_COLGADO
    JNE SiguienteColgadoActivo

    INC DWORD PTR [ESI].Entidad.Reloj

    CMP DWORD PTR [ESI].Entidad.Reloj, 60
    JB SiguienteColgadoActivo

    LEA EDI, defensa
    MOV EDX, 45

BuscarPlantaDebajo:

    CMP BYTE PTR [EDI].Entidad.ID, ID_GIRASOL
    JB SiguientePlantaDebajo

    CMP BYTE PTR [EDI].Entidad.ID, ID_CEREZA
    JA SiguientePlantaDebajo

    MOV AL, BYTE PTR [ESI].Entidad.FilaY
    CMP AL, BYTE PTR [EDI].Entidad.FilaY
    JNE SiguientePlantaDebajo

    MOV AX, WORD PTR [ESI].Entidad.PosX
    CMP AX, WORD PTR [EDI].Entidad.PosX
    JNE SiguientePlantaDebajo

    PUSH ESI
    MOV ESI, EDI
    CALL LimpiarEntidad
    POP ESI

    CALL LimpiarEntidad
    JMP SiguienteColgadoActivo

SiguientePlantaDebajo:
    ADD EDI, TYPE Entidad
    DEC EDX
    JNZ BuscarPlantaDebajo

    CALL LimpiarEntidad

SiguienteColgadoActivo:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ BuscarColgadoActivo

    RET

ActualizarColgado ENDP

; Actualiza la economía del juego. Incrementa un contador cada frame y, al llegar al límite,
; suma soles. La recarga base es lenta y cada girasol activo reduce el tiempo necesario.
ActualizarEconomia PROC USES ESI ECX EAX EBX

    INC DWORD PTR contadorSoles

    MOV EBX, 180

    LEA ESI, defensa
    MOV ECX, 45

Economia_ContarGirasoles:
    CMP BYTE PTR [ESI].Entidad.ID, ID_GIRASOL
    JNE Economia_SiguientePlanta

    SUB EBX, 20

    CMP EBX, 60
    JAE Economia_SiguientePlanta

    MOV EBX, 60

Economia_SiguientePlanta:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ Economia_ContarGirasoles

    CMP DWORD PTR contadorSoles, EBX
    JB Economia_Fin

    MOV DWORD PTR contadorSoles, 0
    ADD DWORD PTR soles, 25

Economia_Fin:
    RET

ActualizarEconomia ENDP

; Actualiza la lógica principal: aplica economía, cereza, instakill,
; mueve zombies, permite que zombies 6, 7 y 8 ataquen plantas,
; y finalmente actualiza los disparos.
ActualizarJuego PROC USES ESI EDI EBX ECX EDX EAX

    CALL ActualizarEconomia
    CALL ActualizarColgado
    CALL AplicarCereza
    CALL AplicarInstakill

    LEA ESI, horda
    MOV EBX, 30

ActualizarJuego_CicloZombie:

    CMP BYTE PTR [ESI].Entidad.ID, 0
    JE ActualizarJuego_SiguienteZombie

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_CLASICO
    JB ActualizarJuego_SiguienteZombie

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_PERIODICO
    JA ActualizarJuego_Movimiento

    LEA EDI, defensa
    MOV EDX, 45

ActualizarJuego_BuscarPlanta:

    CMP BYTE PTR [EDI].Entidad.ID, ID_GIRASOL
    JB ActualizarJuego_SiguientePlanta

    CMP BYTE PTR [EDI].Entidad.ID, ID_CEREZA
    JA ActualizarJuego_SiguientePlanta

    MOV AL, BYTE PTR [ESI].Entidad.FilaY
    CMP AL, BYTE PTR [EDI].Entidad.FilaY
    JNE ActualizarJuego_SiguientePlanta

    MOV AX, WORD PTR [EDI].Entidad.PosX
    ADD AX, 40
    CMP WORD PTR [ESI].Entidad.PosX, AX
    JA ActualizarJuego_SiguientePlanta

    MOV AX, WORD PTR [EDI].Entidad.PosX
    SUB AX, 25
    CMP WORD PTR [ESI].Entidad.PosX, AX
    JB ActualizarJuego_SiguientePlanta

    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_ATACANDO
    INC DWORD PTR [ESI].Entidad.Reloj

    CMP DWORD PTR [ESI].Entidad.Reloj, 60
    JB ActualizarJuego_SiguienteZombie

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    MOV AL, BYTE PTR [EDI].Entidad.Vida
    CMP AL, BYTE PTR [ESI].Entidad.Dano
    JBE ActualizarJuego_EliminarPlanta

    MOV AL, BYTE PTR [ESI].Entidad.Dano
    SUB BYTE PTR [EDI].Entidad.Vida, AL
    JMP ActualizarJuego_SiguienteZombie

ActualizarJuego_EliminarPlanta:

    PUSH ESI
    MOV ESI, EDI
    CALL LimpiarEntidad
    POP ESI

    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_CAMINANDO
    JMP ActualizarJuego_SiguienteZombie

ActualizarJuego_SiguientePlanta:

    ADD EDI, TYPE Entidad
    DEC EDX
    JNZ ActualizarJuego_BuscarPlanta

    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_CAMINANDO

ActualizarJuego_Movimiento:

    CMP BYTE PTR [ESI].Entidad.Estado, ESTADO_CAMINANDO
    JNE ActualizarJuego_SiguienteZombie

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_PERIODICO
    JNE ActualizarJuego_Lento

    CMP BYTE PTR [ESI].Entidad.Vida, 15
    JA ActualizarJuego_Lento

ActualizarJuego_Rapido:

    INC DWORD PTR [ESI].Entidad.Reloj

    CMP DWORD PTR [ESI].Entidad.Reloj, 1
    JB ActualizarJuego_SiguienteZombie

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    CMP WORD PTR [ESI].Entidad.PosX, 1
    JBE ActualizarJuego_EliminarZombie

    SUB WORD PTR [ESI].Entidad.PosX, 1
    JMP ActualizarJuego_SiguienteZombie

ActualizarJuego_Lento:

    INC DWORD PTR [ESI].Entidad.Reloj

    CMP DWORD PTR [ESI].Entidad.Reloj, 2
    JB ActualizarJuego_SiguienteZombie

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    CMP WORD PTR [ESI].Entidad.PosX, 1
    JBE ActualizarJuego_EliminarZombie

    SUB WORD PTR [ESI].Entidad.PosX, 1
    JMP ActualizarJuego_SiguienteZombie

ActualizarJuego_EliminarZombie:

    MOV DWORD PTR juegoTerminado, 1
    CALL LimpiarEntidad

ActualizarJuego_SiguienteZombie:

    ADD ESI, TYPE Entidad
    DEC EBX
    JNZ ActualizarJuego_CicloZombie

    CALL ActualizarDisparos

    RET

ActualizarJuego ENDP

; Crea un disparo de guisante en el primer espacio libre del arreglo de disparos. Recibe fila y posición X.
CrearDisparo PROC USES ESI ECX, fila:DWORD, posicionX:DWORD

    LEA ESI, disparos
    MOV ECX, 50

CrearDisparo_Buscar:
    CMP BYTE PTR [ESI].Entidad.ID, 0
    JE CrearDisparo_Crear

    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ CrearDisparo_Buscar

    RET

CrearDisparo_Crear:
    MOV BYTE PTR [ESI].Entidad.ID, ID_GUISANTE
    MOV BYTE PTR [ESI].Entidad.Vida, 1
    MOV BYTE PTR [ESI].Entidad.Dano, 1
    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_CAMINANDO

    MOV AL, BYTE PTR fila
    MOV BYTE PTR [ESI].Entidad.FilaY, AL

    MOV AX, WORD PTR posicionX
    MOV WORD PTR [ESI].Entidad.PosX, AX

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    RET
CrearDisparo ENDP

; Hace que los lanzaguisantes generen proyectiles, mueve los disparos y aplica daño contra zombies en el mismo carril.
ActualizarDisparos PROC USES ESI EDI ECX EDX EAX

    LEA ESI, defensa
    MOV ECX, 45

ActualizarDisparos_RevisarPlantas:
    CMP BYTE PTR [ESI].Entidad.ID, ID_LANZAGUISANTES
    JNE ActualizarDisparos_SiguientePlanta

    INC DWORD PTR [ESI].Entidad.Reloj

    CMP DWORD PTR [ESI].Entidad.Reloj, 60
    JL ActualizarDisparos_SiguientePlanta

    MOV DWORD PTR [ESI].Entidad.Reloj, 0

    MOVZX EAX, WORD PTR [ESI].Entidad.PosX
    ADD EAX, 30
    PUSH EAX

    MOVZX EAX, BYTE PTR [ESI].Entidad.FilaY
    PUSH EAX

    CALL CrearDisparo
    ADD ESP, 8

ActualizarDisparos_SiguientePlanta:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ ActualizarDisparos_RevisarPlantas

    LEA ESI, disparos
    MOV ECX, 50

ActualizarDisparos_CicloDisparos:
    CMP BYTE PTR [ESI].Entidad.ID, ID_GUISANTE
    JNE ActualizarDisparos_SiguienteDisparo

    ADD WORD PTR [ESI].Entidad.PosX, 4

    CMP WORD PTR [ESI].Entidad.PosX, 800
    JAE ActualizarDisparos_EliminarDisparo

    LEA EDI, horda
    MOV EDX, 30

ActualizarDisparos_BuscarZombie:
    CMP BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_CLASICO
    JB ActualizarDisparos_SiguienteZombie

    CMP BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_COLGADO
    JA ActualizarDisparos_SiguienteZombie

    CMP BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_COLGADO
    JE ActualizarDisparos_SiguienteZombie

    MOV AL, BYTE PTR [ESI].Entidad.FilaY
    CMP AL, BYTE PTR [EDI].Entidad.FilaY
    JNE ActualizarDisparos_SiguienteZombie

    MOV AX, WORD PTR [EDI].Entidad.PosX
    CMP AX, WORD PTR [ESI].Entidad.PosX
    JB ActualizarDisparos_SiguienteZombie

    SUB AX, WORD PTR [ESI].Entidad.PosX

    CMP AX, 20
    JA ActualizarDisparos_SiguienteZombie

    SUB BYTE PTR [EDI].Entidad.Vida, 1

    CMP BYTE PTR [EDI].Entidad.Vida, 0
    JG ActualizarDisparos_EliminarDisparo

    PUSH ESI
    MOV ESI, EDI
    CALL LimpiarEntidad
    POP ESI

ActualizarDisparos_EliminarDisparo:
    CALL LimpiarEntidad
    JMP ActualizarDisparos_SiguienteDisparo

ActualizarDisparos_SiguienteZombie:
    ADD EDI, TYPE Entidad
    DEC EDX
    JNZ ActualizarDisparos_BuscarZombie

ActualizarDisparos_SiguienteDisparo:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ ActualizarDisparos_CicloDisparos

    RET
ActualizarDisparos ENDP

; Busca cerezas activas y aplica su explosión una sola vez.
; La cereza elimina zombies en la misma fila, fila superior y fila inferior,
; y en rango horizontal cercano. Después de explotar, la cereza se borra.
AplicarCereza PROC USES ESI EDI ECX EDX EAX EBX

    LEA ESI, defensa
    MOV ECX, 45

Cereza_Buscar:

    CMP BYTE PTR [ESI].Entidad.ID, ID_CEREZA
    JNE Cereza_Siguiente

    INC DWORD PTR [ESI].Entidad.Reloj

    ; Al llegar a 60: activar sprite de explosión (estado = ESTADO_ATACANDO)
    CMP DWORD PTR [ESI].Entidad.Reloj, 60
    JB Cereza_Siguiente
    JE Cereza_MostrarExplosion

    ; A partir de 80: aplicar daño y eliminar
    CMP DWORD PTR [ESI].Entidad.Reloj, 80
    JB Cereza_Siguiente

    LEA EDI, horda
    MOV EDX, 30

Cereza_BuscarZombie:

    CMP BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_CLASICO
    JB Cereza_SiguienteZombie

    CMP BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_COLGADO
    JA Cereza_SiguienteZombie

    MOV AL, BYTE PTR [ESI].Entidad.FilaY
    MOV BL, BYTE PTR [EDI].Entidad.FilaY

    CMP BL, AL
    JE Cereza_RevisarX

    INC AL
    CMP BL, AL
    JE Cereza_RevisarX

    SUB AL, 2
    CMP BL, AL
    JNE Cereza_SiguienteZombie

Cereza_RevisarX:

    MOV AX, WORD PTR [ESI].Entidad.PosX
    ADD AX, 100
    CMP WORD PTR [EDI].Entidad.PosX, AX
    JA Cereza_SiguienteZombie

    MOV AX, WORD PTR [ESI].Entidad.PosX
    SUB AX, 100
    CMP WORD PTR [EDI].Entidad.PosX, AX
    JB Cereza_SiguienteZombie

    PUSH ESI
    MOV ESI, EDI
    CALL LimpiarEntidad
    POP ESI

Cereza_SiguienteZombie:
    ADD EDI, TYPE Entidad
    DEC EDX
    JNZ Cereza_BuscarZombie

    CALL LimpiarEntidad
    JMP Cereza_Siguiente

Cereza_MostrarExplosion:
    ; Cambiar estado para que el C++ dibuje cherry_[1] (explosión)
    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_ATACANDO

Cereza_Siguiente:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ Cereza_Buscar

    RET

AplicarCereza ENDP

; Aplica ataques instantáneos. La carnívora elimina zombies al contacto y Jack in the box elimina una planta al contacto y luego se destruye.
AplicarInstakill PROC USES ESI EDI ECX EDX EAX

    LEA ESI, defensa
    MOV ECX, 45

Instakill_BuscarCarnivora:

    CMP BYTE PTR [ESI].Entidad.ID, ID_CARNIVORA
    JNE Instakill_SiguienteCarnivora

    ; Si está en cooldown (ESTADO_ATACANDO con reloj > 0), decrementar y esperar
    CMP BYTE PTR [ESI].Entidad.Estado, ESTADO_ATACANDO
    JNE Carnivora_Lista

    ; Está en cooldown: decrementar reloj
    CMP DWORD PTR [ESI].Entidad.Reloj, 0
    JE Carnivora_TerminoCooldown

    DEC DWORD PTR [ESI].Entidad.Reloj
    JMP Instakill_SiguienteCarnivora

Carnivora_TerminoCooldown:
    ; Cooldown terminó: volver a estado idle y listo para atacar
    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_QUIETO
    JMP Instakill_SiguienteCarnivora

Carnivora_Lista:

    ; Solo busca zombies si está en estado idle (ESTADO_QUIETO o ESTADO_INACTIVO)
    LEA EDI, horda
    MOV EDX, 30

Instakill_ZombieParaCarnivora:

    CMP BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_CLASICO
    JB Instakill_SiguienteZombieCarnivora

    CMP BYTE PTR [EDI].Entidad.ID, ID_ZOMBIE_COLGADO
    JA Instakill_SiguienteZombieCarnivora

    MOV AL, BYTE PTR [ESI].Entidad.FilaY
    CMP AL, BYTE PTR [EDI].Entidad.FilaY
    JNE Instakill_SiguienteZombieCarnivora

    MOV AX, WORD PTR [ESI].Entidad.PosX
    ADD AX, 45
    CMP WORD PTR [EDI].Entidad.PosX, AX
    JA Instakill_SiguienteZombieCarnivora

    MOV AX, WORD PTR [ESI].Entidad.PosX
    SUB AX, 25
    CMP WORD PTR [EDI].Entidad.PosX, AX
    JB Instakill_SiguienteZombieCarnivora

    PUSH ESI
    MOV ESI, EDI
    CALL LimpiarEntidad
    POP ESI

    ; Estado 2 = atacando/masticando (sprite chomper_[1])
    MOV BYTE PTR [ESI].Entidad.Estado, ESTADO_ATACANDO
    MOV DWORD PTR [ESI].Entidad.Reloj, 300

    JMP Instakill_SiguienteCarnivora

Instakill_SiguienteZombieCarnivora:
    ADD EDI, TYPE Entidad
    DEC EDX
    JNZ Instakill_ZombieParaCarnivora

Instakill_SiguienteCarnivora:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ Instakill_BuscarCarnivora

    LEA ESI, horda
    MOV ECX, 30

Instakill_BuscarJack:

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_JACK
    JNE Instakill_SiguienteJack

    LEA EDI, defensa
    MOV EDX, 45

Instakill_PlantaParaJack:

    CMP BYTE PTR [EDI].Entidad.ID, ID_GIRASOL
    JB Instakill_SiguientePlantaJack

    CMP BYTE PTR [EDI].Entidad.ID, ID_CEREZA
    JA Instakill_SiguientePlantaJack

    MOV AL, BYTE PTR [ESI].Entidad.FilaY
    CMP AL, BYTE PTR [EDI].Entidad.FilaY
    JNE Instakill_SiguientePlantaJack

    MOV AX, WORD PTR [EDI].Entidad.PosX
    ADD AX, 45
    CMP WORD PTR [ESI].Entidad.PosX, AX
    JA Instakill_SiguientePlantaJack

    MOV AX, WORD PTR [EDI].Entidad.PosX
    SUB AX, 25
    CMP WORD PTR [ESI].Entidad.PosX, AX
    JB Instakill_SiguientePlantaJack

    PUSH ESI
    MOV ESI, EDI
    CALL LimpiarEntidad
    POP ESI

    CALL LimpiarEntidad

    JMP Instakill_SiguienteJack

Instakill_SiguientePlantaJack:
    ADD EDI, TYPE Entidad
    DEC EDX
    JNZ Instakill_PlantaParaJack

Instakill_SiguienteJack:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ Instakill_BuscarJack

    RET

AplicarInstakill ENDP

ConfigurarNivel PROC USES EAX, nivel:DWORD

    CALL InicializarJuego

    MOV EAX, nivel
    MOV DWORD PTR nivelActual, EAX

    MOV DWORD PTR juegoTerminado, 0

    MOV DWORD PTR contadorSpawn, 0
    MOV DWORD PTR hordaFinalActiva, 0
    MOV DWORD PTR nivelTerminado, 0

    MOV DWORD PTR zClasico, 0
    MOV DWORD PTR zCono, 0
    MOV DWORD PTR zPeriodico, 0
    MOV DWORD PTR zJack, 0
    MOV DWORD PTR zColgado, 0

    MOV DWORD PTR hClasico, 0
    MOV DWORD PTR hCono, 0
    MOV DWORD PTR hPeriodico, 0
    MOV DWORD PTR hJack, 0
    MOV DWORD PTR hColgado, 0

    CMP EAX, 99
    JE Nivel_Prueba

    MOV DWORD PTR modoPrueba, 0
    MOV DWORD PTR soles, 0

    CMP EAX, 0
    JE Nivel_1
    CMP EAX, 1
    JE Nivel_2
    CMP EAX, 2
    JE Nivel_3
    CMP EAX, 3
    JE Nivel_4
    CMP EAX, 4
    JE Nivel_5

    JMP ConfigurarNivel_Fin

Nivel_Prueba:
    MOV DWORD PTR modoPrueba, 1
    MOV DWORD PTR soles, 9999
    JMP ConfigurarNivel_Fin

Nivel_1:
    MOV DWORD PTR soles, 150
    MOV DWORD PTR zClasico, 12
    MOV DWORD PTR hClasico, 5
    JMP ConfigurarNivel_Fin

Nivel_2:
    MOV DWORD PTR soles, 50
    MOV DWORD PTR zClasico, 14
    MOV DWORD PTR zCono, 6
    MOV DWORD PTR hClasico, 4
    MOV DWORD PTR hCono, 3
    JMP ConfigurarNivel_Fin

Nivel_3:
    MOV DWORD PTR soles, 50
    MOV DWORD PTR zClasico, 12
    MOV DWORD PTR zCono, 8
    MOV DWORD PTR zPeriodico, 6
    MOV DWORD PTR hClasico, 3
    MOV DWORD PTR hCono, 4
    MOV DWORD PTR hPeriodico, 4
    JMP ConfigurarNivel_Fin

Nivel_4:
    MOV DWORD PTR soles, 50
    MOV DWORD PTR zClasico, 10
    MOV DWORD PTR zCono, 8
    MOV DWORD PTR zPeriodico, 8
    MOV DWORD PTR zJack, 5
    MOV DWORD PTR hCono, 4
    MOV DWORD PTR hPeriodico, 5
    MOV DWORD PTR hJack, 4
    JMP ConfigurarNivel_Fin

Nivel_5:
    MOV DWORD PTR soles, 50
    MOV DWORD PTR zClasico, 10
    MOV DWORD PTR zCono, 10
    MOV DWORD PTR zPeriodico, 10
    MOV DWORD PTR zJack, 6
    MOV DWORD PTR zColgado, 6
    MOV DWORD PTR hClasico, 5
    MOV DWORD PTR hCono, 5
    MOV DWORD PTR hPeriodico, 6
    MOV DWORD PTR hJack, 4
    MOV DWORD PTR hColgado, 4

ConfigurarNivel_Fin:
    RET

ConfigurarNivel ENDP

ContarZombiesActivos PROC USES ESI ECX

    XOR EAX, EAX

    LEA ESI, horda
    MOV ECX, 30

ContarActivos_Ciclo:
    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_CLASICO
    JB ContarActivos_Siguiente

    CMP BYTE PTR [ESI].Entidad.ID, ID_ZOMBIE_COLGADO
    JA ContarActivos_Siguiente

    INC EAX

ContarActivos_Siguiente:
    ADD ESI, TYPE Entidad
    DEC ECX
    JNZ ContarActivos_Ciclo

    RET

ContarZombiesActivos ENDP

TotalZombiesRestantes PROC

    MOV EAX, zClasico
    ADD EAX, zCono
    ADD EAX, zPeriodico
    ADD EAX, zJack
    ADD EAX, zColgado

    ADD EAX, hClasico
    ADD EAX, hCono
    ADD EAX, hPeriodico
    ADD EAX, hJack
    ADD EAX, hColgado

    RET

TotalZombiesRestantes ENDP

ElegirZombieNormalASM PROC USES EBX EDX

    MOV EAX, zClasico
    ADD EAX, zCono
    ADD EAX, zPeriodico
    ADD EAX, zJack
    ADD EAX, zColgado

    CMP EAX, 0
    JE ElegirNormal_Ninguno

    INC DWORD PTR contadorOleada

    MOV EBX, EAX
    MOV EAX, contadorOleada
    XOR EDX, EDX
    DIV EBX

    MOV EAX, EDX

    CMP EAX, zClasico
    JB ElegirNormal_Clasico
    SUB EAX, zClasico

    CMP EAX, zCono
    JB ElegirNormal_Cono
    SUB EAX, zCono

    CMP EAX, zPeriodico
    JB ElegirNormal_Periodico
    SUB EAX, zPeriodico

    CMP EAX, zJack
    JB ElegirNormal_Jack
    SUB EAX, zJack

    CMP EAX, zColgado
    JB ElegirNormal_Colgado

ElegirNormal_Ninguno:
    MOV EAX, 0
    RET

ElegirNormal_Clasico:
    DEC DWORD PTR zClasico
    MOV EAX, ID_ZOMBIE_CLASICO
    RET

ElegirNormal_Cono:
    DEC DWORD PTR zCono
    MOV EAX, ID_ZOMBIE_CONO
    RET

ElegirNormal_Periodico:
    DEC DWORD PTR zPeriodico
    MOV EAX, ID_ZOMBIE_PERIODICO
    RET

ElegirNormal_Jack:
    DEC DWORD PTR zJack
    MOV EAX, ID_ZOMBIE_JACK
    RET

ElegirNormal_Colgado:
    DEC DWORD PTR zColgado
    MOV EAX, ID_ZOMBIE_COLGADO
    RET

ElegirZombieNormalASM ENDP

ElegirZombieHordaASM PROC USES EBX EDX

    MOV EAX, hClasico
    ADD EAX, hCono
    ADD EAX, hPeriodico
    ADD EAX, hJack
    ADD EAX, hColgado

    CMP EAX, 0
    JE ElegirHorda_Ninguno

    INC DWORD PTR contadorOleada

    MOV EBX, EAX
    MOV EAX, contadorOleada
    ADD EAX, contadorSpawn
    XOR EDX, EDX
    DIV EBX

    MOV EAX, EDX

    CMP EAX, hClasico
    JB ElegirHorda_Clasico
    SUB EAX, hClasico

    CMP EAX, hCono
    JB ElegirHorda_Cono
    SUB EAX, hCono

    CMP EAX, hPeriodico
    JB ElegirHorda_Periodico
    SUB EAX, hPeriodico

    CMP EAX, hJack
    JB ElegirHorda_Jack
    SUB EAX, hJack

    CMP EAX, hColgado
    JB ElegirHorda_Colgado

ElegirHorda_Ninguno:
    MOV EAX, 0
    RET

ElegirHorda_Clasico:
    DEC DWORD PTR hClasico
    MOV EAX, ID_ZOMBIE_CLASICO
    RET

ElegirHorda_Cono:
    DEC DWORD PTR hCono
    MOV EAX, ID_ZOMBIE_CONO
    RET

ElegirHorda_Periodico:
    DEC DWORD PTR hPeriodico
    MOV EAX, ID_ZOMBIE_PERIODICO
    RET

ElegirHorda_Jack:
    DEC DWORD PTR hJack
    MOV EAX, ID_ZOMBIE_JACK
    RET

ElegirHorda_Colgado:
    DEC DWORD PTR hColgado
    MOV EAX, ID_ZOMBIE_COLGADO
    RET

ElegirZombieHordaASM ENDP

InvocarZombieASM PROC USES EBX EDX, tipo:DWORD

    MOV EAX, tipo

    CMP EAX, 0
    JE InvocarZombie_Fin

    CMP EAX, ID_ZOMBIE_COLGADO
    JE InvocarZombie_Colgado

    INC DWORD PTR contadorOleada

    MOV EAX, contadorOleada
    XOR EDX, EDX
    MOV EBX, 5
    DIV EBX

    PUSH tipo
    PUSH 790
    PUSH EDX
    CALL AparecerZombie
    ADD ESP, 12

    JMP InvocarZombie_Fin

InvocarZombie_Colgado:
    CALL AparecerZombieColgado

InvocarZombie_Fin:
    RET

InvocarZombieASM ENDP

ActualizarNivel PROC USES EAX

    CMP DWORD PTR modoPrueba, 1
    JE ActualizarNivel_Fin

    CMP DWORD PTR nivelTerminado, 1
    JE ActualizarNivel_Fin

    INC DWORD PTR contadorSpawn

    CMP DWORD PTR hordaFinalActiva, 0
    JNE ActualizarNivel_Horda

ActualizarNivel_Normal:
    CMP DWORD PTR contadorSpawn, 500                  ;Spawn Zombies normales
    JB ActualizarNivel_VerificarFin

    MOV DWORD PTR contadorSpawn, 0

    CALL ElegirZombieNormalASM

    CMP EAX, 0
    JE ActivarHordaFinal

    PUSH EAX
    CALL InvocarZombieASM
    ADD ESP, 4

    JMP ActualizarNivel_VerificarFin

ActivarHordaFinal:
    MOV DWORD PTR hordaFinalActiva, 1
    JMP ActualizarNivel_VerificarFin

ActualizarNivel_Horda:
    CMP DWORD PTR contadorSpawn, 70                   ;Spawn Zombies de la horda final

    JB ActualizarNivel_VerificarFin

    MOV DWORD PTR contadorSpawn, 0

    CALL ElegirZombieHordaASM

    CMP EAX, 0
    JE ActualizarNivel_VerificarFin

    PUSH EAX
    CALL InvocarZombieASM
    ADD ESP, 4

ActualizarNivel_VerificarFin:
    CMP DWORD PTR hordaFinalActiva, 1
    JNE ActualizarNivel_Fin

    CALL TotalZombiesRestantes
    CMP EAX, 0
    JNE ActualizarNivel_Fin

    CALL ContarZombiesActivos
    CMP EAX, 0
    JNE ActualizarNivel_Fin

    MOV DWORD PTR nivelTerminado, 1

ActualizarNivel_Fin:
    RET

ActualizarNivel ENDP



END