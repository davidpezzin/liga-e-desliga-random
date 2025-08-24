#ifndef D8
#define D8 8
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#define MINUTOS(x) ((unsigned long)(x) * 60000UL)

// Pinos
const uint8_t RELAY_PIN         = D8;
const uint8_t LED_RELE_PIN      = LED_BUILTIN; // LED que acende quando o relé está LIGADO
const uint8_t HEARTBEAT_LED_PIN = 12;          // LED que pisca continuamente
const uint8_t POT_PIN           = A0;          // Cursor do potenciômetro

// Configurações elétricas
const bool RELAY_ATIVO_EM_LOW      = true;  // true: relé aciona com LOW (comum)
const bool LED_RELE_ATIVO_EM_HIGH  = true;  // true: LED acende com HIGH
const bool HB_LED_ATIVO_EM_HIGH    = true;  // true: LED acende com HIGH

// Faixa do ADC (ajuste conforme a placa)
const int POT_ADC_MAX = 1023; // 1023 (UNO), 4095 (ESP32)

// Controle do relé e tempos
bool relayLigado = false;
unsigned long proximaTrocaMs = 0;

// Heartbeat (pisca contínuo)
bool hbOn = false;
unsigned long hbPeriodoMs = 500;     // pisca a cada 500 ms
unsigned long hbProximaTrocaMs = 0;

// Telemetria do potenciômetro
unsigned long potPrintPeriodoMs = 1000; // imprime a cada 1s
unsigned long potProximaImpressaoMs = 0;

// Escrita digital (HIGH/LOW)
inline void escreveRelay(bool ligar) {
  if (ligar) {
    digitalWrite(RELAY_PIN, RELAY_ATIVO_EM_LOW ? LOW : HIGH);
    digitalWrite(LED_RELE_PIN, LED_RELE_ATIVO_EM_HIGH ? HIGH : LOW);
  } else {
    digitalWrite(RELAY_PIN, RELAY_ATIVO_EM_LOW ? HIGH : LOW);
    digitalWrite(LED_RELE_PIN, LED_RELE_ATIVO_EM_HIGH ? LOW : HIGH);
  }
}

inline void escreveHeartbeat(bool ligar) {
  digitalWrite(HEARTBEAT_LED_PIN, ligar ? (HB_LED_ATIVO_EM_HIGH ? HIGH : LOW)
                                        : (HB_LED_ATIVO_EM_HIGH ? LOW  : HIGH));
}

// Lê potenciômetro (raw) e limita 0..POT_ADC_MAX
int lerPotRaw() {
  int leitura = analogRead(POT_PIN);
  return constrain(leitura, 0, POT_ADC_MAX);
}

// A partir de uma leitura ADC, calcula a faixa [minMin, maxMax] em minutos
// pot mínimo  -> min=1,  max=8
// pot máximo  -> min=10, max=25
void faixaPorLeitura(int leitura, uint8_t &minMin, uint8_t &maxMax) {
  uint8_t minCalc = (uint8_t) map(leitura, 0, POT_ADC_MAX, 1, 10);
  uint8_t maxCalc = (uint8_t) map(leitura, 0, POT_ADC_MAX, 8, 25);
  if (maxCalc < minCalc) maxCalc = minCalc; // segurança
  minMin = minCalc;
  maxMax = maxCalc;
}

// Versão que usa a leitura atual do POT
void leFaixaDoPot(uint8_t &minMin, uint8_t &maxMax) {
  int leitura = lerPotRaw();
  faixaPorLeitura(leitura, minMin, maxMax);
}

// Agenda o próximo intervalo (em minutos) conforme a faixa do potenciômetro
void agendaProximoIntervalo(bool proximoEstadoLigado) {
  uint8_t minMin, maxMax;
  leFaixaDoPot(minMin, maxMax);

  unsigned long minutos = (unsigned long) random((long)minMin, (long)maxMax + 1);
  unsigned long agora = millis();
  proximaTrocaMs = agora + MINUTOS(minutos);

  // Log da agenda
  Serial.print("POT-> faixa(min): ");
  Serial.print(minMin);
  Serial.print("..");
  Serial.print(maxMax);
  Serial.print(" | Proximo estado: ");
  Serial.print(proximoEstadoLigado ? "LIGADO" : "DESLIGADO");
  Serial.print(" por ");
  Serial.print(minutos);
  Serial.println(" min");
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_RELE_PIN, OUTPUT);
  pinMode(HEARTBEAT_LED_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);

  // Estados iniciais
  relayLigado = false;
  escreveRelay(relayLigado);

  hbOn = false;
  escreveHeartbeat(hbOn);

  Serial.begin(115200);
  delay(50);

  // Semente para aleatoriedade (usa ruído de A0 + micros)
  unsigned long seed = micros();
  #if defined(A0)
    seed ^= analogRead(A0);
  #endif
  randomSeed(seed);

  unsigned long agora = millis();
  hbProximaTrocaMs = agora + hbPeriodoMs;
  potProximaImpressaoMs = agora + potPrintPeriodoMs;

  Serial.println("Simulador de presença iniciado (millis-based).");
  Serial.println("- Pot min: 1..8 min | Pot max: 10..25 min.");
  Serial.println("- LED de estado LIGA com o relé.");
  Serial.println("- LED heartbeat pisca continuamente.");
  Serial.println("- Telemetria do potenciometro impressa a cada 1s.");

  // Agenda o primeiro período (a partir do estado atual DESLIGADO -> próximo será LIGADO)
  agendaProximoIntervalo(true);
}

void loop() {
  unsigned long agora = millis();

  // Pisca contínuo do heartbeat
  if ((long)(agora - hbProximaTrocaMs) >= 0) {
    hbOn = !hbOn;
    escreveHeartbeat(hbOn);
    hbProximaTrocaMs = agora + hbPeriodoMs;
  }

  // Troca do estado do relé conforme janela aleatória dentro da faixa
  if ((long)(agora - proximaTrocaMs) >= 0) {
    relayLigado = !relayLigado;
    escreveRelay(relayLigado);
    agendaProximoIntervalo(!relayLigado);
  }

  // Telemetria do potenciômetro (mostra leitura e faixa mapeada)
  if ((long)(agora - potProximaImpressaoMs) >= 0) {
    int leitura = lerPotRaw();
    uint8_t minMin, maxMax;
    faixaPorLeitura(leitura, minMin, maxMax);
    unsigned long perc = (unsigned long)leitura * 100UL / POT_ADC_MAX;

    Serial.print("POT ADC: ");
    Serial.print(leitura);
    Serial.print(" (");
    Serial.print(perc);
    Serial.print("%) -> faixa(min): ");
    Serial.print(minMin);
    Serial.print("..");
    Serial.println(maxMax);

    potProximaImpressaoMs = agora + potPrintPeriodoMs;
  }
}
