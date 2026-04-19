---
name: sky-hub-ssh
description: >-
  Guide les opérations live sur le hub Sky via SSH (`ssh sky-hub`). À utiliser quand
  l'utilisateur demande de se connecter au hub, diagnostiquer le hub, lire les logs
  du hub, rebuilder `device-service` sur ARM, vérifier les pods k3s sur le hub,
  inspecter l'UART du Pi/OPi, ou faire du debug UART/Matter/Reverb live sur `sky-hub`.
  Le matériel du hub peut varier (RPi 4B en dev, OPi5+ cible prod, KiwiPi alt,
  future board maison) — toujours lire `.claude/context/sky-hub-state.md` d'abord.
---

# Sky Hub SSH — ops live

## Connexion

| Clé | Valeur |
|-----|--------|
| Alias SSH | `sky-hub` (ou `$SKY_HUB_SSH_HOST`) |
| Utilisateur | `sky` (clé SSH déjà provisionnée côté mac) |
| Mot de passe sudo | `pwdno` |
| Groupes du user | `sky` ∈ `www-data` |
| État courant | `.claude/context/sky-hub-state.md` (rafraîchi par `scripts/sky-hub-refresh.sh`) |

```bash
ssh sky-hub                                   # shell interactif
ssh sky-hub "<cmd>"                           # one-shot
ssh -o ConnectTimeout=5 sky-hub true          # ping auth
```

**Avant** toute session de debug sur le hub : lire `.claude/context/sky-hub-state.md`.
Si le fichier dit "inaccessible" ou date de > 1 h, relancer `./scripts/sky-hub-refresh.sh`
(ou `/sky-hub-refresh`). Ne **jamais supposer** que c'est un RPi 4B ou un OPi5+ — vérifie.

## Sudo non-interactif

```bash
ssh sky-hub "echo pwdno | sudo -S <cmd>"
```

OK pour `systemctl status/restart`, `journalctl`, `kubectl` (si `sudo` requis), lecture
de configs système. **À proscrire** pour tout ce qui écrit hors `~sky/` : `apt`,
`ansible-playbook`, `helm install`, `cp /etc/...`. Ces actions-là se font via
`sky-infra` / `sky-infra-min` et un prompt explicite de l'utilisateur.

## Découvrir le hardware

Base minimum pour savoir sur quelle carte on tape :

```bash
ssh sky-hub '
  uname -a
  cat /etc/os-release 2>/dev/null | grep -E "^(PRETTY_NAME|VERSION_ID|ID)="
  cat /proc/device-tree/model 2>/dev/null | tr -d "\0"; echo
  lscpu | grep -E "^(Model name|Architecture|CPU\(s\)):"
  free -h | head -3
  df -h /
  lsblk
  ip -4 -br addr show | grep -v "^lo"
  hostname; hostnamectl 2>/dev/null | head -10
  ls /dev/tty* 2>/dev/null | head -20
  cat /boot/firmware/config.txt 2>/dev/null | grep -E "^(dtoverlay|enable_uart|init_uart|core_freq)" || true
'
```

Indices pour deviner la carte :
- `Raspberry Pi 4 Model B` + `BCM2711` + `/boot/firmware/config.txt` → **RPi 4B** (fallback dev)
- `RK3588S` / `Orange Pi 5 Plus` / arch `aarch64` 8 cores → **OPi5+** (cible prod)
- `Allwinner` → **KiwiPi** alt
- plus tard : `sky-container` ou `sky-hub` (board maison) — voir `firmware/.cursor/plans/05-sky-container/` et `07-sky-hub/`

## Découvrir le mode d'orchestration (k3s vs systemd simple)

```bash
ssh sky-hub '
  systemctl list-units --type=service --state=running --no-pager --no-legend 2>/dev/null \
    | awk "{print \$1}" | grep -iE "orbit|reverb|device-service|k3s|postgres|redis|php|nginx|hostapd|otbr|bluez"
  which kubectl k3s 2>/dev/null
  ls /etc/systemd/system/ 2>/dev/null | grep -iE "orbit|reverb|device-service|k3s"
'
ssh sky-hub "echo pwdno | sudo -S systemctl status device-service orbit-reverb orbit-worker k3s 2>/dev/null | head -80"
```

Deux variantes possibles (voir ADR-007) :
- **`sky-infra-min`** (dev minimal) : systemd + PostgreSQL + Node + PHP-FPM + nginx + `device-service`.
- **`sky-infra`** (prod) : k3s single-node + Helm + BlueZ/OTBR/hostapd bare-metal.

## Commandes courantes

### Logs

```bash
# 200 dernières lignes
ssh sky-hub "journalctl -u device-service -n 200 --no-pager"

# Suivi live (ctrl-c pour quitter)
ssh sky-hub "journalctl -u device-service -f --no-pager"

# Multi-unit
ssh sky-hub "journalctl -u device-service -u orbit-reverb -f --no-pager"

# En mode k3s : logs du pod
ssh sky-hub "echo pwdno | sudo -S kubectl logs -n sky -l app=device-service --tail=200 -f"
```

### Restart de service

```bash
ssh sky-hub "echo pwdno | sudo -S systemctl restart device-service"
ssh sky-hub "echo pwdno | sudo -S systemctl restart orbit-reverb orbit-worker"
```

### UART (bus série co-processeur)

```bash
# Vérifier que le device existe (RPi4B → ttyAMA0, OPi5+ → ttyS*, à confirmer)
ssh sky-hub "ls -la /dev/serial0 /dev/ttyAMA0 /dev/ttyS0 /dev/ttyS2 2>/dev/null"

# Kernel logs UART
ssh sky-hub "dmesg | grep -i uart | tail -10"

# Config RPi
ssh sky-hub "cat /boot/firmware/config.txt 2>/dev/null | grep -E 'dtoverlay|enable_uart'"

# Test TX brut vers ESP32 (2 octets magic 'SK' en LE → 0x4B, 0x53)
ssh sky-hub "echo -ne '\x4B\x53' > /dev/ttyAMA0"
```

Référence : `docs/logiciel/developpement/cablage-serie-rpi4b-esp32.md` (câblage 3 fils,
PL011 libéré du BT via `dtoverlay=disable-bt`, user dans `dialout`).

### k3s

```bash
ssh sky-hub "echo pwdno | sudo -S kubectl get pods -A"
ssh sky-hub "echo pwdno | sudo -S kubectl get nodes -o wide"
ssh sky-hub "echo pwdno | sudo -S kubectl describe pod -n sky <pod>"
```

## Rebuild `device-service` sur le hub (obligatoire : ARM, pas de cross-compile)

Chemin du checkout à **confirmer à la première session** et noter dans le state file
(`~/stollper-pcb`, `/opt/sky`, `/srv/sky`, ou autre). Exemple type :

```bash
# 1. Pull local → push → ssh + pull + build
#    (le code source vit dans le monorepo, on push depuis le mac)

ssh sky-hub "cd ~/stollper-pcb/sky-hub-services/device-service && git pull --ff-only && cargo build --release 2>&1 | tail -40"
ssh sky-hub "echo pwdno | sudo -S systemctl restart device-service"
ssh sky-hub "journalctl -u device-service -f --no-pager"
```

Si le binaire est installé par `cargo install` ou par un hook Ansible, le chemin et
la commande de restart changent. Vérifier dans `.claude/context/sky-hub-state.md`
(section `checkout` et `device-service-bin`).

## Ce qu'on ne fait PAS via SSH

- **Éditer du code destiné à git** (Rust, PHP, C, TS) → édition locale sur mac →
  commit + push → `git pull` + rebuild sur le hub. Pas de vim sur le hub pour
  éviter les divergences de branche.
- **Modifier `/etc/…`, `/boot/…`, installer des paquets** sans `sudo` explicite
  accordé par l'utilisateur et idéalement via Ansible (`sky-infra/` ou `sky-infra-min/`).
- **`rm -rf` destructif** dans `~sky` ou `/var` → demander confirmation à chaque fois.
- **`reboot` ou `systemctl poweroff`** sans demander : couper le hub coupe le debug live.
- **Changer la config UART** (`/boot/firmware/config.txt`) sans coordonner avec le
  firmware bridge : ce serait déphaser RPi et ESP32.

## Références

- Guide humain complet : `docs/logiciel/developpement/sky-hub-ssh.md`
- Câblage UART : `docs/logiciel/developpement/cablage-serie-rpi4b-esp32.md`
- Reverb ↔ Rust : `docs/logiciel/developpement/reverb-compatibilite-rust.md`
- ADR-001 (transport), ADR-007 (infra k3s), ADR-014 (offline-first)
- État live du hub : `.claude/context/sky-hub-state.md`
- Refresh : `./scripts/sky-hub-refresh.sh` ou slash command `/sky-hub-refresh`
