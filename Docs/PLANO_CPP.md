# Plano: Blueprints do Bocha → C++

Escopo: `Content/Bocha/**` + `Content/Lobby/BochaPortal`, e as partes de
`ThirdPerson/BP_ThirdPersonPlayerController` / `BP_ThirdPersonCharacter` que a lógica do Bocha usa.

Fonte da verdade da lógica: `Docs/bp-export/<BP>/` (gerado pelo commandlet
`BochaExportBlueprints`; `.txt` = legível, `.t3d` = texto dos nós, `_summary.txt` = variáveis/componentes/widgets).
Para regerar (editor fechado):

```bash
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$PWD/BochaNet/BochaNet.uproject" -run=BochaExportBlueprints -unattended -nullrhi -nosplash -stdout
```

Estratégia: classe base C++ + reparent do BP (o BP fica só com dados: mesh, material, layout, IA/IMC).

## Status

- [x] Fase 0 — módulos `BochaNet` (runtime) e `BochaNetEditor` (editor), compilando
- [x] Fase 1 — export dos grafos (14 BPs)
- [x] Fase 2 — bolas (`ABochaBall/Bocha/Bolim`) — testado
- [x] Fase 3 — widgets (`UBochaHUDWidget`) — testado
- [x] Fase 4 — MiniGameCharacter + MinigameManager (antiga Fase 7) — testado
- [x] Fase 5+6 — PlayerController + BochaPortal + Character do lobby (feitas juntas) — testado
- [x] Fase 7 — MinimapCapture — testado
- [x] Fase 8 — limpeza (falta o teste final PIE Listen Server 2 jogadores)

## Como o jogo funciona hoje (extraído do export)

1. `BP_ThirdPersonCharacter`: overlap com `BochaPortal.InteractionArea` (só se `IsLocallyControlled`) → `NearbyPortal`.
   `IA_Interact` → `Portal.Interact()`.
2. `BochaPortal.Interact` → `PC.ServerRequestStart(Portal)` **(Server, Reliable)**.
3. Servidor: se `!Portal.bMinigameStarted` → `Portal.Multicast_StartMinigame()` **(NetMulticast, Reliable)**.
4. Em **cada máquina**: `bMinigameStarted = true` → `LoadLevelInstance("/Game/Bocha/CanchaMap", (5000,0,0))` →
   `OnLevelShown` → acha ator com tag `CanchaMapStart` → spawna `BP_MiniGameCharacter` ali →
   `PC.MinigameCharacter = ...`, `SetIgnoreMove/LookInput(true)` → spawna `BochaGameMode` **como ator**
   (`TargetCharacter` = minigame char) → `PC.MinigameManager`, `Char.GameMode` → `SetViewTargetWithBlend(char)`.
5. `BochaGameMode.BeginPlay` (local): cria `WBP_StartGame` (AddToViewport desabilitado), spawna `ABolim`
   não-replicado, sem física, preso no character em `(100,0,-30)`; cria `WBP_HUD`. Tecla `C` → câmera na bola.
6. Input do minigame passa pelo `BP_ThirdPersonPlayerController` (IA_BochaMove/Strength/Throw → `DoMove`/`DoStrength`/`DoThrow`
   no `MinigameCharacter` se válido) **e** pelo próprio character (IMC_Bocha prioridade 10).
   - `DoMove(x)`: `AddMovementInput(Capsule.Right, x)`.
   - `DoStrength(y)`: `HUD.SetThrowStrength(HUD.GetThrowStrength() + y*10)` (clamp 100..1000).
   - `DoThrow`: se `bCanThrow` → `SetCanThrow(true)` (!) → `Impulse = Forward*Strength + (0,0,Strength*0.32)` →
     `SendThrow(Impulse, CurrentBall is ABolim)` → `NotifyBolimThrown`/`NotifyBallThrown` → `bCanThrow = true`.
   - `SendThrow`: `PC.ServerThrow(CurrentBall.Location, Impulse, bIsBolim)` **(Server, Reliable)** → `Destroy(CurrentBall)`.
7. Servidor `ServerThrow`: spawna `ABolim` ou `ABocha(bBlue = !IsLocalPlayerController)` replicado, liga física, `AddImpulse`.
8. `NotifyBallThrown`: `ToggleCurrentPlayer` (0↔1) → `SpawnBall(isBlue = CurrentPlayer==1)`.
   `NotifyBolimThrown`: `SpawnBall(CurrentPlayer==1)` (TODO: esperar bola parar).
   `SpawnBall`: spawna `ABocha(bBlue = !IsServer)` local, não replicado, material azul se `isBlue`, preso no character.

## Destino de cada asset

| Asset | Destino |
|---|---|
| `ABall` | `ABochaBall : AActor` — `Sphere` (UStaticMeshComponent, root), `bReplicates`, `bReplicateMovement`. Reparent |
| `ABocha` | `ABochaBocha : ABochaBall` — `UPROPERTY(ReplicatedUsing=OnRep_bBlue, ExposeOnSpawn) bBlue`, `BlueMaterial`. Reparent |
| `ABolim` | `ABochaBolim : ABochaBall` (vazio; escala/massa ficam no BP). Reparent |
| `BochaGameMode` | `ABochaMinigameManager : AActor`. BP reparentado de GameModeBase para ele (o nome do asset continua `BochaGameMode`) ✔ |
| `BP_MiniGameCharacter` | `ABochaMinigameCharacter : ACharacter`. Reparent ✔ (ponte `SendThrow` da Fase 4 já virou C++; EventGraph vazio) |
| `BP_ThirdPersonPlayerController` | `ABochaPlayerController : APlayerController` — RPCs `ServerRequestStart`/`ServerThrow`, roteamento IA_Bocha*, `IA_Interact` + portal próximo, setup de input do template (IMC_Default/MouseLook/touch). Reparent ✔ |
| `BP_ThirdPersonCharacter` | **Sem reparent**: portal próximo e `IA_Interact` foram para o PlayerController; do BP saíram só a variável `NearbyPortal` e os 10 nós do portal. Template (Move/Aim/Jump/touch) intacto ✔ |
| `Lobby/BochaPortal` | `ABochaPortal : AActor` — `MulticastStartMinigame`, level instance do CanchaMap, spawn de character/manager; overlap via `NotifyActorBegin/EndOverlap` (componentes `InteractionArea`/`ABall` continuam no BP). Reparent ✔ |
| `WBP_HUD` | `UBochaHUDWidget : UUserWidget` — `BindWidget` p/ `Slider_Strength`, `P1_Bolim`, `P2_Bolim`; `Get/SetThrowStrength`, `SetP1Bolim`. `AdjustStrength` (não usada, clamp 0..400 contraditório) foi descartada. Reparent ✔ |
| `WBP_StartGame` | Fica em BP: sem lógica (`Button_0` sem uso). `WaitForP2` vazio removido ✔ |
| `UI/BP_MinimapCapture` | `ABochaMinimapCapture : AActor` — esconde o player, timer 0.15s `CaptureScene`. O `SceneCaptureComponent2D` continua no BP: render target e câmera estão configurados na instância do `CanchaMap`. Reparent ✔ |
| `WBP_Minimap` | Fica em BP (sem lógica) |
| `BP_CameraRig` | Não era referenciado e a lógica era morta (Tick punha o ator em 0,0,0). **Apagado** ✔ |
| `Player/BP_PlayerController` | Não era referenciado, grafo vazio. **Apagado** ✔ |
| `Input/*`, `Materials/*`, `RT_Minimap`, `CanchaMap` | Continuam assets; referenciados por `UPROPERTY(EditDefaultsOnly)` |

## Fases

Antes de cada reparent: commit. Editor fechado durante build/reparent.
Reparent via commandlet `BochaReparent` com um JSON por fase em `Docs/reparent/` (sem `-Save` é dry run:
recompila tudo e não grava nada). Verificação depois de salvar: `-Spec=Docs/reparent/verificar.json`.

```bash
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$PWD/BochaNet/BochaNet.uproject" -run=BochaReparent -Spec="$PWD/Docs/reparent/<fase>.json" [-Save] -unattended -nullrhi -nosplash -nocrashreports -stdout
```

Lições da Fase 2:
- Componente/variável do BP com o mesmo nome de um do C++ (`Sphere`, `bBlue`): remover do BP (`removeComponents`,
  `removeVariables`). Depois do reparent o CDO herda o valor antigo (null) no ponteiro nativo e os valores do
  componente voltam ao padrão; o commandlet restaura o ponteiro e `resetComponents` devolve os valores do C++.
- Variáveis tipadas com a classe BP antiga (`CurrentBall : ABall`) precisam de `retypeVariables`, e nós
  `Get Sphere` de `ABall_C` de `retargetMembers`.
- Erros de compilação no meio do log são esperados (dependentes recompilam antes do retype); vale o
  "Dry run OK" final.

2. **Bolas** — `ABochaBall/Bocha/Bolim`. O material passa a ser aplicado em `OnRep_bBlue` + `BeginPlay`
   (hoje há dois lugares aplicando material azul: `ABocha.BeginPlay` e `SpawnBall`).
3. **Widgets** — `UBochaHUDWidget`. Chamadas no Character reapontadas com `retargetFunctions` + `pinRenames`
   (`Strength`→`ReturnValue`, `NewParam`→`Strength`).
4. **MinigameCharacter + MinigameManager** — feitas juntas porque o character chama `Notify*Thrown`/`CurrentBall`.
   - Força fica no character (`ThrowStrength`, começa em 200 = default do slider), HUD só exibe.
   - `SetCanThrow(false)` corrigido.
   - Removido por não ter efeito: prints de debug; eventos IA_Bocha* do próprio character (ele nunca é possuído,
     quem roteia é o PC); cast `GetGameMode → BochaGameMode` (sempre falhava, o Portal seta `GameMode`);
     criação do `WBP_StartGame` (nunca ia pra tela); tecla C → câmera na bola (manager nunca recebia input).
   - Override de `AreaClass` no capsule não portado (capsule do Character não afeta navegação).
5+6. **PlayerController + BochaPortal + LobbyCharacter** — feitas juntas por causa do ciclo Portal ↔ PC.
   - Removido por ser debug: prints "Bora jogar uma bocha" e nomes de pawn/character.
   - Editor aberto durante o build gera DLLs com sufixo (hot reload) e o commandlet roda código velho:
     sempre fechar o editor antes.
7. **MinimapCapture** — componente encontrado por `FindComponentByClass` em vez de migrado para C++.
8. **Limpeza** ✔ — `BP_CameraRig` e `Player/BP_PlayerController` apagados; `WaitForP2` removido.
   O GameMode override do `CanchaMap` já carrega como `None` (o `BochaGameMode` deixou de ser GameMode), então
   o mapa não foi regravado: a referência velha some quando o mapa for salvo no editor.
   `Docs/bp-export/` é um retrato do estado no último export: guarda grafos originais que já não existem
   (ex.: `WBP_HUD/GetThrowStrength.txt`) e pastas de BPs apagados. Para o estado atual, olhar os `_summary.txt`.

## Problemas encontrados no BP

Decididos:

1. **Corrigir (Fase 4):** `DoThrow` chama `SetCanThrow(true)` logo após checar `bCanThrow`; no C++ vira
   `SetCanThrow(false)` — `bCanThrow` volta a `true` só depois do `Notify*Thrown`.
2. **Manter:** `Multicast_StartMinigame` faz todas as máquinas entrarem no minigame quando um jogador interage
   (intencional: 2 jogadores jogam juntos).

Em aberto (portar igual por enquanto):

0. Impulso usa produto componente a componente `Forward * (Strength,0,0)` — só o X do forward conta.
   Igual ao BP; só faz diferença se o character girar.

3. `ServerThrow` confia na `SpawnLocation` e no `Impulse` do cliente, sem validação.
4. Input do minigame é tratado duas vezes (PC roteia para o char **e** o char tem IMC próprio com prioridade 10).
5. **Bug observado no teste da Fase 4 (já existia antes):** no host a bola aparece azul de forma intercalada.
   Causa provável: cor da bola decidida de formas diferentes: local `bBlue = !IsServer`; no servidor `bBlue = !IsLocalPlayerController`;
   material local por `CurrentPlayer == 1`.
6. `AdjustStrength` (clamp 0..400) não é usada; `SetThrowStrength` usa clamp 100..1000.
7. `GetPlayerController(0)` / `GetPlayerCharacter(0)` usados em vários lugares — trocar por owner/controller explícito.
8. TODO "WAIT FOR BALL TO STOP" em `NotifyBolimThrown`.
