# Sky Hub — état courant du développement

> Rafraîchi : `2026-04-17` — à la main (audit complet des repos + `git log`)
> Source : scan direct des 9 repos Sky + `.cursorrules` des jalons
> Pendant : `.claude/context/sky-hub-state.md` (matériel runtime du hub)

> **Règle d'usage** : avant d'annoncer "il faut faire X", relire ce fichier pour
> ne pas re-proposer ce qui est déjà fait. Mettre à jour à la fin d'une session
> qui modifie l'état d'un jalon/milestone. Rafraîchissable via
> `scripts/sky-dev-state-refresh.sh` (scan git log + structure repo).

## Résumé 1-ligne

J1 livré à ~90 %, J2 livré à ~85 % (Phase A Matter+Wi-Fi bout-en-bout fonctionne,
reste finition CRC UART, push CI sky-proto → consommateurs, M6 E2E). J3 à peine
scaffoldé. J4-J6 non démarrés. Bloqueurs matériels : ESP32-C6 (Phase B Thread),
KiwiPi Pro (benchs prod). Rien de critique bloqué.

## Repos — snapshot

| Repo | Dernier commit | État global |
|------|----------------|-------------|
| `sky-proto` | `a556456` claude code | F0.1/F0.2/F0.3 écrits, CI pas validée en prod |
| `sky-framework` | `90766c4` delete log | `sky_proto` headers + `sky_serial` livrés (+ fix CRC UART récent) |
| `sky-orbit` | `f28c091` websocket connexion hub | J1 complet + contrôle réel Window Covering + pairing |
| `sky-hub-services` | `738c122` fix CRC UART antenna | `device-service` Rust complet (serial, state, websocket) |
| `sky-luxia` | `5fa1c59` claude code | Matter Window Covering + Wi-Fi commissioning OK |
| `sky-hub-antenna` | `763aa44` c6 bridge | Phase A WROOM-32 : bridge Matter + série + Wi-Fi |
| `sky-infra` | local | Ansible/Helm/docker/manifests/terraform scaffolds |
| `sky-watch` | — | uniquement `README.md` + `CLAUDE.md` (scaffold) |
| `sky-hub-antenna-wroom` | — | variante bridge WROOM, à inspecter |

## Jalon 1 — k3s + Orbit sur écran (Mois 1-2)

| Milestone | État | Notes |
|-----------|------|-------|
| M0 scaffolding architecture | ✅ | `sky-orbit/packages/sky/*` : core, feature, menu, widget, device, member, settings, llm-config |
| M1 infrastructure as code | ⚠️ partiel | `sky-infra/` contient ansible + helm + manifests + terraform ; à valider sur KiwiPi quand elle arrive |
| M2 Orbit core application | ✅ | auth RBAC + layout + Reverb connecté (commits `permission registrar`, `websocket connexion hub`) |
| M3 dashboard widgets | ✅ | commit `responsive UI on widget components` |
| M4 feature members | ✅ | `packages/sky/member/` + `MemberSeeder.php` |
| M5 feature devices | ✅ | `packages/sky/device/` + `Device.php` + `PairingRequest.php` |
| M6 feature settings | ✅ | `packages/sky/settings/` |
| M7 feature LLM config | ✅ | `packages/sky/llm-config/` + `LlmConfigSeeder.php` (squelette pour J4) |
| M8 dev environment | ⚠️ partiel | tests PHPUnit scaffoldés (`tests/Feature`, `tests/Unit`), docker-compose présent |
| M9 deployment validation | ⚠️ partiel | validé sur RPi4B actuel (services up), à rejouer sur KiwiPi |

## Jalon 2 — Luxia via Matter + Wi-Fi (Mois 2-3)

| Milestone | État | Notes |
|-----------|------|-------|
| **M0 sky-proto registre clusters** | ✅ 95 % | voir détail ci-dessous |
| M1 sky-framework sky_serial | ✅ | `components/sky_serial/` : `{framing,crc,types}` + `src/{framing,crc}.c` ; intégration ESP-IDF Component Manager version 0.2.0 |
| M2 firmware ESP32 bridge (Phase A WROOM) | ✅ | `sky-hub-antenna/main/` : `bridge_{matter,serial,wifi,commands}` — relay Matter Wi-Fi via UART |
| M3 firmware Luxia Wi-Fi fabric | ✅ | Phase A : `luxia_{matter,motor,wifi,cmd,http}` — commissioning Wi-Fi OK. Phase B (Thread) attend le C6. |
| M4 device-service bridge Rust | ✅ | `sky-hub-services/device-service/src/{config,main,serial/{framing,port,transport},state/{cache,commissioning,registry},websocket/{client,commands,events}}` — tout est là |
| M5 Orbit contrôle réel devices | ✅ | commit `real device control with pairing workflow and Window Covering UI` |
| M6 intégration & validation E2E | ❓ | à vérifier : tests E2E formalisés ? 5 critères d'acceptation passés ? |

### M0 sky-proto — détail

| Tâche | État | Remarque |
|-------|------|----------|
| F0.1-T01 init repo | ✅ | structure conforme |
| F0.1-T02 JSON Schema | ✅ | `schemas/cluster.schema.json` draft-07 strict |
| F0.1-T03 README | ✅ | README.md + CLAUDE.md complets |
| F0.2-T01 cluster 0x0102 | ✅ | 7 attrs + 4 cmds (UpOrOpen, DownOrClose, StopMotion, GoToLiftPercentage) revision 5 |
| F0.2-T02 validation schema | ✅ | `scripts/validate.sh` + `npm run validate` |
| F0.3-T01 gén. headers C | ✅ local | `generated/c/sky_cluster_window_covering.h` conforme |
| F0.3-T02 gén. classes PHP | ✅ local | `generated/php/WindowCovering/{AttributeId,CommandId,Attributes}.php` |
| F0.3-T03 gén. types TS | ✅ local | `generated/typescript/window-covering.ts` |
| F0.3-T04 GitHub Actions | ⚠️ | workflow écrit mais jamais vérifié en prod — les 3 deploy keys (`SKY_{FRAMEWORK,ORBIT,HUB_SERVICES}_DEPLOY_KEY`) sont-elles configurées sur GitHub ? |

### Restes M0 (à faire pour valider)

1. **Vérifier GitHub Actions** : pousser un petit changement sur sky-proto `main`,
   observer le workflow `Validate & Generate`, et que les deploy keys laissent
   push-targets cloner les 3 repos cibles.
2. ✅ **Path C aligné** (2026-04-17) : le header a été déplacé dans
   `components/sky_proto/include/sky_proto/sky_cluster_window_covering.h`
   pour correspondre à la destination du workflow CI. Includes dans
   `sky-hub-antenna/main/` mis à jour en `#include "sky_proto/sky_cluster_window_covering.h"`.
3. **sky-orbit PHP codegen** : pas de dossier `packages/sky-proto/` ; les classes
   PHP générées (`AttributeId`, `CommandId`, `Attributes`) ne sont consommées
   nulle part. À importer si M5/M6 veulent typer fort la pile cluster côté PHP.
4. **sky-hub-services TS codegen** : idem, pas de `packages/sky-proto/` ; le
   `device-service` en Rust n'en a pas besoin (travail direct en bytes), mais si
   un pod TS (orchestrateur, STT wrapper) apparaît plus tard, il consommera ça.
5. **clusters/sky-custom/** : dossier vide. Définir `sky:v1:voice-command`
   (0xFC01) et `sky:v1:watch-sensor` (0xFC02) est prévu pour J3, pas bloquant J2.
6. **Publier sur ESP-IDF Component Manager** : le composant est versioné 0.2.0,
   mais semble tiré via submodule/local plutôt qu'enregistré. À faire quand
   l'API est stable.

## Jalon 3 — Montre BLE/Wi-Fi (Mois 3-5) — parallèle J4

| Milestone | État | Notes |
|-----------|------|-------|
| Tout | ❌ | `sky-watch/` contient juste `README.md` + `CLAUDE.md`. Matos capteurs (SpO2+HR, gyro/accelo) reçu selon l'utilisateur — le scaffold ESP-IDF complet (NimBLE, Wi-Fi, écran I2S micro) est à lancer. |

## Jalon 4 — LLM tool calling (Mois 3-5) — parallèle J3

| Milestone | État | Notes |
|-----------|------|-------|
| Tout | ❌ | Le package `sky/llm-config` existe côté Orbit (J1-M7), mais Ollama/STT/router backend non démarrés. Benchmarks inférence locale pas commencés. |

## Jalon 5 — Pipeline vocal complète (Mois 5-6)

Dépend de J3 + J4. Pas commencé.

## Jalon 6 — TTS + notifications (Mois 6-7)

Dépend de J5. Pas commencé.

## Incidents / bugs actifs

- **CRC UART "corrupted data send on bridge antenna"** : fix appliqué sur
  `sky-framework@90766c4` + `sky-hub-services@738c122`. Post-mortem dans
  `docs/logiciel/developpement/troubleshooting-sky-serial-crc.md`. À surveiller
  (mémoire utilisateur : table CRC corrompue sky-framework + self-test proposé).

## Matériel en attente

| Matos | Bloque | Workaround actuel |
|-------|--------|-------------------|
| ESP32-C6 | J2 Phase B (OTBR/Thread), Luxia prod | WROOM-32 en Phase A — code applicatif identique |
| KiwiPi Pro | perf prod pipeline vocale, benchs LLM cible | RPi4B (`sky-hub`) en dev : Debian trixie aarch64, 1.8 GiB RAM, 29 GB disk |

## Prochaine étape recommandée (logique de dépendance)

1. **M0-6 finition sky-proto** (~1 j) : valider workflow CI, aligner path C,
   pousser les artefacts aux consommateurs si utiles.
2. **J2-M6 E2E Luxia** (~1-2 j) : écrire les 5 critères d'acceptation formels,
   tests bout-en-bout Orbit → device-service → bridge → Luxia → store bouge.
3. **J3 scaffold `sky-watch`** (~3-5 j) : projet ESP-IDF + NimBLE +
   Wi-Fi + driver SpO2/HR (MAX30102 ou équivalent) + driver gyro/accelo
   (MPU6050/ICM) + écran. Squad Watch peut avancer en parallèle.
4. **Luxia produit** (mécanique + design) : moteur long run, batterie, PCB KiCad,
   design 3D — indépendant du soft, non bloquant.
5. **J4 Ollama/LLM** : benchmarks inférence sur RPi4B actuel pour dimensionner
   la KiwiPi Pro quand elle arrive.

## Pointeurs

- Règles transversales : `../CLAUDE.md`
- Jalons détaillés : `../docs/logiciel/jalons/jalon-{1..6}-*/`
- ADR autorité : `../docs/logiciel/adr/`
- État hub runtime : `sky-hub-state.md`
- Mémoire utilisateur : `../memory/MEMORY.md`
