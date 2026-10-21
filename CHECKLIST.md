# Checklist: Trabalho 1 - Desenvolvimento do Minigame (Puzzle 3D)

## Programação II

Este checklist contém os requisitos técnicos para o desenvolvimento do minigame (puzzle 3D) que servirá como base para a integração em rede. Os itens abaixo refletem os conteúdos abordados em **Programação II** e devem ser implementados considerando a execução local no lado do cliente.

### 1. Estruturação do Nível e Atores

- [x]  **Criação do Subnível:** O minigame deve ser construído em um nível (mapa) isolado, preparado para ser carregado via *Level Streaming* no mapa principal.
- [x]  **Construção do Ambiente:** Utilização de formas geométricas primitivas (cubos, esferas, cilindros) ou assets externos para compor a área do puzzle.
- [x]  **Sistemas de Coordenadas:** Posicionamento e transformação (Translação, Rotação, Escala) corretos dos atores que compõem o desafio lógico.

### 2. Controle e Câmera

- [x]  **Configuração do Pawn/Character:** Definição do ator controlado pelo jogador durante a execução do minigame.
- [x]  **Câmera:** Implementação de câmera adequada ao puzzle (em primeira ou terceira pessoa, fixa ou móvel), aplicando os conceitos de navegação vistos em aula.
- [x]  **Eventos de Entrada (Inputs):** Mapeamento e implementação dos comandos de controle (teclado/mouse ou gamepad) necessários para resolver o puzzle, **utilizando o sistema de *Enhanced Input***.

### 3. Lógica de Gameplay (Blueprints e/ou C++)

- [x]  **Mecânica Principal:** Implementação da lógica de resolução do puzzle utilizando *Visual Scripting* (Blueprints) ou código nativo (C++).
- [x]  **Condição de Vitória:** Programação de um **estado claro** que valide a conclusão do puzzle (ex: variável booleana `bPuzzleResolvido` definida como *True*, ou disparo de um *Event Dispatcher* local).

### 4. Física e Colisões

- [x]  **Volumes de Colisão:** Configuração de colisões (canais *Block* e/ou *Overlap*) adequadas para impedir que o jogador saia da área do puzzle ou atravesse objetos indevidos.
- [x]  **Corpos Rígidos (Rigid Bodies):** Se a mecânica do puzzle exigir (ex: empurrar blocos, botões de pressão), ativação e configuração da simulação de física nos atores correspondentes.

### 5. Boas Práticas e Organização

- [ ]  **Organização do Outliner:** Agrupamento lógico dos elementos do cenário e nomenclatura padronizada e descritiva para todos os atores no *World Outliner*.
- [x]  **Limpeza de Lógica:** Uso de cores padrão nos nós de Blueprint (caso utilize visual scripting) e formatação correta de código, mantendo a distinção arquitetural do que é processado na interface e no ambiente físico do jogo.
- [ ]  Entrega de acordo com o documento *Instruções para Entrega dos Trabalhos* (no Moodle)


## Redes

- [x] O ambiente principal deverá utilizar o template Third Person padrão da engine.
- [x] O mapa base (level do 3rd person) deve suportar a instanciação simultânea de múltiplos jogadores (Listen Server)
- [x] Deve existir um objeto interativo compartilhado no mapa principal. Todos os jogadores devem poder interagir com ele sem que o objeto seja coletado ou destruído.
- [ ] A interação com este objeto deve disparar localmente (apenas para o cliente que interagiu) um minijogo 3D utilizando a técnica de Level Streaming. O minijogo roda no lado do cliente.
- [x] O primeiro jogador a satisfazer a condição de vitória do puzzle deve enviar
um evento de validação para o servidor (Server RPC).
- [x] O servidor, ao receber o sinal de vitória, deve processar o encerramento da partida e emitir um aviso global (Game Over) para todos os clientes, indicando o vencedor.
- [x] Lembre-se de utilizar nós de validação (como Is Locally Controlled ou Is Local Player Controller) para garantir que o Level Streaming não puxe a câmera dos outros jogadores da rede.
- [x] Testar utilizando o modo Play in Editor (PIE) configurado como Listen Server com pelo menos 2 jogadores



# Checklist: Trabalho 2

Refinamento de cenário 3D: animação, navegação e interface. Enunciado: `Docs/EnunciadoT2.pdf`.
Entrega: **27/10/2026** até 23h59 (Moodle) e apresentação em aula. Plano: `Docs/PLANO_T2.md`.

### Controle de Animação (25%)

- [x] **Pipeline de Animação:** malha esquelética (*Skeletal Mesh*) controlada por um *Animation Blueprint* (`UAnimInstance`). *(Jogador do minijogo com Manny/Quinn por cor e `ABP_BochaPlayer`, filho do `UBochaAnimInstance` em C++.)*
- [x] **Máquina de Estados:** estados de locomoção com interpolação contínua (*Blend Space 1D/2D* por velocidade/direção). *(`Locomotion`: Idle ↔ Walk/Run com `BS_Idle_Walk_Run` 2D — X = `Direction`, Y = `GroundSpeed`.)*
- [x] **Máquina de Estados:** estados de transição para salto, queda, ataque ou ação contextual da mecânica do jogo. *(`Main States`: Jump / Fall Loop / Land por `IsFalling`; arremesso como montage `AM_BochaThrow` no `DefaultSlot`.)*
- [x] **Anim Notifies:** ao menos um *Anim Notify* disparando eventos precisos em tempo de execução (som, partículas, *hitbox* ou liberação de controle). *(`UBochaAnimNotify_Release` "Soltar Bola" no `AM_BochaThrow`: a bola só sai nesse quadro.)*

### Navegação, Cenário e Câmera (25%)

- [x] **Malha de Navegação:** volume de navegação ativo (`NavMeshBoundsVolume`) no nível. *(No `CanchaMap`, cobrindo a cancha; NavMesh gerada em tempo de execução porque a cancha é level instance.)*
- [x] **Comportamento Navegável:** funcionalidade que dependa de resolução de caminhos (`NavigationSystem`): agente autônomo (NPC) indo a alvos/pontos de patrulha, ou *Click-to-Move* via `AIController`/`SimpleMoveToLocation`. *(Árbitro `ABochaReferee`: `AIController::MoveToLocation` até o bolim depois de cada bocha, mede (notify "Medir") e volta.)*
- [x] **Arquitetura de Câmera:** `CameraComponent` + `SpringArmComponent` com distância (`TargetArmLength`), *Camera Lag* e tratamento de oclusão (*Probe Size* / colisão de câmera). *(Minijogo em primeira pessoa: câmera alinhada ao eixo de visão, 30 cm à frente do peito, girando com a mira, lag de rotação; câmera que segue a bola com SpringArm (braço 300, lag, probe 12 no canal `Camera`); lobby em terceira pessoa com `CameraBoom` (braço 400, lag, probe).)*

### Interface do Usuário — UMG (25%)

- [x] **Widget Principal (HUD):** interface via *Unreal Motion Graphics* (`UserWidget`). *(`WBP_HUD` / `UBochaHUDWidget`.)*
- [x] **Exibição de Parâmetros de Estado:** informações vitais em tempo real (placar, bochas restantes, vez, quem tem o ponto, força).
- [x] **Desacoplamento de Dados:** atualização orientada a eventos (*Event Dispatchers*/Delegates ou chamadas sob demanda); **sem** *Tick/Property Binding* para consultas pesadas. *(HUD e telas por delegates do `ABochaMatch` (`OnStateChanged`, `OnAnnounce`, `OnGameOver`); força por chamada direta; minimapa com lista de bolas mantida pelos eventos `OnBallBeginPlay`/`OnBallEndPlay`.)*
- [x] **Feedback de Estado de Jogo:** telas ou elementos indicando transições claras (início, pausa, vitória ou derrota). *(`WBP_GameStateScreen`: Início (cor do jogador, sorteio, controles), Pausa local na tecla P, Vitória e Derrota por jogador.)*
- [x] **Organização do *Widget Tree*** (critério de avaliação). *(`WBP_HUD`: `HUDRoot`, `ScorePanel`, `RedNameRow`, `RedBochasRow`, `BlueNameRow`, `BlueBochasRow`; `WBP_GameStateScreen`: `ScreenRoot` › `Backdrop` › `CardContent` › `TitleText`/`SubtitleText`/`ControlsText`/`ContinueButton`.)*

### Física e Colisão

- [x] **Perfis de colisão explícitos:** *Collision Presets* e canais de traçado/objeto definidos entre os atores do cenário. *(Canais `BochaBall`, `BochaReferee` e `BochaGround`; presets `BochaBall` e `BochaReferee` em `Config/DefaultEngine.ini`.)*
- [x] **Física de corpos rígidos:** *Simulate Physics* nas bolas, essencial para a mecânica.

### Memorial Descritivo Técnico e Organização (25%)

- [ ] **Memorial:** responder às 10 perguntas (A. Animação e Cinemática 1–2; B. Cenário, Navegação e Câmera 3–5; C. Interface e Arquitetura de Dados 6–8; D. Física e Escopo do MVP 9–10). *(Rascunho em `Docs/MEMORIAL.md`: falta revisar, completar a 7 (inspirações) e passar para o documento do Moodle.)*
- [x] **Nota para atores sem malha:** justificar no memorial a cinemática procedural das bolas (rolamento e freio controlado em C++). *(Memorial, pergunta 1.)*
- [ ] **Nomenclatura na engine:** prefixos padrão (`BP_`, `WBP_`, `M_`, `T_`, `IA_`, `IMC_`, `ABP_`, `BS_`…). *(Parcial: há assets como `ABall`, `BochaGameMode`, `BallBlueMaterial`, `clay-01`.)*
- [ ] **Organização do *Content Browser*:** pastas por tipo/sistema e sem assets sem uso (`WBP_Minimap`, `RT_Minimap`, `BP_MinimapCapture`).
- [ ] Entrega de acordo com o documento *Instruções para Entrega dos Trabalhos* (Moodle), até 27/10/2026.
