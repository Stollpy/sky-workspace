---
description: Résume l'avancement d'un jalon à partir de ses .cursorrules (M*/F*).
argument-hint: <numero-jalon> (ex. 2)
---

Tu vas produire un bilan synthétique d'un jalon Sky en lisant ses
règles `.cursorrules` et les specs `Tnn-*.md`.

Contexte :
- numéro du jalon : `$1`
- racine des jalons : `docs/logiciel/jalons/jalon-$1-*/`
- nommage strict (docs/logiciel/jalons/.cursorrules) :
  `Mnn-*/Fnn.nn-*/Tnn-*.md`

Étapes :

1. Si `$1` est vide, liste les 6 jalons disponibles et demande
   lequel résumer.
2. Trouve le dossier `docs/logiciel/jalons/jalon-$1-*/` (Glob).
3. Lis le `.cursorrules` transversal + le `.cursorrules` du jalon
   pour récupérer : périmètre, ADR référencés, critères de succès,
   langages autorisés, squads.
4. Pour chaque milestone `M*/` présent :
   - Lis son `.cursorrules`
   - Liste les features `F*/` (titre + statut si mentionné)
5. Produit en sortie :
   - Titre du jalon, mois cible, statut global
   - ADR applicables
   - Liste `M0..Mn` avec une ligne de résumé chacun
   - Features en cours ou bloquées si l'info est dans un `Tnn-*.md`
   - Squads impliqués
6. Ne lis pas chaque `Tnn-*.md` en entier — trop volumineux. Contente-toi
   du `.cursorrules` et des noms de features.

Réfère-toi au CLAUDE.md racine (tableau "Jalons") pour le contexte global.
