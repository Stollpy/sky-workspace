#!/usr/bin/env bash
# init-sky-repos.sh — Initialise la structure multi-repo Sky (idempotent).
set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BASE="${SCRIPT_DIR}"

# Couleurs
readonly C_RESET='\033[0m'
readonly C_DIM='\033[2m'
readonly C_RED='\033[0;31m'
readonly C_GREEN='\033[0;32m'
readonly C_YELLOW='\033[0;33m'
readonly C_BLUE='\033[0;34m'
readonly C_CYAN='\033[0;36m'

log_step()    { echo -e "${C_BLUE}▶${C_RESET} $*"; }
log_ok()      { echo -e "${C_GREEN}✓${C_RESET} $*"; }
log_skip()    { echo -e "${C_YELLOW}⋯${C_RESET} $*"; }
log_warn()    { echo -e "${C_YELLOW}!${C_RESET} $*"; }
log_err()     { echo -e "${C_RED}✗${C_RESET} $*" >&2; }

header() {
  echo ""
  echo -e "${C_CYAN}══════════════════════════════════════════════════════════════${C_RESET}"
  echo -e "${C_CYAN}  Sky — initialisation des dépôts${C_RESET}"
  echo -e "${C_DIM}  Base: ${BASE}${C_RESET}"
  echo -e "${C_CYAN}══════════════════════════════════════════════════════════════${C_RESET}"
}

# Écrit le contenu dans un fichier uniquement s'il n'existe pas (idempotent).
write_if_missing() {
  local path="$1"
  local content="$2"
  if [[ -f "$path" ]]; then
    log_skip "Fichier déjà présent, non écrasé : ${path#$BASE/}"
    return 0
  fi
  printf '%s\n' "$content" > "$path"
  log_ok "Créé : ${path#$BASE/}"
}

init_git_if_needed() {
  local dir="$1"
  if [[ -d "$dir/.git" ]]; then
    log_skip "Dépôt git déjà initialisé : ${dir#$BASE/}"
    return 0
  fi
  (cd "$dir" && git init --initial-branch=main >/dev/null 2>&1 || git init >/dev/null)
  log_ok "git init : ${dir#$BASE/}"
}

gitignore_docs() {
  cat <<'EOF'
.DS_Store
*~
*.swp
.idea/
.vscode/
EOF
}

gitignore_esp_idf() {
  cat <<'EOF'
# ESP-IDF
build/
sdkconfig
sdkconfig.old
dependencies.lock
managed_components/
*.pyc
__pycache__/
EOF
}

gitignore_laravel_vue() {
  cat <<'EOF'
# Laravel / PHP
/vendor
.env
.env.*
!.env.example
.phpunit.result.cache
Homestead.json
Homestead.yaml
auth.json
storage/*.key

# Node / front (Vite, Vue)
/node_modules
/public/hot
/public/build
npm-debug.log*
yarn-debug.log*
yarn-error.log*
pnpm-debug.log*

# IDE
.idea/
.vscode/
.fleet/

# OS
.DS_Store
Thumbs.db
EOF
}

gitignore_hub_services() {
  cat <<'EOF'
# Rust
/target/
**/target/

# Node / TypeScript
node_modules/
dist/
build/
*.tsbuildinfo

# Env
.env
.env.*

# IDE / OS
.idea/
.vscode/
.DS_Store
EOF
}

gitignore_infra() {
  cat <<'EOF'
# Secrets & clés (ne jamais committer)
*.pem
*.key
secrets/
*.secret

# Terraform (si utilisé)
.terraform/
*.tfstate
*.tfstate.*

# Ansible vault
*.vault

# Env local
.env
.env.*

# OS
.DS_Store
EOF
}

gitignore_proto() {
  cat <<'EOF'
# Sorties de génération locale (ajuster selon la CI)
/generated/
gen/
dist/
build/

# Tooling Node (CI / scripts)
node_modules/

# Env
.env

.idea/
.vscode/
.DS_Store
EOF
}

CURSOR_HEADER='// TODO: Define context rules for this repo'

process_repo() {
  local name="$1"
  local title="$2"
  local description="$3"
  local gitignore_fn="$4"

  local repo_path="${BASE}/${name}"
  log_step "Dépôt : ${name}"

  mkdir -p "$repo_path"

  init_git_if_needed "$repo_path"

  local readme_md
  readme_md="$(printf '# %s\n\n%s' "$title" "$description")"
  write_if_missing "${repo_path}/README.md" "$readme_md"

  write_if_missing "${repo_path}/.cursorrules" "${CURSOR_HEADER}"

  local gi_content
  gi_content="$("$gitignore_fn")"
  write_if_missing "${repo_path}/.gitignore" "$gi_content"
}

main() {
  header

  # Références : docs/logiciel/adr/adr-008, adr-00-resume, adr-009
  process_repo "sky-docs" \
    "Documentation Sky" \
    "Documentation centralisée de l’écosystème Sky : ADR, guides d’architecture, spécifications et suivi produit (complément au code applicatif)." \
    gitignore_docs

  process_repo "sky-framework" \
    "SDK firmware Sky" \
    "SDK firmware partagé, distribué via ESP-IDF Component Manager (tags SemVer). Base commune pour les produits Luxia, montre et ESP32-C6." \
    gitignore_esp_idf

  process_repo "luxia-firmware" \
    "Firmware Luxia" \
    "Firmware du produit Luxia ; consomme \`sky-framework\` via \`idf_component.yml\` (sans sous-modules git)." \
    gitignore_esp_idf

  process_repo "sky-watch-firmware" \
    "Firmware montre Sky" \
    "Firmware de la montre connectée (ESP32-Pico, BLE + Wi‑Fi), pipeline vocale et intégration avec le hub." \
    gitignore_esp_idf

  process_repo "sky-c6-firmware" \
    "Firmware ESP32-C6 (border router)" \
    "Firmware du coprocesseur ESP32-C6 en rôle de border router / radio Matter pour l’orchestration avec l’OPi5+." \
    gitignore_esp_idf

  process_repo "sky-orbit" \
    "Sky Orbit" \
    "Application web Sky Orbit : Laravel 12, Inertia v2 et Vue 3 — tableau de bord et interface opérateur (PostgreSQL, Redis, Reverb)." \
    gitignore_laravel_vue

  process_repo "sky-hub-services" \
    "Services du hub Sky" \
    "Microservices pont (device-service, STT, TTS, etc.) — principalement Rust et TypeScript selon les besoins de performance." \
    gitignore_hub_services

  process_repo "sky-infra" \
    "Infrastructure Sky" \
    "Charts Helm, Dockerfiles, provisioning (Ansible/cloud-init), scripts k3s et \`docker-compose\` d’environnement de développement local." \
    gitignore_infra

  process_repo "sky-proto" \
    "Registre proto Sky (Matter)" \
    "Registre unique des clusters Matter (\`standard\` + \`sky:v{N}:*\`) ; la CI génère les artefacts C, PHP et TypeScript pour les autres dépôts." \
    gitignore_proto

  echo ""
  log_ok "Terminé. Dépôts traités sous : ${BASE}"
}

main "$@"
