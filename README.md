<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=02A6F4&height=120&section=header"/>
<h1 align="center">Embarcatech - Projeto Integrado - BitDogLab </h1>

## Objetivo do Projeto

Desenvolver um semáforo inteligente com modo noturno e acessibilidade, utilizando a plaquinha BitDog Lab com RP2040. O sistema simula um semáforo real com dois modos de operação (Normal e Noturno), alternados por botão, exibindo estados em uma matriz de LEDs 5x5, LED RGB e display OLED, além de emitir sinais sonoros via buzzer para acessibilidade de pessoas cegas. Inclui um botão para modo BOOTSEL, facilitando o upload de firmware.

## 🗒️ Lista de requisitos

- **Leitura de botões (A e B):** Botão A para alternar modos, botão B para modo BOOTSEL;
- **Utilização da matriz de LEDs:** Representação visual do estado do semáforo;
- **Utilização de LED RGB:** Sinalização das cores do semáforo;
- **Exibição de informações em tempo real no display gráfico 128x64 (SSD1306):** Modo e estado do semáforo;
- **Utilização do buzzer:** Sinais sonoros distintos para acessibilidade;
- **Estruturação do projeto no ambiente VS Code:** Configurado para desenvolvimento com RP2040 e FreeRTOS;
- **Uso exclusivo de tarefas FreeRTOS:** Sem filas, semáforos ou mutexes;
  

## 🛠 Tecnologias

1. **Microcontrolador:** Raspberry Pi Pico W (na BitDogLab).
2. **Display OLED SSD1306:** 128x64 pixels, conectado via I2C (GPIO 14 - SDA, GPIO 15 - SCL).
3. **Botão A:** GPIO 5 (alternar modos).
4. **Botão B:** GPIO 6 (modo BOOTSEL).
5. **Matriz de LEDs:** WS2812 (GPIO 7).
6. **LED RGB:** GPIOs 11 (verde), 12 (azul), 13 (vermelho).
7. **Buzzer:** GPIO 10.
8. **Linguagem de Programação:** C.
9. **Frameworks:** Pico SDK, FreeRTOS.


## 🔧 Funcionalidades Implementadas:

**Funções dos Componentes**
   
- **Display:** Ciclo de cores (Verde: 5s, Amarelo: 2s, Vermelho: 5s) com sinais sonoros (Verde: 1 beep de 200ms, Amarelo: 4 beeps rápidos de 100ms, Vermelho: 2 beeps de 200ms a cada 1.5s).
- **Modo Noturno:** Amarelo piscando (1s ligado, 1s desligado) com beep a cada 2s.
- **Display OLED:** Exibe o modo ("Modo Normal" ou "Modo Noturno") e estado ("Verde", "Pode Atravessar", etc.), com mensagens centralizadas.
- **Matriz de LEDs (WS2812):** Representa o semáforo em formato 5x5, com luzes centrais indicando Verde (linha 3), Amarelo (linha 2) ou Vermelho (linha 1).
- **LED RGB:** Sinaliza as cores em sincronia com a matriz (Verde: G=1, Amarelo: R=1/G=1, Vermelho: R=1).
- **Buzzer:** Emite sons distintos para acessibilidade, conforme o estado e modo.
- **Botão A:** Alterna entre Modo Normal e Noturno com debounce de 200ms.
- **Botão B:** Ativa o modo BOOTSEL via interrupção, facilitando o upload de firmware.


## 🚀 Passos para Compilação e Upload do projeto Ohmímetro com Matriz de LEDs

1. **Instale o Pico SDK:** Certifique-se de que o ambiente está configurado com o Pico SDK e as bibliotecas do FreeRTOS.
2. **Crie uma pasta `build`**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make

3. **Transferir o firmware para a placa:**

- Conectar a placa BitDogLab ao computador
- Pressione o botão B para entrar no modo BOOTSEL (ou mantenha pressionado enquanto conecta o cabo USB)
- Copiar o arquivo .uf2 gerado para o drive da placa

4. **Testar o projeto**

- Após o upload, desconecte e reconecte a placa.

🛠🔧🛠🔧🛠🔧


## 🎥 Demonstração: 

- Para ver o funcionamento do projeto, acesse o vídeo de demonstração gravado por José Vinicius em: https://www.youtube.com/watch?v=frLmub0GoRQ

## 💻 Desenvolvedor
 
<table>
  <tr>
    <td align="center"><img style="" src="https://avatars.githubusercontent.com/u/191687774?v=4" width="100px;" alt=""/><br /><sub><b> José Vinicius </b></sub></a><br />👨‍💻</a></td>
  </tr>
</table>
