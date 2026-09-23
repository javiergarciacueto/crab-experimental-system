/*
 * C.R.A.B. — Controlled Regulable Agitation Box
 * Arduino controller for programmable mechanical stimulation protocols.
 *
 * Developed by Anabella De Bortoli and Javier García Cueto
 * Laboratorio 6 — Physics Department, FCEyN, Universidad de Buenos Aires
 * Experimental work at IFIBYNE (CONICET-UBA), 2025.
 *
 * This public version preserves the logic of the experimental controller.
 */

// === Comandos disponibles ===
// M0: Velocidad (%) (0-100)
// M1: Cantidad de ciclos
// M2-M3: Tiempo mínimo y máximo de marcha
// M4-M5: Tiempo mínimo y máximo de pausa
// M6-M7: Tiempo mínimo y máximo de descanso
// M8: Duración total de la prueba
// El valor máximo de cada intervalo debe ser mayor o igual al mínimo.
// P: Imprime los parámetros actuales
// R: Reinicia la máquina de estados

// === Librerias ===
#include <EEPROM.h>
#include <avr/wdt.h> 

// === Pines ===
const int entrada1 = 9;
const int entrada2 = 3;  
const int enableA = 10;
const int ledFreno = 8;
const int boton = 6;

// === Variables de configuración ===
int velocidadPWM;
int ciclosMaximos;
unsigned long tMarchaMin, tMarchaMax;
unsigned long tPausaMin, tPausaMax;
unsigned long tDescansoMin, tDescansoMax;
unsigned long tMaxFuncionamiento;

// === Dirección base EEPROM ===
const int DIR_BASE = 0;
const int BYTES_INT = 2;
const int BYTES_ULONG = 4;

// === Variables de control ===
unsigned long tiempoAnterior = 0;
unsigned long inicioExperimento = 0;

bool ultimoEstadoBoton = false;
bool estadoEstableBoton = HIGH;
unsigned long ultimoCambioBoton = 0;
unsigned int tiempoDebounce = 5;

unsigned long ultimoParpadeo = 0;
bool estadoLed = false;

unsigned long tiempoMarcha, tiempoPausa, descansoLargo;
int cicloActual = 0;

bool enModoManual = false;
bool tiempoMaximoAlcanzado = false;


// === Maquina de estados ===
enum Estado {
  ESPERA_INICIAL,
  MARCHA,
  PAUSA,
  DESCANSO,
  MODO_MANUAL,
  APAGADO
};
Estado estadoActual = ESPERA_INICIAL;
Estado estadoPrevio = ESPERA_INICIAL;

// === Funciones de los parámetros variables ===
void escribirParametro(int index, unsigned long valor) {
  int offset;
  if (index == 0) {
    offset = DIR_BASE;
    EEPROM.put(offset, (int)valor);
  } else if (index == 1) {
    offset = DIR_BASE + BYTES_INT;
    EEPROM.put(offset, (int)valor);
  } else {
    offset = DIR_BASE + 2*BYTES_INT + (index - 2) * BYTES_ULONG;
    EEPROM.put(offset, valor);
  }
}

void leerParametros() {
  int offset = DIR_BASE;
  EEPROM.get(offset, velocidadPWM); offset += BYTES_INT;
  EEPROM.get(offset, ciclosMaximos); offset += BYTES_INT;
  EEPROM.get(offset, tMarchaMin); offset += BYTES_ULONG;
  EEPROM.get(offset, tMarchaMax); offset += BYTES_ULONG;
  EEPROM.get(offset, tPausaMin);  offset += BYTES_ULONG;
  EEPROM.get(offset, tPausaMax);  offset += BYTES_ULONG;
  EEPROM.get(offset, tDescansoMin); offset += BYTES_ULONG;
  EEPROM.get(offset, tDescansoMax); offset += BYTES_ULONG;
  EEPROM.get(offset, tMaxFuncionamiento);
}

void printParametros() {
  Serial.println(F("--------------------------"));
  Serial.print(F("Velocidad (%) = "));
  Serial.println(velocidadPWM);
  Serial.print(F("Ciclos = "));
  Serial.println(ciclosMaximos);
  Serial.print(F("Marcha min (ms) = "));
  Serial.println(tMarchaMin);
  Serial.print(F("Marcha max (ms) = "));
  Serial.println(tMarchaMax);
  Serial.print(F("Pausa min (ms) = "));
  Serial.println(tPausaMin);
  Serial.print(F("Pausa max (ms) = "));
  Serial.println(tPausaMax);
  Serial.print(F("Descanso min (ms) = "));
  Serial.println(tDescansoMin);
  Serial.print(F("Descanso max (ms) = "));
  Serial.println(tDescansoMax);
  Serial.print(F("Tiempo max funcionamiento (ms) = "));
  Serial.println(tMaxFuncionamiento);
  Serial.println(F("--------------------------"));
}

void validarYGuardarParametro(int index, unsigned long valor) {
  bool valido = false;
  String errorMsg = "Error";
  
  switch(index) {
    case 0:
      valido = (valor <= 100);
      errorMsg = F("ERROR: Ingrese porcentaje válido (0-100)");
      break;
    case 1:
      valido = true;
      break;
    case 2:
      valido = true;
      break;
    case 3:
      valido = (valor >= tMarchaMin);
      errorMsg = F("ERROR: Tiempo max debe ser >= min");
      break;
    case 4:
      valido = true;
      break;
    case 5:
      valido = (valor >= tPausaMin);
      errorMsg = F("ERROR: Tiempo max debe ser >= min");
      break;
    case 6:
      valido = true;
      break;
    case 7:
      valido = (valor >= tDescansoMin);
      errorMsg = F("ERROR: Tiempo max debe ser >= min");
      break;
    case 8:
      valido = true;
      break;
  }
  
  if (valido) {
    escribirParametro(index, valor);
    leerParametros();
    Serial.print(F("Guardado M"));
    Serial.print(index);
    Serial.print(F(" = "));
    Serial.println(valor);
  } else {
    Serial.println(errorMsg);
  }
}

// === Funciones para comandos y comunicacion serial ===
void imprimirModo() {
  Serial.print(F("Estado actual: "));
  switch(estadoActual) {
    case ESPERA_INICIAL: Serial.println(F("ESPERA_INICIAL")); break;
    case MARCHA: Serial.println(F("MARCHA")); break;
    case PAUSA: Serial.println(F("PAUSA")); break;
    case DESCANSO: Serial.println(F("DESCANSO")); break;
    case MODO_MANUAL: Serial.println(F("MODO_MANUAL")); break;
    case APAGADO: Serial.println(F("APAGADO")); break;
  }
}

void reiniciarSistema() {
  estadoActual = ESPERA_INICIAL;
  cicloActual = 0;
  tiempoAnterior = millis();
  inicioExperimento = millis();
  tiempoMaximoAlcanzado = false;
  imprimirModo();
}

void ejecutarComando(String comando) {
  comando.trim();
  
  if (comando.startsWith("M")) {
    int index = comando.substring(1).toInt();
    if (index < 0 || index > 8) {
      Serial.println(F("ERROR: índice fuera de rango (0-8)"));
      return;
    }
    
    Serial.print(F("Ingrese el valor para "));
    switch(index) {
      case 0: Serial.println(F("velocidad (%)")); break;
      case 1: Serial.println(F("ciclos")); break;
      case 2: Serial.println(F("tiempo minimo de marcha (ms)")); break;
      case 3: Serial.println(F("tiempo maximo de marcha (ms)")); break;
      case 4: Serial.println(F("tiempo minimo de pausa (ms)")); break;
      case 5: Serial.println(F("tiempo maximo de pausa (ms)")); break;
      case 6: Serial.println(F("tiempo minimo de descanso (ms)")); break;
      case 7: Serial.println(F("tiempo maximo de descanso (ms)")); break;
      case 8: Serial.println(F("tiempo maximo de funcionamiento (ms)")); break;
    }
    
    unsigned long startWait = millis();
    while (!Serial.available() && (millis() - startWait < 7000)) {
      wdt_reset();
      delay(10);
    }
    
    if (Serial.available()) {
      unsigned long valor = Serial.parseInt();
      validarYGuardarParametro(index, valor);
    } else {
      Serial.println(F("Timeout: No se recibió input"));
    }
    
    while (Serial.available()) Serial.read();
    
  } else if (comando.startsWith("R")) {
    Serial.println(F("Reiniciando..."));
    reiniciarSistema();
    
  } else if (comando.startsWith("P")) {
    printParametros();
  
  }else if (comando.startsWith("T")){
    Serial.print(F("Tiempo transcurrido: "));
    Serial.print((millis() - inicioExperimento)/60000,0);
    Serial.print(":");
    Serial.println(((millis() - inicioExperimento) % 60000 )/1000);
  }
}

// === Funciones de motor, LED y botón ===

void motorMarcha() {
  int pwm = map(velocidadPWM, 0, 100, 0, 255);
  analogWrite(entrada1, pwm);
  digitalWrite(entrada2, LOW);
  digitalWrite(enableA, HIGH);
  digitalWrite(ledFreno, LOW);
}

void motorParado() {
  analogWrite(entrada1, 0);
  digitalWrite(entrada2, LOW);
  digitalWrite(enableA, HIGH);
  digitalWrite(ledFreno, LOW);
}

void motorDescanso() {
  analogWrite(entrada1, 0);
  digitalWrite(entrada2, LOW);
  digitalWrite(enableA, HIGH);
  digitalWrite(ledFreno, HIGH);
}

void motorApagado() {
  analogWrite(entrada1, 0);
  digitalWrite(entrada2, LOW);
  digitalWrite(enableA, LOW);
  digitalWrite(ledFreno, LOW);
}


bool botonPresionado() {
  bool lectura = digitalRead(boton);
  if (lectura != ultimoEstadoBoton) {
    ultimoCambioBoton = millis();
    ultimoEstadoBoton = lectura;
  }
  if ((millis() - ultimoCambioBoton) > tiempoDebounce) {
    if (estadoEstableBoton != lectura) {
      estadoEstableBoton = lectura;
    }
  }
  return estadoEstableBoton == LOW;
}

// === Funciones de control temporal ===
// Función segura para comparación de tiempo
bool tiempoCumplido(unsigned long inicio, unsigned long duracion) {
  return (millis() - inicio) >= duracion;
}


void verificarTiempoMaximo() {
  if (!tiempoMaximoAlcanzado && tMaxFuncionamiento > 0 && 
      tiempoCumplido(inicioExperimento, tMaxFuncionamiento)) {
    tiempoMaximoAlcanzado = true;
    motorApagado();
    Serial.println(F("Tiempo máximo alcanzado, pasando a APAGADO"));
    estadoActual = APAGADO;
  }
}

// Función para alimentar el watchdog
void alimentarWatchdog() {
  asm volatile ("wdr"); // Watchdog reset instruction
}


// === Máquina de estados ===

void maquinaEstados() {
  switch (estadoActual) {
    case ESPERA_INICIAL:
      motorParado();
      if (tiempoCumplido(tiempoAnterior, 1000)) {
        estadoActual = MARCHA;
        imprimirModo();
        tiempoAnterior = millis();
        tiempoMarcha = random(tMarchaMin, tMarchaMax + 1);
      }
      break;

    case MARCHA:
      motorMarcha();
      if (tiempoCumplido(tiempoAnterior, tiempoMarcha)) {
        estadoActual = PAUSA;
        imprimirModo();
        tiempoAnterior = millis();
        tiempoPausa = random(tPausaMin, tPausaMax + 1);
      }
      break;

    case PAUSA:
      motorParado();
      if (tiempoCumplido(tiempoAnterior, tiempoPausa)) {
        cicloActual++;
       
        if (cicloActual >= ciclosMaximos) {
          estadoActual = DESCANSO;
          imprimirModo();
          tiempoAnterior = millis();
          cicloActual = 0;
          descansoLargo = random(tDescansoMin, tDescansoMax + 1);
          Serial.print(F("Tiempo de descanso elegido (min): "));
          Serial.print(descansoLargo / 60000.0, 0);
          Serial.print(":");
          Serial.println((descansoLargo % 60000 )/1000);
        } else {
          estadoActual = MARCHA;
          imprimirModo();
          tiempoAnterior = millis();
          tiempoMarcha = random(tMarchaMin, tMarchaMax + 1);
        }
      }
      break;

    case DESCANSO:
      motorDescanso();
      if (tiempoCumplido(tiempoAnterior, descansoLargo)) {
        estadoActual = MARCHA;
        imprimirModo();
        tiempoAnterior = millis();
        tiempoMarcha = random(tMarchaMin, tMarchaMax + 1);
      }
      break;

    case MODO_MANUAL:
      motorMarcha();
      if (tiempoCumplido(ultimoParpadeo, 250)) {
        ultimoParpadeo = millis();
        estadoLed = !estadoLed;
        digitalWrite(ledFreno, estadoLed);
      }
      break;
    
    case APAGADO:
      motorApagado();
      wdt_disable();
      while(true){
        delay(1000);
        if(Serial.available()){
          String linea = Serial.readStringUntil("\n");
          linea.trim();
          if(linea.startsWith("R")){
            Serial.println("Reiniciando desde APAGADO...");
            asm volatile ("jmp 0");
          }
        }
      }
  }
}


// === Setup ===
void setup() {
  
  // Leer motivo de reset INMEDIATAMENTE
  uint8_t mcusr = MCUSR;
  MCUSR = 0;
  wdt_disable();
  Serial.begin(9600);
  delay(100);
  
  // Verificar razón de reset
  Serial.print(F("MCUSR: 0x")); 
  Serial.println(mcusr, HEX);
  if (mcusr & (1<<PORF)) Serial.println(F("Power-on Reset"));
  if (mcusr & (1<<EXTRF)) Serial.println(F("External Reset (pin)"));
  if (mcusr & (1<<BORF)) Serial.println(F("*** BROWN-OUT RESET ***"));
  if (mcusr & (1<<WDRF)) Serial.println(F("Watchdog Reset"));
  if (mcusr == 0x0) { Serial.println(F("Reset por software"));}
  
  Serial.println(F("Inicializando..."));
  // Configurar pines
  pinMode(entrada1, OUTPUT);
  pinMode(entrada2, OUTPUT);
  pinMode(enableA, OUTPUT);
  pinMode(ledFreno, OUTPUT);
  pinMode(boton, INPUT_PULLUP);

  motorApagado();
  leerParametros();
  printParametros();
  tiempoAnterior = millis();
  inicioExperimento = millis();
  wdt_enable(WDTO_4S); // Watchdog con timeout de 4 s.
}


// === Loop ===
void loop() {

  alimentarWatchdog();
  verificarTiempoMaximo();
  maquinaEstados();
  delay(1); // Evita iteraciones innecesariamente rápidas del loop principal.

  if (Serial.available()) {
    String linea = Serial.readStringUntil("\n");
    ejecutarComando(linea);
    while (Serial.available()) Serial.read();
  }

  if (botonPresionado()) {
    if (!enModoManual) {
      enModoManual = true;
      estadoPrevio = estadoActual;
      estadoActual = MODO_MANUAL;
      imprimirModo();
      tiempoAnterior = millis();
    }
  } else {
    if (enModoManual) {
      enModoManual = false; 
      estadoActual = estadoPrevio;
      imprimirModo();
      tiempoAnterior = millis();
      digitalWrite(ledFreno, LOW);
    }
  }
}
