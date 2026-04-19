---
description: Ajoute un git worktree pour un jalon sur un repo Sky donné.
argument-hint: <repo> <slug-jalon> (ex. sky-orbit 1-k3s-orbit-ecran)
---

Tu vas créer un nouveau worktree pour travailler un jalon en parallèle,
selon le workflow décrit dans `docs/logiciel/developpement/git-worktrees-sky.md`.

Contexte :
- repo cible : `$1`
- slug du jalon : `$2`
- le script crée `worktrees/$1/$2/` sur la branche `jalon/$2`
- repos valides : `sky-luxia`, `sky-luxia-s3`, `sky-hub-antenna`,
  `sky-hub-antenna-wroom`, `sky-framework`, `sky-hub-services`,
  `sky-infra`, `sky-infra-min`, `sky-orbit`, `sky-proto`,
  `sky-watch` (liste dans `scripts/sky-worktree.sh`)

Étapes à exécuter :

1. Si `$1` ou `$2` sont vides, affiche un rappel d'usage et arrête.
2. Lance la commande : `./scripts/sky-worktree.sh add $1 $2`
3. Si ça réussit, exécute `git -C worktrees/$1/$2 status` pour montrer
   l'état initial du worktree (HEAD, branche `jalon/$2`).
4. Rappelle à l'utilisateur la commande pour y entrer :
   `cd worktrees/$1/$2`
5. Ne modifie rien dans le worktree — seulement l'afficher.

Si l'utilisateur n'a pas encore exécuté `ensure-commits`, le script le
fait implicitement pour le repo visé (commit vide si nécessaire).
