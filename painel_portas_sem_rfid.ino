/*
  ============================================================
  Painel de Controle de Portas - VERSÃO PROVISÓRIA SEM RFID
  ============================================================
  Igual ao projeto completo, mas sem o leitor RFID: o menu
  aparece direto na tela assim que o Arduino liga, sem precisar
  aproximar nenhuma tag. Útil para testar display, servos e
  botão isoladamente antes de juntar o RC522.

  - Display colorido 0.96" 160x80 (controlador ST7735S, SPI)
  - 3 Motores Servo SG90 (POSICIONAL) -> cada um abre/fecha uma porta
    girando apenas entre 0 graus (fechada) e 90 graus (aberta).
  - 1 Botão único (INPUT_PULLUP, outro lado no GND):
      * Tela inicial -> 2 cliques RÁPIDOS (até 400ms entre eles) abrem
        o painel de opções
      * Painel de opções aberto:
          - Clique CURTO -> navega para a próxima opção (cíclico)
          - Clique LONGO (segurar) -> abre a porta selecionada se
            estiver fechada, ou fecha se já estiver aberta
      * Se ninguém fechar manualmente, a porta fecha sozinha depois
        de 3 minutos. Enquanto está aberta, o menu mostra a
        contagem regressiva (mm:ss) no lugar de "Fechada", e o painel
        não volta pra tela inicial sozinho.
      * Sem porta aberta, o painel volta pra tela inicial depois de
        30s sem uso.

  BIBLIOTECAS NECESSÁRIAS (Gerenciador de Bibliotecas da IDE Arduino):
  - Adafruit GFX Library
  - Adafruit ST7735 and ST7789 Library
  - Servo (já vem instalada por padrão)
  (repare que a MFRC522 NÃO é necessária nesta versão)

  LIGAÇÕES (Arduino UNO R3):
  ------------------------------------------------------------
  Display 160x80 ST7735S (SPI):
    GND      -> GND
    VCC      -> 3.3V
    SCL/SCK  -> D13
    SDA/MOSI -> D11
    RES/RST  -> D6
    DC/A0    -> D7
    CS       -> D8
    BLK      -> 3.3V

  Botão único (um lado no pino, outro no GND, usa pull-up interno):
    Botão -> D2

  Servos SG90 (fio de sinal laranja/amarelo):
    Servo Porta 1 -> D3
    Servo Porta 2 -> D4
    Servo Porta 3 -> D5
    IMPORTANTE: alimente os servos com uma fonte externa de 5-6V
    (mínimo 2A). NÃO use o 5V do Arduino para os 3 servos.
    Una o GND da fonte externa ao GND do Arduino.
    Capacitor de 470-1000uF entre V+ e GND da fonte dos servos.
  ------------------------------------------------------------
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Servo.h>

// ---------- Configuração do Display ----------
#define TFT_CS   8
#define TFT_DC   7
#define TFT_RST  6

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int TELA_LARGURA = 160;
const int TELA_ALTURA  = 80;

// ---------- Configuração do Botão ----------
#define BOTAO_PIN 2
const unsigned long TEMPO_CLIQUE_LONGO = 600;   // ms para considerar "segurar"
const unsigned long TEMPO_DEBOUNCE     = 40;    // ms de debounce
const unsigned long JANELA_DUPLO_CLIQUE = 400;  // ms máximo entre os 2 cliques

bool estadoBotaoAnterior = HIGH;
bool botaoPressionado = false;
bool cliqueLongoDisparado = false;
unsigned long momentoPressionado = 0;
unsigned long momentoUltimaMudanca = 0;
unsigned long momentoUltimoCliqueCurto = 0; // para detectar o 2º clique do duplo clique

// ---------- Estado do menu ----------
bool menuAtivo = false; // false = tela inicial, true = painel de opções visível
const unsigned long TIMEOUT_MENU = 30000; // volta pra tela inicial após 30s sem uso (só se nenhuma porta estiver aberta)
unsigned long tempoAtividadeMenu = 0;

// ---------- Configuração dos Servos ----------
#define SERVO_PORTA1 3
#define SERVO_PORTA2 4
#define SERVO_PORTA3 5

Servo servoPorta1;
Servo servoPorta2;
Servo servoPorta3;

// Servo posicional: gira SOMENTE entre esses dois ângulos, nunca mais
// que isso. 0 = porta fechada, 90 = porta aberta.
const int ANGULO_FECHADO = 90;
const int ANGULO_ABERTO  = 0;
const unsigned long TEMPO_LIMITE_ABERTA = 180000UL; // 3 minutos até fechar sozinha

int opcaoSelecionada = 0; // 0 = Porta 1, 1 = Porta 2, 2 = Porta 3
bool portaAberta[3] = {false, false, false};
unsigned long momentoAbertura[3] = {0, 0, 0};
unsigned long ultimaAtualizacaoMenu = 0;

void setup() {
  Serial.begin(9600);

  pinMode(BOTAO_PIN, INPUT_PULLUP);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  posicionarServosFechados();

  tft.initR(INITR_MINI160x80_PLUGIN); // troque para INITR_MINI160x80 se as cores saírem erradas
  tft.setRotation(1);                 // teste 0, 1, 2 ou 3 se a orientação estiver errada
  tft.invertDisplay(true);            // troque para false se as cores saírem no negativo
  tft.fillScreen(ST77XX_BLACK);

  resetarEstadoBotao();
  mostrarTelaInicial(); // tela de espera; dê 2 cliques rápidos para abrir o menu
}

void loop() {
  verificarTimeoutPortas();
  lerBotao();
  if (menuAtivo) {
    atualizarContagemNoMenu();
    verificarTimeoutMenu();
  }
}

// ---------------- Tela ----------------
void mostrarTelaInicial() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.setCursor(18, 10);
  tft.print("Para");
  tft.setCursor(18, 32);
  tft.print("Daniely");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(14, 60);
  tft.print("Clique 2x: abrir menu");
}

void mostrarMenu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(2, 2);
  tft.print("Clique: navega");
  tft.setCursor(2, 12);
  tft.print("Segure: abre/fecha");
  tft.drawFastHLine(0, 24, TELA_LARGURA, ST77XX_WHITE);

  const char* nomes[3] = {"Porta 1", "Porta 2", "Porta 3"};
  const int posY[3] = {32, 48, 64};

  for (int i = 0; i < 3; i++) {
    if (i == opcaoSelecionada) {
      tft.fillRect(0, posY[i] - 3, TELA_LARGURA, 15, ST77XX_BLUE);
      tft.setTextColor(ST77XX_WHITE);
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setTextSize(1);
    tft.setCursor(6, posY[i]);
    tft.print(i == opcaoSelecionada ? "> " : "  ");
    tft.print(nomes[i]);

    tft.setTextColor(portaAberta[i] ? ST77XX_GREEN : ST77XX_RED);
    tft.setCursor(100, posY[i]);
    if (portaAberta[i]) {
      tft.print(formatarTempoRestante(i));
    } else {
      tft.print("Fechada");
    }
  }
}

String formatarTempoRestante(int indice) {
  unsigned long decorrido = millis() - momentoAbertura[indice];
  unsigned long restante = (decorrido < TEMPO_LIMITE_ABERTA) ? (TEMPO_LIMITE_ABERTA - decorrido) : 0;
  unsigned int segundosTotais = restante / 1000;
  unsigned int minutos = segundosTotais / 60;
  unsigned int segundos = segundosTotais % 60;

  char buffer[6];
  sprintf(buffer, "%01u:%02u", minutos, segundos);
  return String(buffer);
}

void mostrarMensagemAbrindo(int numeroPorta) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(14, 18);
  tft.print("Abrindo");
  tft.setCursor(14, 42);
  tft.print("Porta ");
  tft.print(numeroPorta);
}

void mostrarMensagemFechando(int numeroPorta) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(14, 18);
  tft.print("Fechando");
  tft.setCursor(14, 42);
  tft.print("Porta ");
  tft.print(numeroPorta);
}

// ---------------- Botão ----------------
void resetarEstadoBotao() {
  estadoBotaoAnterior = digitalRead(BOTAO_PIN);
  botaoPressionado = false;
  cliqueLongoDisparado = false;
  momentoUltimaMudanca = millis();
}

// Chamado depois de executar uma ação de clique longo (abrir/fechar).
// Se o dedo ainda estiver no botão nesse momento, NÃO trata como um
// clique novo -- marca que o clique longo já foi "consumido" e só
// aceita a próxima ação depois que o botão for solto de verdade.
// Sem isso, o código reabria/refechava a porta sozinho logo em seguida.
void finalizarAposAcao() {
  bool aindaPressionado = (digitalRead(BOTAO_PIN) == LOW);
  estadoBotaoAnterior = aindaPressionado ? LOW : HIGH;
  botaoPressionado = aindaPressionado;
  cliqueLongoDisparado = aindaPressionado; // impede repetir a ação enquanto seguro
  momentoUltimaMudanca = millis();
}

void lerBotao() {
  bool leituraAtual = digitalRead(BOTAO_PIN);

  if (leituraAtual != estadoBotaoAnterior) {
    momentoUltimaMudanca = millis();
    estadoBotaoAnterior = leituraAtual;
  }

  if ((millis() - momentoUltimaMudanca) > TEMPO_DEBOUNCE) {
    if (leituraAtual == LOW && !botaoPressionado) {
      botaoPressionado = true;
      cliqueLongoDisparado = false;
      momentoPressionado = millis();
    }

    // Clique longo só tem efeito com o menu já aberto (abre/fecha a porta
    // selecionada). Na tela inicial, segurar o botão não faz nada.
    if (menuAtivo && leituraAtual == LOW && botaoPressionado && !cliqueLongoDisparado) {
      if (millis() - momentoPressionado >= TEMPO_CLIQUE_LONGO) {
        cliqueLongoDisparado = true;
        confirmarSelecao();
        return;
      }
    }

    if (leituraAtual == HIGH && botaoPressionado) {
      botaoPressionado = false;
      if (!cliqueLongoDisparado) {
        unsigned long agora = millis();

        if (menuAtivo) {
          // Menu já aberto -> clique curto sempre navega
          opcaoSelecionada = (opcaoSelecionada + 1) % 3;
          tempoAtividadeMenu = agora;
          mostrarMenu();
        } else {
          // Tela inicial -> só reage se for o 2º clique dentro da janela
          if (agora - momentoUltimoCliqueCurto <= JANELA_DUPLO_CLIQUE) {
            menuAtivo = true;
            opcaoSelecionada = 0;
            tempoAtividadeMenu = agora;
            mostrarMenu();
          }
        }

        momentoUltimoCliqueCurto = agora;
      }
    }
  }
}

// ---------------- Servos ----------------
void posicionarServosFechados() {
  servoPorta1.attach(SERVO_PORTA1);
  servoPorta2.attach(SERVO_PORTA2);
  servoPorta3.attach(SERVO_PORTA3);
  servoPorta1.write(ANGULO_FECHADO);
  servoPorta2.write(ANGULO_FECHADO);
  servoPorta3.write(ANGULO_FECHADO);
  delay(600); // tempo para o curso completo
  servoPorta1.detach();
  servoPorta2.detach();
  servoPorta3.detach();
}

void atualizarContagemNoMenu() {
  bool algumaAberta = portaAberta[0] || portaAberta[1] || portaAberta[2];
  if (!algumaAberta) return;

  if (millis() - ultimaAtualizacaoMenu >= 1000) {
    ultimaAtualizacaoMenu = millis();
    mostrarMenu();
  }
}

// Volta para a tela inicial após TIMEOUT_MENU sem uso, mas só se
// nenhuma porta estiver aberta (senão a contagem regressiva some).
void verificarTimeoutMenu() {
  bool algumaAberta = portaAberta[0] || portaAberta[1] || portaAberta[2];
  if (algumaAberta) return;

  if (millis() - tempoAtividadeMenu > TIMEOUT_MENU) {
    menuAtivo = false;
    mostrarTelaInicial();
  }
}

void verificarTimeoutPortas() {
  for (int i = 0; i < 3; i++) {
    if (portaAberta[i] && (millis() - momentoAbertura[i] >= TEMPO_LIMITE_ABERTA)) {
      fecharPortaAutomaticamente(i);
    }
  }
}

void fecharPortaAutomaticamente(int indice) {
  const int pinosServo[3] = {SERVO_PORTA1, SERVO_PORTA2, SERVO_PORTA3};
  Servo* servos[3] = {&servoPorta1, &servoPorta2, &servoPorta3};
  Servo* s = servos[indice];

  mostrarMensagemFechando(indice + 1);
  s->attach(pinosServo[indice]);
  s->write(ANGULO_FECHADO);
  delay(600);
  s->detach(); // solta o sinal -> servo para de se mover, sem jitter
  portaAberta[indice] = false;

  tempoAtividadeMenu = millis();
  mostrarMenu();
}

void confirmarSelecao() {
  const int pinosServo[3] = {SERVO_PORTA1, SERVO_PORTA2, SERVO_PORTA3};
  Servo* servos[3] = {&servoPorta1, &servoPorta2, &servoPorta3};
  int numeroPorta = opcaoSelecionada + 1;
  Servo* s = servos[opcaoSelecionada];

  if (!portaAberta[opcaoSelecionada]) {
    // Estava fechada (0 graus) -> abre (vai até 90 graus)
    mostrarMensagemAbrindo(numeroPorta);
    s->attach(pinosServo[opcaoSelecionada]);
    s->write(ANGULO_ABERTO);
    delay(600); // tempo para o curso completo (0 -> 90)
    s->detach(); // solta o sinal -> servo para de se mover, sem jitter
    momentoAbertura[opcaoSelecionada] = millis();
    portaAberta[opcaoSelecionada] = true;
  } else {
    // Estava aberta (90 graus) -> fecha (volta para 0 graus)
    mostrarMensagemFechando(numeroPorta);
    s->attach(pinosServo[opcaoSelecionada]);
    s->write(ANGULO_FECHADO);
    delay(600); // tempo para o curso completo (90 -> 0)
    s->detach();
    portaAberta[opcaoSelecionada] = false;
  }

  tempoAtividadeMenu = millis();
  finalizarAposAcao(); // evita reabrir/refechar sozinho se o botão ainda estiver pressionado
  mostrarMenu();
}
