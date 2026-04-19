---
description: Liste les worktrees ouverts sur chaque repo Sky + résume l'état courant.
---

Tu vas inspecter les worktrees actifs sur l'ensemble des repos Sky.

Étapes :

1. Exécute `./scripts/sky-worktree.sh list`.
2. Parse la sortie (sections `=== <repo> ===` suivies de la sortie
   brute de `git worktree list`).
3. Résume sous forme de tableau Markdown :
   - Colonnes : `Repo`, `Worktree path`, `Branch`, `HEAD court`
   - Note les branches `jalon/*` pour les distinguer de `main`.
4. Si un repo n'apparaît pas (pas de `.git`), signale-le.
5. Si un worktree semble orphelin (chemin qui n'existe plus), suggère
   `./scripts/sky-worktree.sh prune`.

Utilise la convention de slugs du projet : `jalon-<n>-<nom-court>` (ex.
`jalon/1-k3s-orbit-ecran`). Rappelle le chemin canonique
`worktrees/<repo>/<slug>/` si utile.

Ne modifie rien — lecture seule.
