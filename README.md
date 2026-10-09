# 🔐 Painel de Controle de Portas com Arduino

Caixa de joias de madeira (MDF) com **3 portas controladas por servos**, **um único botão** e um **display colorido** que mostra o menu e o estado de cada porta. Este repositório traz a **versão provisória sem RFID**, ideal para validar display, servos e lógica do botão antes de integrar o leitor de tags.

![Infográfico do projeto](docs/infografico.png)

## 💡 Motivação

Este projeto nasceu de uma situação do dia a dia: minha namorada, Daniely, ficava incomodada com pessoas mexendo nas coisas dela. A partir dessa insatisfação surgiu a ideia de criar uma caixa de joias que mantém tudo guardado e só abre quando ela decide, com controle individual de cada porta e fechamento automático.

O que começou como uma solução para um problema real virou também um projeto de estudo de eletrônica, programação embarcada e interação com um único botão.

## ✨ Funcionalidades

- Tela inicial personalizável (mensagem de boas-vindas)
- **2 cliques rápidos** abrem o painel de opções
- **Clique curto** navega entre as 3 portas
- **Clique longo (segurar)** abre ou fecha a porta selecionada
- **Fechamento automático** após 3 minutos, com **contagem regressiva** (`mm:ss`) no display
- O painel volta sozinho para a tela inicial após 30 s sem uso (se nenhuma porta estiver aberta)
- Debounce por software e proteção contra reabrir/fechar sozinho com o botão ainda pressionado

## 🧰 Componentes

| Qtd | Item |
|---|---|
| 1 | Arduino Uno R3 |
| 1 | Display TFT 0,96" 160x80 (ST7735S, SPI) |
| 3 | Servo SG90 (posicional) |
| 1 | Botão (usa `INPUT_PULLUP`, sem resistor externo) |
| 1 | Fonte externa 5–6 V, mínimo 2 A (para os servos) |
| 1 | Capacitor eletrolítico 470–1000 µF (na fonte dos servos) |
| — | Jumpers, protoboard, caixa de MDF, parafusos e cola quente |

## 🔌 Ligações

**Display ST7735S (SPI)**

| Pino do display | Arduino |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SCL / SCK | D13 |
| SDA / MOSI | D11 |
| RES / RST | D6 |
| DC / A0 | D7 |
| CS | D8 |
| BLK | 3.3V |

**Botão e servos**

| Componente | Arduino |
|---|---|
| Botão (outro lado no GND) | D2 |
| Servo Porta 1 (sinal) | D3 |
| Servo Porta 2 (sinal) | D4 |
| Servo Porta 3 (sinal) | D5 |

> ⚠️ **Alimentação dos servos:** use a fonte externa (5–6 V) e **una o GND dela ao GND do Arduino**. Não alimente os 3 servos pelo 5 V do Arduino: o pico de corrente pode causar reinicializações e travamentos.

## 📚 Bibliotecas

Instale pelo Gerenciador de Bibliotecas da IDE Arduino:

- **Adafruit GFX Library**
- **Adafruit ST7735 and ST7789 Library** (aceite instalar as dependências)
- **Servo** (já vem com a IDE)

> A biblioteca MFRC522 **não** é necessária nesta versão.

## 🚀 Como usar

1. Monte o circuito conforme as tabelas acima.
2. Abra `painel_portas_sem_rfid.ino` na IDE Arduino.
3. Selecione **Arduino Uno** e a porta COM correta.
4. Grave o sketch.
5. Na tela inicial, dê **2 cliques rápidos** para abrir o menu.
6. **Clique** para trocar de porta e **segure** o botão para abrir/fechar.

## ⚙️ Configurações no código

```cpp
const int ANGULO_FECHADO = 90;                       // posição do servo com a porta fechada
const int ANGULO_ABERTO  = 0;                        // posição do servo com a porta aberta
const unsigned long TEMPO_LIMITE_ABERTA = 180000UL;  // fechamento automático (3 min)
const unsigned long TEMPO_CLIQUE_LONGO  = 600;       // ms para considerar "segurar"
const unsigned long JANELA_DUPLO_CLIQUE = 400;       // ms máximo entre os 2 cliques
const unsigned long TIMEOUT_MENU        = 30000;     // volta à tela inicial sem uso
```

Os ângulos dependem de como o servo foi fixado na caixa. Se a porta abrir "ao contrário", basta trocar os dois valores.

## 🖥️ Ajustes do display

Painéis 160x80 variam entre fabricantes. Se a imagem sair torta ou com cores estranhas, mexa em `setup()`:

```cpp
tft.initR(INITR_MINI160x80_PLUGIN);  // alternativa: INITR_MINI160x80
tft.setRotation(1);                  // teste 0, 1, 2 ou 3
tft.invertDisplay(true);             // alternativa: false
```

## 🛠️ Solução de problemas

| Sintoma | O que verificar |
|---|---|
| Tela totalmente apagada | Pino `BLK` ligado ao 3.3V e fiação de `SCL`/`SDA` em D13/D11 |
| Texto cortado ou sobreposto | Valor de `setRotation()` |
| Cores com tom único ou invertidas | Variante do `initR` e `invertDisplay` |
| Arduino reinicia quando um servo se move | Servos na fonte externa, GND em comum e capacitor instalado |
| Servo treme parado | O código já usa `detach()`; confira a fonte e o GND |

## 🗺️ Roadmap

- [x] Menu navegável com um único botão
- [x] Abertura/fechamento individual de cada porta
- [x] Fechamento automático com contagem regressiva
- [ ] Integração com leitor **RFID RC522** (liberação do menu por tag autorizada)
- [ ] Persistência de configurações
- [ ] Sinalização sonora

## 📄 Licença

Defina a licença do projeto (por exemplo, MIT) adicionando um arquivo `LICENSE`.

## 👤 Autor

**Alexsander Sudario Abreu** — [github.com/alexsander020](https://github.com/alexsander020)
