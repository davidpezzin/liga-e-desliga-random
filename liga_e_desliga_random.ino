/*
  Simulador de presença: relé em D8 alterna entre LIGADO e DESLIGADO
  por intervalos aleatórios entre 3 e 8 minutos.

  Observações:
  - Se o seu módulo relé for "ativo em LOW" (muito comum), deixe RELAY_ATIVO_EM_LOW = true.
    Caso contrário, coloque false.
  - Em placas ESP8266/ESP32 você pode usar D8; em Arduino Uno/Nano use 8.
*/

#ifndef D8
#define D8 8
#endif

const uint8_t RELAY_PIN = D8;
const bool RELAY_ATIVO_EM_LOW = false;  // true: aciona com LOW (comum); false: aciona com HIGH

inline void ligaRele()   { digitalWrite(RELAY_PIN, RELAY_ATIVO_EM_LOW ? LOW  : HIGH); }
inline void desligaRele(){ digitalWrite(RELAY_PIN, RELAY_ATIVO_EM_LOW ? HIGH : LOW ); }

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  desligaRele();

  Serial.begin(115200);
  delay(50);

  // Semente de aleatoriedade: use um pino analógico "flutuante" se possível.
  // Combina analogRead(A0) com micros() para melhorar a entropia.
  unsigned long seed = 0;
  #if defined(A0)
    seed ^= analogRead(A0);
  #endif
  seed ^= micros();
  randomSeed(seed);

  Serial.println("Simulador de presença iniciado.");
}

void loop() {
  // Intervalos aleatórios entre 3 e 8 minutos (random(3,9) -> 3..8)
  unsigned long minutosLigado   = random(3, 9);
  unsigned long minutosDesligado= random(3, 9);

  // Liga
  Serial.print("Ligado por ");
  Serial.print(minutosLigado);
  Serial.println(" minuto(s).");
  ligaRele();
  delay(minutosLigado * 60000UL);

  // Desliga
  Serial.print("Desligado por ");
  Serial.print(minutosDesligado);
  Serial.println(" minuto(s).");
  desligaRele();
  delay(minutosDesligado * 60000UL);
}