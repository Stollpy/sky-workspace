#!/usr/bin/env bash

set -euo pipefail

BASE_URL="${1:-http://192.168.2.99}"
TARGET_PERCENT="${2:-50}"

if ! [[ "$TARGET_PERCENT" =~ ^[0-9]+$ ]] || (( TARGET_PERCENT < 0 || TARGET_PERCENT > 100 )); then
  echo "Erreur: TARGET_PERCENT doit etre entre 0 et 100."
  echo "Usage: $0 [base_url] [target_percent]"
  exit 1
fi

call_post() {
  local path="$1"
  local payload="${2:-}"
  local url="${BASE_URL}${path}"

  local response
  local http_code

  if [[ -n "$payload" ]]; then
    response="$(curl -sS -X POST "$url" \
      -H "Content-Type: application/json" \
      -d "$payload" \
      -w '\n%{http_code}')"
  else
    response="$(curl -sS -X POST "$url" \
      -w '\n%{http_code}')"
  fi

  http_code="${response##*$'\n'}"
  response="${response%$'\n'*}"

  echo "POST $path -> HTTP $http_code"
  echo "$response"
  echo

  if [[ "$http_code" != "200" ]]; then
    echo "Erreur sur $path (HTTP $http_code), arret du script."
    exit 1
  fi
}

echo "=== Test Luxia HTTP Server ==="
echo "Base URL      : $BASE_URL"
echo "Target percent: $TARGET_PERCENT"
echo

echo "1) Demarrage calibration (direction cw)"
call_post "/api/calibrate/start" '{"direction":"cw"}'

read -r -p "Place le moteur sur la position OPEN, puis Entree..."
echo "2) Marquer OPEN"
call_post "/api/calibrate/mark-open"

read -r -p "Place le moteur sur la position CLOSED, puis Entree..."
echo "3) Marquer CLOSED"
call_post "/api/calibrate/mark-closed"

echo "4) Verification statut calibration"
status_response="$(curl -sS "${BASE_URL}/api/calibrate/status")"
echo "GET /api/calibrate/status"
echo "$status_response"
echo

echo "5) Start moteur vers ${TARGET_PERCENT}%"
call_post "/api/motor/move" "{\"percent\":${TARGET_PERCENT}}"

echo "6) Statut global"
global_status="$(curl -sS "${BASE_URL}/api/status")"
echo "GET /api/status"
echo "$global_status"
echo

echo "Termine."
