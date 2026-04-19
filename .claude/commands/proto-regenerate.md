---
description: Valide et régénère les artefacts sky-proto (C, PHP, TS) et montre le diff.
---

Tu vas régénérer les artefacts générés depuis les définitions de
clusters Matter dans `sky-proto/`. C'est la source de vérité unique
des clusters pour toute la stack Sky.

Contexte :
- repo : `sky-proto/`
- clusters source : `clusters/standard/` (Matter standard 0x0000-0x7FFF)
  et `clusters/sky-custom/` (namespace `sky:v{N}:*`, 0xFC00-0xFFFE)
- générateurs : `generators/{c,php,typescript}/`
- sortie : `generated/` (gitignored — ne pas commit ici, les artefacts
  sont poussés vers les repos cibles par la CI `.github/workflows/generate.yml`)

Étapes :

1. Vérifie que `sky-proto/node_modules/` existe. Sinon lance
   `cd sky-proto && npm ci`.
2. Exécute en séquence :
   ```bash
   cd sky-proto
   npm run validate
   npm run generate
   ```
   - `validate` → `bash scripts/validate.sh` (ajv sur `schemas/cluster.schema.json`)
   - `generate` enchaîne `generate:c`, `generate:php`, `generate:ts`
3. Une fois terminé, affiche un résumé :
   - Liste des fichiers nouvellement générés par langue
   - `git -C sky-proto status --short generated/` pour voir ce qui a bougé
   - Si des fichiers `generated/` ont changé, propose `git -C sky-proto diff generated/ | head -100`
4. N'écris pas dans `generated/` manuellement. Ne commit pas non plus
   (la CI s'en charge). Informe l'utilisateur que pour diffuser, il
   faut pousser la source dans `clusters/` puis la CI pousse les artefacts
   vers les repos cibles (sky-framework, sky-orbit, sky-hub-services).

En cas d'échec de validation :
- Affiche le message AJV complet
- Pointe vers `schemas/cluster.schema.json` pour vérifier le schéma
- Vérifie que le nom de fichier suit `{id_hex}-{kebab-name}.json`
