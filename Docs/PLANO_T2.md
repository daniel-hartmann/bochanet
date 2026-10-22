# Plano: Trabalho 2 — Animação, Navegação e Interface

Enunciado: `Docs/EnunciadoT2.pdf`. Checklist: `CHECKLIST.md`, seção "Trabalho 2".
Entrega: **27/10/2026** (hoje é 21/10: 6 dias). Os quatro critérios valem 25% cada:
Animação · Navegação e Câmera · UMG · Memorial e Organização.

## Ponto de partida

| Pilar | O que já existe | O que falta |
|---|---|---|
| Animação | Só o template: lobby com `SKM_Quinn_Simple` + `ABP_Unarmed` (state machine, `BS_Idle_Walk_Run`, pulo/queda) | Personagem do minijogo sem malha; nenhum AnimBP nosso; nenhum Anim Notify; nenhum estado de arremesso |
| Navegação | Nada | `NavMeshBoundsVolume`, agente navegando (NavigationSystem) |
| Câmera | SpringArm + Camera no personagem do minijogo e no do lobby; câmera que segue a bola | Lag, colisão/probe e distância configurados e justificados |
| UMG | HUD (`UBochaHUDWidget`) com placar, bochas, vez, ponto, força, avisos; atualizado por delegates | Minimapa sem consulta por frame; telas de início/pausa/vitória/derrota; Widget Tree com nomes |
| Física | Bolas com Simulate Physics | Canais e presets de colisão próprios |
| Organização | — | Nomes padrão, pastas, assets sem uso, memorial |

## A ideia que amarra animação e navegação: o árbitro

Um **NPC árbitro** na cancha cobre de uma vez os itens mais pesados de animação e navegação, dentro do tema do jogo:

1. Depois que as bolas param, o servidor manda o árbitro **andar até o bolim** (`AIController::MoveToLocation`
   na NavMesh da cancha) — *comportamento navegável*.
2. Ao chegar, ele toca uma animação de **medir** (montage); um **Anim Notify** no quadro em que ele "mede" dispara
   o anúncio do ponto ("Ponto: Azul") — *notify com evento preciso de gameplay*.
3. Depois ele **volta** para a lateral da cancha.
4. Ele usa malha esquelética e um AnimBP nosso com locomoção por blend space — *pipeline de animação*.

Para não empurrar as bolas, a cápsula dele **ignora o canal de colisão das bolas** (e as bolas ignoram ele) —
o que também mostra o uso de *canais/presets de colisão*.

O **jogador do minijogo** ganha corpo também: malha, o mesmo AnimBP (locomoção lateral) e um estado/montage de
**arremesso**, com um Anim Notify `Release` no quadro em que a mão solta a bola — a bola só sai nesse momento.

## Fases

Legenda: **[C]** eu faço por código/commandlet · **[E]** precisa ser feito por você no editor (eu escrevo o passo a passo).

### F1 — Colisão (meio dia) ✔ feita
Canais `BochaBall` (objeto, `ECC_GameTraceChannel1`), `BochaReferee` (objeto, `2`) e `BochaGround` (traçado, `3`);
presets `BochaBall` (ignora `BochaReferee` e `Camera`) e `BochaReferee` (ignora `BochaBall`, `Camera`, `BochaGround`).
Constantes em `Source/BochaNet/BochaCollision.h`. Bola na mão sem colisão. Conferido pela calibração: preset aplicado
nos BPs das bolas e distâncias iguais às de antes.

- [C] Canal de objeto `BochaBall` e presets `BochaBall` e `BochaReferee` em `Config/DefaultEngine.ini`.
- [C] Bolas usam `BochaBall` (bloqueia cancha e bolas; ignora o árbitro); `IsOnGround` com canal de traçado próprio.
- [C] Bolas não afetam a NavMesh (`CanEverAffectNavigation = false`).

### F2 — Câmera (meio dia) ✔ feita (falta teste no PIE)
- **Minijogo em primeira pessoa** (decisão 4): câmera 30 cm à frente do peito e 10 cm abaixo do centro, alinhada ao
  eixo de visão e girando com a mira (yaw do ator — o personagem não é possuído, então não há Controller Rotation;
  justificar no memorial). SpringArm com braço 0, sem probe, só lag de rotação (12) para suavizar a mira.
- **Câmera que segue a bola** (terceira pessoa durante o rolamento): pivô na bola + SpringArm em rotação absoluta
  (−25°, olhando +X), braço 300, lag 5, colisão com probe 12 no canal `Camera` (bolas ignoram esse canal).
- **Lobby** (terceira pessoa do template): `CameraBoom` com braço 400, `bEnableCameraLag` (10), probe 12/`Camera`.
- Commandlet ganhou `setComponents` (propriedades de componentes de BP).

### F3 — Animação do jogador (1 dia) ✔ feita (falta teste no PIE)
Feita toda por código/commandlet (sem passos no editor):
- `UBochaAnimInstance` (C++): `Velocity`, `GroundSpeed`, `Direction`, `ShouldMove`, `IsFalling` em
  `NativeUpdateAnimation`, com os mesmos nomes e regras do template (direção limitada a ±45° só se o personagem
  gira para o movimento) — resposta da pergunta 2 do memorial.
- `ABP_BochaPlayer` (`Bocha/Animations`): cópia do `ABP_Unarmed` reparentada para o `UBochaAnimInstance`, sem
  EventGraph nem variáveis em BP. Mantém as state machines `Locomotion` (Idle ↔ Walk/Run, `BS_Idle_Walk_Run` 2D)
  e `Main States` (Jump / Fall Loop / Land) e o `Slot 'DefaultSlot'`.
- `AM_BochaThrow` (`Bocha/Animations`): montage do `MM_Attack_01` com o notify `UBochaAnimNotify_Release`
  ("Soltar Bola") a 40% (0,40 s). `DoThrow` toca o montage; a bola só sai no notify (`ReleaseBall`), com
  rede de segurança se o montage for interrompido.
- `BP_MiniGameCharacter`: `RedMesh` = Manny, `BlueMesh` = Quinn, `AnimClass` = `ABP_BochaPlayer`, `ThrowMontage`.
- Ajuste opcional no editor: arrastar o notify no `AM_BochaThrow` para o quadro exato em que a mão "solta",
  ou trocar a animação base por `MM_Attack_02/03` ou `MM_ChargedAttack`.

### F4 — Árbitro e navegação (1,5 dia) ✔ feita e testada
- `ABochaReferee` (C++, Character replicado, `AIController` no servidor, preset `BochaReferee`): `WalkTo` via
  `AIController::MoveToLocation` (pathfinding, destino projetado na NavMesh), `Measure` (montage por Multicast),
  `OnArrived`/`OnMeasured`. Ignora caminhadas antigas abortadas (ID da requisição).
- `BP_BochaReferee` (`Bocha/`): `SKM_Manny_Simple` com `BasicShapeMaterial` cinza em todos os slots, `ABP_BochaPlayer`,
  `AM_BochaMeasure` (do `MM_Land`, notify `UBochaAnimNotify_Measure` "Medir" a 0,43 s).
- `ABochaMatch`: fase nova `Measuring`. Depois de cada bocha parar (e das anulações), o árbitro anda até 70 cm antes
  do bolim, mede, e só no notify o ponto é anunciado (`ResolveThrow`); depois volta para `RefereeHome`
  (lado dos jogadores, X −2450, junto à tábua esquerda: fora do quadro da câmera que segue a bola; trote a 400 cm/s). Sem caminho ou mais de `MeasureTimeout` = 10 s: o jogo segue e o log avisa.
  Spawn no primeiro arremesso (a cancha já está carregada no servidor). `RefereeClass` no `BP_BochaMatch`.
- NavMesh: `RuntimeGeneration=Dynamic` e `bForceRebuildOnLoad=True` em `[/Script/NavigationSystem.RecastNavMesh]`
  (a cancha é level instance carregado em (5000, 0, 0)).
- [E] ✔ `NavMeshBoundsVolume` no `CanchaMap` — Location (−1275, −200, 100), Scale (13.5, 2.2, 1.0).
- `ShouldMove` do `UBochaAnimInstance` passou a depender só da velocidade (> 3 cm/s): o AI segue o caminho com
  aceleração zero (e as cópias remotas também), então pela regra do template o árbitro deslizava em Idle.
- Camera do jogador: 40 cm à frente e pescoço escondido (o corpo atravessava a câmera no arremesso).

### F5 — UMG (1 dia) ✔ feita (falta teste no PIE)
- **Minimapa orientado a eventos:** `ABochaBall::OnBallBeginPlay`/`OnBallEndPlay` (delegates estáticos, filtrados por
  mundo); `UBochaMinimapWidget` monta a lista uma vez e só lê posições ao desenhar. O `ABochaMatch` é procurado uma vez.
- **`WBP_GameStateScreen`** (filho de `UBochaStateScreenWidget`, árvore montada por commandlet): `ScreenRoot` ›
  `Backdrop` (preto 65%) › `CardContent` › `TitleText` / `SubtitleText` / `ControlsText` / `ContinueButton`.
  Telas: **Início** (6 s: cor do jogador, sorteio, controles), **Pausa** (P; local: cursor, só a tela recebe o
  mouse, comandos do minijogo bloqueados para quem pausou; botão Continuar), **Vitória**/**Derrota** (por jogador,
  com o placar). Mostradas pelo manager em resposta a eventos.
- `IA_BochaPause` (P) no `IMC_Bocha`. Esc não é usado porque no PIE ele encerra a sessão.
- **`WBP_HUD`:** widgets renomeados (`HUDRoot`, `ScorePanel`, `RedNameRow`, `RedBochasRow`, `BlueNameRow`,
  `BlueBochasRow`); os `P1_*`/`P2_*` mantêm o nome porque são ligados ao C++ (BindWidget).
- Commandlet: `createWidgets` (Widget Tree por JSON) e `renameWidgets`.

### F6 — Organização e memorial (1 dia) ✔ feita (falta: memorial pergunta 7 + revisão; Outliner do lobby)
- `Docs/MEMORIAL.md`: rascunho das 10 respostas.
- `Lvl_Cancha` (ex-`CanchaMap`): pastas Court / Gameplay / Navigation / Lighting, rótulos `SM_CourtFloor`,
  `SM_CourtBoards`, `PlayerStart_Court`, `NavMeshBounds_Court`, `PointLight_Court`; `BP_MinimapCapture` removido.
- Apagados: `WBP_Minimap`, `RT_Minimap`, `BP_MinimapCapture`, `WBP_StartGame`, a classe C++ `ABochaMinimapCapture`,
  os redirectors e a pasta antiga `/Game/Blueprints` (4 redirectors de setembro).
- Renomeados: `Blueprints/Balls/BP_Ball|BP_Bocha|BP_Bolim`, `Blueprints/BP_BochaMinigameManager`,
  `Blueprints/BP_BochaMatch`, `Blueprints/BP_BochaReferee`, `Blueprints/BP_BochaPlayerCharacter`, `Maps/Lvl_Cancha`,
  `M_BallBlue`, `M_BallRed`, `M_Clay`, `M_Floor`, `M_Wall`, `T_Clay01`, `Lobby/BP_BochaPortal`.
- Como foi feito (o `IAssetTools::RenameAssets`/`FixupReferencers` pedem diálogos/Slate e não funcionam sem interface):
  `ObjectTools::RenameSingleObject` com redirector → salvar → regravar os referenciadores (`fixupRedirectorsIn`)
  → apagar os redirectors. Specs: `t2-f6-organizacao.json`, `t2-f6-renomear.json`, `t2-f6-redirectors.json`.
- Verificado: todos os BPs compilam do disco; teste headless no `Lvl_Cancha` (calibrador) sem erros de carregamento.
- [E] Falta: Outliner do lobby (`Lvl_ThirdPerson`, World Partition) e decidir o `Materials/clay-01.png` (arquivo-fonte
  solto dentro de `Content`).

## Cronograma sugerido

| Dia | Fases |
|---|---|
| 21–22/10 | F1, F2, F3 (código) + passos do editor da F3 |
| 23–24/10 | F4 (árbitro + NavMesh) |
| 25/10 | F5 (UMG) |
| 26/10 | F6 (organização + memorial), teste final em PIE com 2 jogadores |
| 27/10 | Margem, revisão e entrega |

## Decisões (21/10)

1. **Árbitro na cancha** (F4), não NPC no lobby.
2. **Pausa como menu local:** ESC abre o menu só para quem apertou; a partida continua para o outro jogador.
3. **Malha por cor:** vermelho com `SKM_Manny_Simple`, azul com `SKM_Quinn_Simple`.
4. **Minijogo em primeira pessoa**, como era antes (a terceira pessoa da F2 foi desfeita).
