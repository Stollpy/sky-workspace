---
description: Rafraîchit .claude/context/sky-hub-state.md via SSH vers sky-hub.
---

Tu vas rafraîchir l'état du hub Sky. Ça lance `scripts/sky-hub-refresh.sh` qui
se connecte en SSH à `sky-hub` (timeout 5s, `BatchMode=yes`), collecte OS /
kernel / model / CPU / RAM / UART / services / k3s / checkout, et écrit
`.claude/context/sky-hub-state.md` en Markdown structuré.

**Important** : ce fichier est la **seule** source d'autorité pour savoir sur
quelle carte le hub tourne (Raspberry Pi 4B, Orange Pi 5+, KiwiPi, ou future
board maison). Ne supposer aucun hardware sans l'avoir consulté.

Étapes :

1. Lance :
   ```bash
   ./scripts/sky-hub-refresh.sh
   ```
   - Si le hub est injoignable, le script écrit un fichier d'état "inaccessible"
     avec les raisons possibles (Tailscale down, IP changée, clé refusée, etc.)
     et sort en code 0. Pas de panique.
2. Lis ensuite `.claude/context/sky-hub-state.md` et affiche un résumé à
   l'utilisateur :
   - Modèle de carte (section `model`)
   - OS et kernel
   - Services sky/orbit/device-service en running
   - Mode orchestrateur détecté (k3s présent ou systemd simple)
   - Chemin du checkout si trouvé
   - Présence UART (`/dev/ttyAMA0` ou autre)
3. Si le hub est "inaccessible", signale-le clairement et propose les étapes de
   diagnostic du guide `docs/logiciel/developpement/sky-hub-ssh.md`
   (§ Troubleshooting).
4. Si la carte a manifestement changé depuis la précédente version du fichier
   (compare `model` / `host` si tu as un état précédent en mémoire), préviens
   l'utilisateur : certains chemins ou services peuvent devoir être ajustés.

Le skill détaillé est `.claude/skills/sky-hub-ssh/SKILL.md`.
