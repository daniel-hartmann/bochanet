# Memorial Descritivo Técnico — Trabalho 2 (rascunho)

Projeto **BochaNet**: lobby em terceira pessoa (template Third Person) com um portal que leva os jogadores a um
minijogo de **bocha** (regras CBTG, modalidade individual), jogado em rede (Listen Server) numa cancha 3D.
Unreal Engine 5.8, lógica em **C++** (módulo `BochaNet`) e Blueprints só como dados (malhas, materiais, classes,
layout de UMG).

> Rascunho para revisão: copie as respostas para o documento técnico do Moodle e ajuste o que quiser.

---

## A. Animação e Cinemática

### 1. Técnicas empregadas

- **Esqueleto e rigging (Skeletal Mesh + Animation Blueprint):** o jogador do minijogo usa `SKM_Manny_Simple`
  (vermelho) ou `SKM_Quinn_Simple` (azul); o árbitro usa o Manny com material cinza. Os dois são animados pelo
  `ABP_BochaPlayer`, filho da classe C++ `UBochaAnimInstance`.
- **Máquinas de estados:** `Locomotion` (Idle ↔ Walk/Run, com o Blend Space 2D `BS_Idle_Walk_Run` por direção e
  velocidade) e `Main States` (Jump → Fall Loop → Land, por `IsFalling`).
- **Montages + Anim Notifies:** o arremesso (`AM_BochaThrow`) e a medição do árbitro (`AM_BochaMeasure`) tocam no
  `DefaultSlot`. Cada um tem um *Anim Notify* em C++ que dispara gameplay no quadro exato:
  - `UBochaAnimNotify_Release` ("Soltar Bola"): a bola só sai da mão nesse quadro;
  - `UBochaAnimNotify_Measure` ("Medir"): o ponto só é anunciado quando o árbitro se agacha sobre o bolim.
- **Cinemática procedural (atores sem malha):** as bolas são esferas com física. O rolamento é controlado em C++
  (`ABochaBall::Tick`): resistência de rolamento constante (210 cm/s²), parada abaixo de 5 cm/s, lançamento rente
  ao chão já com a rotação de rolamento. O bolim usa **freio controlado** (desaceleração calculada a cada quadro,
  v² = 2·a·d) para parar exatamente no ponto escolhido pela força. A câmera que segue a bola usa o lag do
  SpringArm como interpolação.

**Relevância para o game feel e a legibilidade:** o notify de soltura sincroniza a saída da bola com o gesto, o que
dá peso ao arremesso; o árbitro andando até o bolim e medindo transforma o "cálculo do ponto" num evento visível e
com tempo de leitura, e a câmera que persegue a bola deixa claro onde cada jogada terminou. A cinemática procedural
das bolas foi escolhida no lugar de timelines porque o resultado precisa ser **físico** (colisões entre bolas) e,
ao mesmo tempo, **previsível**: distância = v²/420 (medida pelo comando `Bocha.CalibrateThrows`).

### 2. Extração de dados

A classe C++ `UBochaAnimInstance` lê o `UCharacterMovementComponent` em `NativeUpdateAnimation`, uma vez por
quadro, e expõe variáveis `BlueprintReadOnly` que a state machine consome:

| Variável | Origem | Uso |
|---|---|---|
| `Velocity` | `MovementComponent->Velocity` | base das outras |
| `GroundSpeed` | `Velocity.Size2D()` | eixo Y do Blend Space |
| `Direction` | `UKismetAnimationLibrary::CalculateDirection` (±45° se gira para o movimento) | eixo X do Blend Space |
| `ShouldMove` | `GroundSpeed > 3 cm/s` | transição Idle ↔ Walk/Run |
| `IsFalling` | `MovementComponent->IsFalling()` | Jump / Fall Loop / Land |

O AnimBP não tem EventGraph: toda a extração está em C++. `ShouldMove` depende só da velocidade porque o árbitro,
guiado por `AIController`, segue o caminho com aceleração zero (e as cópias remotas também); com a regra do
template (velocidade **e** aceleração) ele deslizaria parado em Idle.

---

## B. Cenário, Navegação e Câmera

### 3. Estruturação de cenário

- O minijogo é um **nível isolado** (`Lvl_Cancha`) carregado por **Level Streaming** (`LoadLevelInstance`) em
  (5000, 0, 0) quando os jogadores entram pelo portal do lobby; ao fim da partida é descarregado.
- A cancha é modular: piso e tábuas são malhas geradas com o *CubeGrid* (blocos com colisão simples), em escala
  real de 26,50 m × 4 m (Art. 1º); as linhas do regulamento (A, B, C, D, E, B', A') são dados (`FBochaCourt`) e
  são desenhadas no chão.
- **Colisão por canais e presets** (`Config/DefaultEngine.ini`): canais `BochaBall` e `BochaReferee` (objeto) e
  `BochaGround` (traçado); preset `BochaBall` (bloqueia cancha e bolas, ignora árbitro e câmera) e `BochaReferee`
  (ignora bolas, câmera e o traçado de chão).
- **Benefício:** só quem joga carrega a cancha; atores pesados (NavMesh, árbitro) existem só durante a partida; a
  separação por canais evita interações indesejadas (o árbitro atravessa as bolas sem movê-las, a câmera não
  colide com elas) sem lógica extra.

### 4. Necessidade e uso de NavMesh

Sim. O **árbitro** (`ABochaReferee`, Character com `AIController`) consome a NavMesh: depois de cada bocha ele anda
pelo `NavigationSystem` (`AIController::MoveToLocation`, com pathfinding e destino projetado na malha) até 70 cm
antes do bolim, mede e volta à lateral. Os limites navegáveis vêm de um `NavMeshBoundsVolume` que cobre o piso
da cancha; as tábuas ficam fora. Como a cancha aparece em tempo de execução, numa posição deslocada, a NavMesh é
gerada dinamicamente (`RuntimeGeneration = Dynamic`, `bForceRebuildOnLoad`). As bolas não alteram a NavMesh
(`CanEverAffectNavigation = false`).

### 5. Papel da câmera

- **Minijogo em primeira pessoa:** a câmera fica na altura dos olhos, 40 cm à frente do centro, inclinada 10° para
  baixo e alinhada ao eixo de visão do personagem. A mira gira o personagem (yaw), e a câmera gira junto, com lag
  de rotação para suavizar. O personagem do minijogo existe só na máquina local e não é possuído pelo
  PlayerController (por causa da arquitetura em rede), por isso a rotação vem do ator e não do *Controller
  Rotation*. A cabeça e o pescoço da malha ficam escondidos para não ocluir a visão.
- **Câmera que segue a bola (terceira pessoa durante o rolamento):** pivô na bola + `SpringArmComponent` em rotação
  absoluta (−25°, olhando para o fundo da cancha), `TargetArmLength` 300, *Camera Lag* 5, colisão com *Probe Size*
  12 no canal `Camera` (as tábuas encurtam o braço; as bolas ignoram esse canal). O braço sai 50 cm acima da bola
  para a sonda não nascer dentro do piso.
- **Lobby em terceira pessoa:** `CameraBoom` com `TargetArmLength` 400, *Camera Lag* 10 e *Probe Size* 12.

---

## C. Interface e Arquitetura de Dados

### 6. Elementos obrigatórios de interface

- **HUD (`WBP_HUD`, `UBochaHUDWidget`):** linhas de cada jogador com cor, "(você)", placar, quem tem o ponto e de
  quem é a vez; ícones das bochas restantes (apagam quando jogadas); ícone do bolim em quem vai lançá-lo; barra de
  força do arremesso; avisos de regras em letra grande (sorteio, ponto, bolim irregular, bocha/mão anulada).
- **Minimapa esquemático (`UBochaMinimapWidget`):** trecho de 10 m da cancha centrado no bolim, alto contraste,
  linhas do regulamento e bolas ampliadas.
- **Telas de estado (`WBP_GameStateScreen`):** Início (cor do jogador, sorteio, controles), Pausa (local, tecla P),
  Vitória e Derrota (por jogador, com placar).

### 7. Referências e inspirações

*(Preencher.)* Sugestões: placares de transmissões de bocha/bocce e de curling (linha por equipe com ícones das
pedras restantes), jogos de golfe e boliche (barra de força e câmera que acompanha a bola), e minimapas táticos
esquemáticos. A disposição — placar no canto, força ao lado, minimapa no canto oposto e avisos centralizados no
alto — mantém o centro da tela livre para a mira em primeira pessoa.

### 8. Comunicação ator–interface

Sem *Tick* nem *Property Binding*: a interface reage a **eventos**.

- `ABochaMatch` (estado replicado da partida) dispara `OnStateChanged` (fase, vez, placar, bochas, ponto),
  `OnAnnounce` (avisos) e `OnGameOver`; o `ABochaMinigameManager` (lado local) assina esses delegates e chama
  métodos do HUD e das telas (`ShowPlayers`, `ShowBochasLeft`, `ShowMessage`, `Show(EBochaScreen…)`).
- A força é enviada pelo personagem ao HUD por chamada direta quando muda.
- O minimapa mantém a própria lista de bolas pelos eventos `ABochaBall::OnBallBeginPlay`/`OnBallEndPlay` e, ao
  desenhar, só lê posições.
- Os widgets são ligados ao C++ por `BindWidget` (nomes no Widget Tree), sem acoplar a lógica ao layout.

---

## D. Física e Escopo do MVP

### 9. Simulação física

Sim, é central: bochas e bolim são corpos rígidos (*Simulate Physics*) que rolam, batem nas tábuas e se empurram —
é isso que permite aproximar, encostar e deslocar bolas, como no jogo real. A física roda no servidor e as posições
são replicadas. Ajustes feitos para a física ser controlável: sem *Angular Damping* e sem o limite de 3600°/s de
rotação (que faziam a bola escorregar em vez de rolar), lançamento rente ao chão com rotação de rolamento,
resistência de rolamento constante e freio controlado no bolim. Os contatos são classificados por canais de colisão
e por eventos de *hit* (ex.: bocha que bate direto na tábua do fundo sem fazer jogo é anulada).

### 10. Especificação do MVP

- **Animação:** jogador com malha esquelética, AnimBP com duas state machines (locomoção com Blend Space 2D e
  pulo/queda), montage de arremesso com notify que solta a bola; árbitro animado (corrida e medição com notify).
- **Navegação:** NavMesh na cancha e árbitro autônomo com `AIController` andando até o bolim e voltando.
- **Interface:** HUD orientado a eventos (placar, vez, ponto, bochas, força, avisos), minimapa e telas de início,
  pausa, vitória e derrota.
- **Base de jogo:** partida em rede para 2 jogadores, regras de bocha (vez por distância, validade do bolim,
  anulações, pontos por mão, vitória aos 12) e câmera que acompanha a bola.
