#!/usr/bin/env bash
# Met à jour .claude/context/sky-hub-state.md avec l'état courant du hub SSH.
#
# Usage : ./scripts/sky-hub-refresh.sh
# Env   : SKY_HUB_SSH_HOST (défaut: sky-hub), SKY_HUB_SSH_TIMEOUT (défaut: 5)
#
# Idempotent, gère l'absence du hub (timeout) sans planter. Appelé par le hook
# SessionStart (en arrière-plan, détaché) et par le slash command /sky-hub-refresh.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/.claude/context/sky-hub-state.md"
mkdir -p "$(dirname "$OUT")"

HOST="${SKY_HUB_SSH_HOST:-sky-hub}"
TIMEOUT="${SKY_HUB_SSH_TIMEOUT:-5}"

ts="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

# Ping SSH : timeout court, pas de prompt. Si ça rate → état "inaccessible".
if ! ssh -o ConnectTimeout="$TIMEOUT" \
        -o BatchMode=yes \
        -o StrictHostKeyChecking=accept-new \
        "$HOST" true 2>/dev/null; then
  cat > "$OUT" <<EOF
# Sky Hub — état (inaccessible)

> Dernière tentative : \`$ts\`
> Hôte : \`$HOST\` — **injoignable** (timeout ${TIMEOUT}s ou auth refusée).

Le hub SSH n'a pas répondu. Raisons possibles :

- hub éteint,
- tunnel Tailscale down (\`tailscale status\`),
- IP LAN changée, mDNS cassé,
- clé publique plus dans \`~sky/.ssh/authorized_keys\` (après réinstall),
- empreinte SSH changée (carte réimagée → \`ssh-keygen -R $HOST\`).

Relancer : \`./scripts/sky-hub-refresh.sh\` ou \`/sky-hub-refresh\`.
Voir \`docs/logiciel/developpement/sky-hub-ssh.md\` § Troubleshooting.
EOF
  echo "Sky Hub injoignable → $OUT" >&2
  exit 0
fi

# Exécution distante : préfixe chaque section par "### <nom>".
raw="$(ssh -o ConnectTimeout="$TIMEOUT" "$HOST" '
  echo "### os";            cat /etc/os-release 2>/dev/null | grep -E "^(PRETTY_NAME|VERSION_ID|ID)=" || echo "n/a"
  echo "### kernel";         uname -a
  echo "### model";          (cat /proc/device-tree/model 2>/dev/null | tr -d "\0"; echo) || echo "n/a"
  echo "### cpu";            lscpu 2>/dev/null | grep -E "^(Model name|Architecture|CPU\(s\)):" || echo "n/a"
  echo "### mem";            free -h | head -3
  echo "### disk";           df -h / | tail -1
  echo "### host";           hostname; hostnamectl 2>/dev/null | grep -E "^[[:space:]]*(Static hostname|Machine ID|Boot ID|Operating System|Kernel|Architecture|Hardware Vendor|Hardware Model):" || true
  echo "### net";            ip -4 -br addr show | grep -v "^lo" || echo "n/a"
  echo "### uart";           ls -la /dev/serial0 /dev/ttyAMA0 /dev/ttyS0 /dev/ttyS2 2>/dev/null || echo "aucun device série classique trouvé"
  echo "### boot-uart-cfg";  cat /boot/firmware/config.txt 2>/dev/null | grep -E "^(dtoverlay|enable_uart|init_uart|core_freq)" | head -20 || echo "n/a (pas un boot RPi)"
  echo "### services";       systemctl list-units --type=service --state=running --no-pager --no-legend 2>/dev/null | awk "{print \$1}" | grep -iE "orbit|reverb|device-service|k3s|postgres|redis|php|nginx|hostapd|otbr|bluez|avahi|dnsmasq" | sort -u || echo "aucun service sky détecté"
  echo "### k3s-pods";       if command -v kubectl >/dev/null 2>&1; then kubectl get pods -A --no-headers 2>/dev/null | head -20 || echo "kubectl présent mais inaccessible"; else echo "kubectl: non installé"; fi
  echo "### checkout";       ls -d ~/stollper-pcb ~/sky ~/sky-hub 2>/dev/null || true; find ~ -maxdepth 3 -type d -name sky-hub-services 2>/dev/null | head -3
  echo "### device-service-bin"; bin="$(find ~ -type f -name device-service -path "*release*" 2>/dev/null | head -1)"; if [ -n "$bin" ]; then ls -la "$bin"; else echo "binaire release non trouvé"; fi
' 2>&1)"

# Formatage Markdown : chaque section "### x" devient "## x" suivi d'un bloc ```fenced```.
{
  echo "# Sky Hub — état courant"
  echo ""
  echo "> Rafraîchi : \`$ts\` — via \`ssh $HOST\`"
  echo "> Source : \`scripts/sky-hub-refresh.sh\`"
  echo ""
  echo "> Le hardware réel varie (RPi4B dev / OPi5+ prod / KiwiPi alt / custom futur)."
  echo "> Toujours lire la section \`model\` avant d'assumer quoi que ce soit."
  echo ""
  awk '
    BEGIN { in_block = 0 }
    /^### / {
      if (in_block) { print "```"; print "" }
      # Ligne "### name" → titre H2 + fence ouvert
      sub(/^### /, "## ", $0)
      print $0
      print "```"
      in_block = 1
      next
    }
    { if (in_block) print $0 }
    END { if (in_block) print "```" }
  ' <<< "$raw"
} > "$OUT"

echo "Sky Hub state refreshed → $OUT"
