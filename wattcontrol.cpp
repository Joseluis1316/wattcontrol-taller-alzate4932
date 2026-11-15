// =============================================================
// WattControl v2 - Gestor de energia de un negocio (El Nino 2026)
// Negocio: TALLER DE MECANIZADO (ultimo digito 2 - 3)
// =============================================================
// --- Librerias --------------------------------------------------------------
// iostream: cout y cin
#include <iostream>
// iomanip: setw y setprecision
#include <iomanip>
// string: el tipo string
#include <string>
// cstdlib: rand() y srand()
#include <cstdlib>

using namespace std;

// --- Directivas del preprocesador -------------------------------------------
// Horas de la simulacion: un dia
#define HORAS_DIA 24
// Cuantas cargas tiene el negocio: 1 critica + 2 aplazables
#define MAX_CARGAS 3
// Semilla de rand(): misma semilla, mismos precios
#define SEMILLA 7

// =============================================================
// CONSTANTES DEL NEGOCIO (si cambia el negocio, solo se cambia este bloque)
// Valores de la tabla de la seccion 3 del proyecto de aula (taller de mecanizado)
// =============================================================
const string NOMBRE_NEGOCIO = "Taller de mecanizado";
// Nombres de las cargas
const string NOMBRE_CRITICA = "Iluminacion";
const string NOMBRE_APLAZABLE_1 = "Torno CNC";
const string NOMBRE_APLAZABLE_2 = "Compresor";
// kW de cada carga
const float POTENCIA_CRITICA = 1.5;
const float POTENCIA_APLAZABLE_1 = 7.5;
const float POTENCIA_APLAZABLE_2 = 3.0;
// kW: potencia contratada
const float POTENCIA_CONTRATADA = 10.0;
// COP/kWh: si el precio pasa de aqui, la energia esta cara
const float PRECIO_CARO = 750.0;
// Horas minimas de funcionamiento al dia de cada aplazable (justificar en el PDF)
const int HORAS_MIN_APLAZABLE_1 = 6;
const int HORAS_MIN_APLAZABLE_2 = 4;

// =============================================================
// CONSTANTES GENERALES (iguales para todos los negocios)
// =============================================================
// La hora pico va de 18 h a 20 h (incluidas)
const int PICO_INICIO = 18;
const int PICO_FIN = 20;
// %: por debajo de este nivel de embalses hay riesgo de racionamiento
const float NIVEL_ALERTA_EMBALSES = 40.0;
// COP/kWh: con embalses bajos, por encima de este precio hay emergencia de racionamiento
const float PRECIO_EMERGENCIA = 1500.0;
// 1 de cada 20 lecturas del precio falla
const int FALLA_CADA = 20;
// Rango (%) de los embalses que genera el Mercado al azar
const int EMBALSE_MIN = 35;
const int EMBALSE_MAX = 85;
// Posicion de la carga critica en el arreglo de cargas
const int INDICE_CRITICA = 0;
// Limites de las lecturas manuales
const int HORA_MIN = 0;
const int HORA_MAX = 23;
const float EMBALSE_PORC_MIN = 0;
const float EMBALSE_PORC_MAX = 100;
// Numero de franjas horarias del Mercado
const int NUM_FRANJAS = 5;
// Ultima hora de cada franja: madrugada, manana-mediodia, tarde, pico, noche
const int FRANJA_FIN[NUM_FRANJAS] = {6, 14, 17, 20, 23};
// Precio minimo y maximo real de cada franja (COP/kWh, Anexo B, XM 21-27 sep 2026)
const int PRECIO_MIN[NUM_FRANJAS] = {398, 456, 596, 705, 565};
const int PRECIO_MAX[NUM_FRANJAS] = {640, 970, 1040, 1612, 970};

// =============================================================
// Clase base abstracta Carga
// =============================================================
class Carga {
protected:
    // Nombre de la carga
    string nombre;
    // Potencia en kW
    float potenciaKW;
    // true si esta encendida en esta hora
    bool encendida;
    // Horas que ha estado encendida
    int horasUso;

public:
    Carga(string n, float p) {
        nombre = n;
        potenciaKW = p;
        encendida = false;
        horasUso = 0;
    }

    // Destructor virtual: obligatorio en una clase base con metodos virtuales
    virtual ~Carga() {}

    // Polimorfismo: metodo virtual puro; cada hija aplica su propia regla.
    // Esto hace que Carga sea ABSTRACTA
    virtual bool debeEncender(int hora, float precio, float embalses) = 0;

    // Por defecto una carga cumple su minimo; la aplazable lo reemplaza
    virtual bool cumpleMinimo() { return true; }

    void encender() {
        encendida = true;
        horasUso++;
    }

    void apagar() { encendida = false; }

    // Deja la carga como nueva (para repetir la simulacion)
    void reiniciar() {
        encendida = false;
        horasUso = 0;
    }

    // Costo de esta hora: kW x precio si esta encendida, 0 si no
    float costoHora(float precio) {
        if (encendida) return potenciaKW * precio;
        return 0;
    }

    string getNombre() { return nombre; }
    float getPotencia() { return potenciaKW; }
    bool estaEncendida() { return encendida; }
    int getHorasUso() { return horasUso; }
};

// =============================================================
// Herencia: CargaCritica ES UNA Carga (nunca se apaga)
// =============================================================
class CargaCritica : public Carga {
public:
    CargaCritica(string n, float p) : Carga(n, p) {}

    // Regla 1: la critica siempre permanece encendida
    bool debeEncender(int hora, float precio, float embalses) override {
        return true;
    }
};

// =============================================================
// Herencia: CargaAplazable ES UNA Carga (se aplaza en pico, con energia cara
// o con embalses bajos)
// =============================================================
class CargaAplazable : public Carga {
private:
    // Horas minimas que debe funcionar al dia
    int horasMinimas;

public:
    CargaAplazable(string n, float p, int hMin) : Carga(n, p) {
        horasMinimas = hMin;
    }

    // Reglas 2 a 5 en una sola condicion compuesta:
    // se APLAZA si embalses < 40 % || es hora pico || precio > precio caro.
    // Se enciende si NO ocurre ninguna de las tres (regla 5)
    bool debeEncender(int hora, float precio, float embalses) override {
        bool horaPico = hora >= PICO_INICIO && hora <= PICO_FIN;
        return !(embalses < NIVEL_ALERTA_EMBALSES || horaPico || precio > PRECIO_CARO);
    }

    // true si ya funciono las horas minimas del dia
    bool cumpleMinimo() override {
        return horasUso >= horasMinimas;
    }
};

// =============================================================
// Medidor: parte del Negocio (composicion)
// =============================================================
class Medidor {
private:
    // kWh acumulados y costo acumulado (COP)
    float kWhTotal;
    float costoTotal;

public:
    Medidor() {
        kWhTotal = 0;
        costoTotal = 0;
    }

    // Suma una hora de consumo: kW x 1 h = kWh; costo = kWh x precio
    void acumular(float kW, float precio) {
        kWhTotal += kW;
        costoTotal += kW * precio;
    }

    void reiniciar() {
        kWhTotal = 0;
        costoTotal = 0;
    }

    float getKWh() { return kWhTotal; }
    float getCosto() { return costoTotal; }
};

// =============================================================
// Mercado: da el precio de cada hora y el nivel de los embalses (asociacion)
// =============================================================
class Mercado {
public:
    // Precio al azar dentro del rango real de la franja de esa hora
    float precioHora(int hora) {
        int franja = 0;
        // Busca la franja: la primera cuya ultima hora no sea menor que la hora
        while (hora > FRANJA_FIN[franja]) franja++;
        return PRECIO_MIN[franja] + rand() % (PRECIO_MAX[franja] - PRECIO_MIN[franja] + 1);
    }

    // Nivel de los embalses (%) al azar entre EMBALSE_MIN y EMBALSE_MAX
    float nivelEmbalses(int hora) {
        return EMBALSE_MIN + rand() % (EMBALSE_MAX - EMBALSE_MIN + 1);
    }
};

// =============================================================
// Negocio: junta todo
// =============================================================
class Negocio {
private:
    // Nombre del negocio
    string nombre;
    // COMPOSICION: el medidor nace y muere con el negocio
    Medidor medidor;
    // AGREGACION: punteros a Carga; las cargas se crean afuera
    Carga* cargas[MAX_CARGAS];
    int numCargas;
    // ASOCIACION: el negocio conoce al mercado y le pide datos
    Mercado* mercado;

    // Resultados de la ultima simulacion (para la opcion 4)
    float costoSin;
    int horasSimuladas, fallas;
    bool emergencia, produccionIncompleta;

    // Decide que cargas se encienden en una hora (reglas 1 a 6).
    // Devuelve en excedio si hubo que aplicar la regla 6
    void decidir(int hora, float precio, float embalses, bool decision[], bool &excedio) {
        float kwEncendidos = 0;
        // Polimorfismo: la misma llamada para todas las cargas
        for (int i = 0; i < numCargas; i++) {
            decision[i] = cargas[i]->debeEncender(hora, precio, embalses);
            if (decision[i]) kwEncendidos += cargas[i]->getPotencia();
        }
        excedio = false;
        // Regla 6: si se pasa de la potencia contratada, se apaga la aplazable de mayor potencia
        if (kwEncendidos > POTENCIA_CONTRATADA) {
            int mayor = -1;
            for (int i = 0; i < numCargas; i++) {
                if (i == INDICE_CRITICA) continue;
                if (decision[i] && (mayor == -1 || cargas[i]->getPotencia() > cargas[mayor]->getPotencia())) {
                    mayor = i;
                }
            }
            if (mayor != -1) {
                decision[mayor] = false;
                excedio = true;
            }
        }
    }

public:
    Negocio(string n, Mercado* m) {
        nombre = n;
        mercado = m;
        numCargas = 0;
        costoSin = 0;
        horasSimuladas = 0;
        fallas = 0;
        emergencia = false;
        produccionIncompleta = false;
    }

    // Agrega una carga ya creada afuera (agregacion)
    void agregarCarga(Carga* c) {
        if (numCargas < MAX_CARGAS) {
            cargas[numCargas] = c;
            numCargas++;
        }
    }

    void mostrar() {
        cout << "Negocio " << nombre << " | potencia contratada " << POTENCIA_CONTRATADA << " kW" << endl;
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << cargas[i]->getNombre() << " (" << cargas[i]->getPotencia() << " kW)" << endl;
        }
    }

    // Opcion 2: que haria cada carga con una lectura (hora, precio, embalses)
    void evaluar(int hora, float precio, float embalses) {
        bool decision[MAX_CARGAS];
        bool excedio;
        if (embalses < NIVEL_ALERTA_EMBALSES) {
            cout << "  ALERTA: riesgo de racionamiento, solo funciona la carga critica" << endl;
        }
        decidir(hora, precio, embalses, decision, excedio);
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << setw(12) << left << cargas[i]->getNombre() << right
                 << (decision[i] ? "ENCENDER" : "APLAZAR") << endl;
        }
        if (excedio) {
            cout << "  ALERTA: se supera la potencia contratada, se apaga la aplazable de mayor potencia" << endl;
        }
    }

    // Opcion 3: simula 24 horas, con gestor y sin gestor
    void simularDia() {
        // Todo empieza de nuevo
        medidor.reiniciar();
        for (int i = 0; i < numCargas; i++) cargas[i]->reiniciar();
        costoSin = 0;
        horasSimuladas = 0;
        fallas = 0;
        emergencia = false;
        produccionIncompleta = false;

        // Potencia de todas las cargas juntas (escenario sin gestor)
        float potenciaTotal = 0;
        for (int i = 0; i < numCargas; i++) potenciaTotal += cargas[i]->getPotencia();

        cout << fixed << setprecision(0);
        cout << setw(5) << "Hora" << setw(8) << "Precio" << setw(8) << "Embal";
        for (int i = 0; i < numCargas; i++) cout << setw(13) << cargas[i]->getNombre();
        cout << setw(9) << "Costo" << endl;

        for (int h = 0; h < HORAS_DIA; h++) {
            // ASOCIACION: el negocio le pide los datos al mercado
            float precio = mercado->precioHora(h);
            float embalses = mercado->nivelEmbalses(h);

            // break: emergencia de racionamiento -> termina la simulacion
            if (precio > PRECIO_EMERGENCIA && embalses < NIVEL_ALERTA_EMBALSES) {
                cout << setw(5) << h << setw(8) << precio << setw(8) << embalses
                     << "  EMERGENCIA DE RACIONAMIENTO: se detiene la simulacion" << endl;
                emergencia = true;
                break;
            }
            horasSimuladas++;

            // Sin gestor: las tres cargas encendidas toda la hora
            costoSin += potenciaTotal * precio;

            // continue: falla la lectura del precio -> solo funciona la carga critica
            if (rand() % FALLA_CADA == 0) {
                float costoFalla = 0;
                for (int i = 0; i < numCargas; i++) {
                    if (i == INDICE_CRITICA) {
                        cargas[i]->encender();
                        costoFalla += cargas[i]->costoHora(precio);
                        medidor.acumular(cargas[i]->getPotencia(), precio);
                    } else {
                        cargas[i]->apagar();
                    }
                }
                fallas++;
                cout << setw(5) << h << setw(8) << "--" << setw(8) << embalses
                     << "  falla de lectura: solo la critica ($" << costoFalla << ")" << endl;
                continue;
            }

            // Con gestor: cada carga decide (polimorfismo) y se aplica la regla 6
            bool decision[MAX_CARGAS];
            bool excedio;
            decidir(h, precio, embalses, decision, excedio);

            float kwHora = 0;
            float costoHora = 0;
            cout << setw(5) << h << setw(8) << precio << setw(8) << embalses;
            for (int i = 0; i < numCargas; i++) {
                if (decision[i]) cargas[i]->encender();
                else cargas[i]->apagar();
                if (cargas[i]->estaEncendida()) kwHora += cargas[i]->getPotencia();
                costoHora += cargas[i]->costoHora(precio);
                cout << setw(13) << (cargas[i]->estaEncendida() ? "ON" : "off");
            }
            // El medidor acumula los kWh y el costo de la hora
            medidor.acumular(kwHora, precio);
            cout << setw(9) << costoHora;
            if (excedio) cout << "  ALERTA: supera potencia contratada";
            cout << endl;
        }

        // Horas minimas: si alguna carga no las cumplio, produccion incompleta
        for (int i = 0; i < numCargas; i++) {
            if (!cargas[i]->cumpleMinimo()) produccionIncompleta = true;
        }
        if (produccionIncompleta) cout << "ALERTA: produccion incompleta" << endl;
        cout << "Simulacion terminada. Vea el resumen con la opcion 4." << endl;
    }

    // Opcion 4: resumen y ahorro
    void resumen() {
        float costoCon = medidor.getCosto();
        cout << fixed << setprecision(0);
        cout << "Negocio " << nombre << endl;
        cout << "Horas simuladas: " << horasSimuladas << " de " << HORAS_DIA
             << " | fallas de lectura: " << fallas << endl;
        if (emergencia) cout << "Hubo emergencia de racionamiento: la simulacion se detuvo." << endl;
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << setw(12) << left << cargas[i]->getNombre() << right
                 << cargas[i]->getHorasUso() << " h, "
                 << cargas[i]->getHorasUso() * cargas[i]->getPotencia() << " kWh" << endl;
        }
        cout << "Energia total: " << medidor.getKWh() << " kWh" << endl;
        cout << "Costo con gestor: $" << costoCon << endl;
        cout << "Costo sin gestor: $" << costoSin << endl;
        cout << "Ahorro:           $" << costoSin - costoCon;
        if (costoSin > 0) cout << " (" << setprecision(1) << (costoSin - costoCon) / costoSin * 100 << " %)";
        cout << endl;
        if (produccionIncompleta) cout << "ALERTA: produccion incompleta" << endl;
    }
};

// main: el programa empieza aqui
int main() {
    srand(SEMILLA);

    // El mercado existe por su cuenta
    Mercado mercado;
    // Las cargas se crean afuera del negocio
    CargaCritica critica(NOMBRE_CRITICA, POTENCIA_CRITICA);
    CargaAplazable aplazable1(NOMBRE_APLAZABLE_1, POTENCIA_APLAZABLE_1, HORAS_MIN_APLAZABLE_1);
    CargaAplazable aplazable2(NOMBRE_APLAZABLE_2, POTENCIA_APLAZABLE_2, HORAS_MIN_APLAZABLE_2);
    // El negocio conoce al mercado (asociacion)
    Negocio negocio(NOMBRE_NEGOCIO, &mercado);
    // Se le agregan las cargas (agregacion); la critica va primero
    negocio.agregarCarga(&critica);
    negocio.agregarCarga(&aplazable1);
    negocio.agregarCarga(&aplazable2);
    negocio.mostrar();

    // Lectura manual: valores de ejemplo
    int hora = 12;
    float precio = 500, embalses = 70;
    bool haySimulacion = false;
    int opcion;

    do {
        cout << endl << "===== WattControl v2 =====" << endl;
        cout << "1. Ingresar lectura manual (hora, precio, embalses)" << endl;
        cout << "2. Evaluar decision de cada carga" << endl;
        cout << "3. Simular un dia completo (24 h)" << endl;
        cout << "4. Ver resumen de la ultima simulacion" << endl;
        cout << "0. Salir" << endl;
        cout << "Opcion: ";
        cin >> opcion;

        switch (opcion) {
        case 1:
            do {
                cout << "Hora (" << HORA_MIN << " a " << HORA_MAX << "): ";
                cin >> hora;
                if (hora < HORA_MIN || hora > HORA_MAX) cout << "  Hora invalida." << endl;
            } while (hora < HORA_MIN || hora > HORA_MAX);
            do {
                cout << "Precio de bolsa (COP/kWh): ";
                cin >> precio;
                if (precio <= 0) cout << "  El precio debe ser mayor que 0." << endl;
            } while (precio <= 0);
            do {
                cout << "Nivel de los embalses (" << EMBALSE_PORC_MIN << " a " << EMBALSE_PORC_MAX << " %): ";
                cin >> embalses;
                if (embalses < EMBALSE_PORC_MIN || embalses > EMBALSE_PORC_MAX) cout << "  Embalses invalidos." << endl;
            } while (embalses < EMBALSE_PORC_MIN || embalses > EMBALSE_PORC_MAX);
            cout << "Lectura guardada." << endl;
            break;
        case 2:
            cout << "Hora " << hora << " | precio " << precio << " | embalses " << embalses << " %" << endl;
            negocio.evaluar(hora, precio, embalses);
            break;
        case 3:
            negocio.simularDia();
            haySimulacion = true;
            break;
        case 4:
            if (haySimulacion) negocio.resumen();
            else cout << "Primero simule un dia con la opcion 3." << endl;
            break;
        case 0:
            cout << "Hasta luego." << endl;
            break;
        default:
            cout << "Opcion invalida." << endl;
        }
        if (opcion != 0) {
            cout << endl << "Presione Enter para continuar...";
            cin.ignore();
            cin.get();
        }
    } while (opcion != 0);

    return 0;
}
