#!/usr/bin/env bash
# sky-worktree.sh — Git worktrees pour travailler plusieurs jalons en parallèle (repos Sky).
# Usage : voir docs/logiciel/developpement/git-worktrees-sky.md
set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BASE="$(cd "${SCRIPT_DIR}/.." && pwd)"
readonly WORKTREE_ROOT="${SKY_WORKTREE_ROOT:-${BASE}/worktrees}"

# Dépôts Sky sous BASE (même liste que init-sky-repos.sh, sans sky-docs).
readonly SKY_REPOS=(
  luxia-firmware
  sky-c6-firmware
  sky-framework
  sky-hub-services
  sky-infra
  sky-orbit
  sky-proto
  sky-watch-firmware
)

log() { echo "[sky-worktree] $*"; }
err() { echo "[sky-worktree] ERREUR: $*" >&2; exit 1; }

repo_path() {
  local name="$1"
  echo "${BASE}/${name}"
}

ensure_initial_commit() {
  local name="$1"
  local r
  r="$(repo_path "$name")"
  [[ -d "${r}/.git" ]] || err "Pas de dépôt git : ${r}"
  if git -C "$r" rev-parse HEAD >/dev/null 2>&1; then
    log "${name}: commit de base OK"
    return 0
  fi
  log "${name}: aucun commit — création d'un commit vide (nécessaire pour les worktrees)"
  git -C "$r" add -A 2>/dev/null || true
  git -C "$r" commit --allow-empty -m "chore: initial commit (worktree baseline)" \
    || err "Impossible de créer le commit initial dans ${name}"
}

cmd_ensure_all() {
  local n
  for n in "${SKY_REPOS[@]}"; do
    ensure_initial_commit "$n"
  done
  log "Tous les dépôts Sky ont au moins un commit."
}

cmd_add() {
  local name="$1"
  local slug="$2"
  [[ -n "$name" && -n "$slug" ]] || err "usage: add <repo> <slug-jalon>   ex: add sky-orbit 1-k3s-orbit-ecran"
  local r wt branch
  r="$(repo_path "$name")"
  [[ -d "${r}/.git" ]] || err "Dépôt inconnu : ${name}"
  ensure_initial_commit "$name"
  branch="jalon/${slug}"
  wt="${WORKTREE_ROOT}/${name}/${slug}"
  mkdir -p "$(dirname "$wt")"
  if [[ -d "$wt" ]]; then
    err "Le chemin existe déjà : ${wt}"
  fi
  if git -C "$r" show-ref --verify --quiet "refs/heads/${branch}"; then
    log "Branche existante ${branch} — attachement du worktree"
    git -C "$r" worktree add "$wt" "$branch"
  else
    log "Création branche ${branch} depuis HEAD"
    git -C "$r" worktree add -b "$branch" "$wt"
  fi
  log "Worktree prêt : ${wt}"
  log "Branche : ${branch}"
}

cmd_list() {
  local n r
  for n in "${SKY_REPOS[@]}"; do
    r="$(repo_path "$n")"
    [[ -d "${r}/.git" ]] || continue
    echo "=== ${n} ==="
    git -C "$r" worktree list || true
    echo ""
  done
}

cmd_remove() {
  local name="$1"
  local slug="$2"
  [[ -n "$name" && -n "$slug" ]] || err "usage: remove <repo> <slug-jalon>"
  local r wt
  r="$(repo_path "$name")"
  wt="${WORKTREE_ROOT}/${name}/${slug}"
  [[ -d "${r}/.git" ]] || err "Dépôt inconnu : ${name}"
  [[ -d "$wt" ]] || err "Worktree introuvable : ${wt}"
  git -C "$r" worktree remove --force "$wt"
  log "Worktree supprimé : ${wt}"
}

cmd_prune() {
  local n r
  for n in "${SKY_REPOS[@]}"; do
    r="$(repo_path "$n")"
    [[ -d "${r}/.git" ]] || continue
    git -C "$r" worktree prune -v
  done
}

usage() {
  cat <<EOF
Usage:
  $(basename "$0") ensure-commits     — garantit un commit dans chaque repo Sky (requis pour worktree)
  $(basename "$0") add <repo> <slug> — ajoute worktrees/<repo>/<slug> sur branche jalon/<slug>
  $(basename "$0") list              — liste les worktrees de chaque repo
  $(basename "$0") remove <repo> <slug> — supprime un worktree (dossier + entrée git)
  $(basename "$0") prune             — nettoie les worktrees orphelins

Variable d'environnement :
  SKY_WORKTREE_ROOT   — racine des worktrees (défaut : ${BASE}/worktrees)

Exemple :
  $(basename "$0") ensure-commits
  $(basename "$0") add sky-orbit 1-k3s-orbit-ecran
EOF
}

main() {
  case "${1:-}" in
    ensure-commits) cmd_ensure_all ;;
    add) shift; cmd_add "${1:-}" "${2:-}" ;;
    list) cmd_list ;;
    remove) shift; cmd_remove "${1:-}" "${2:-}" ;;
    prune) cmd_prune ;;
    -h|--help|help) usage ;;
    *) usage; exit 1 ;;
  esac
}

main "$@"
