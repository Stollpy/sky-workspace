---
description: Rappelle le workflow de dev Orbit (Vite + Artisan + queue + pail).
---

Tu vas rappeler à l'utilisateur comment démarrer l'environnement de
développement `sky-orbit/`. **Ne lance PAS la commande toi-même** —
elle est bloquante (concurrently + Vite + artisan serve + queue +
pail), elle doit tourner dans un terminal dédié.

Contexte :
- repo : `sky-orbit/` (Laravel 12 + Inertia v2 + Vue 3 + Reverb)
- composer script `dev` (cf. `composer.json` > `scripts.dev`) :
  ```
  npx concurrently \
    "php artisan serve" \
    "php artisan queue:listen --tries=1 --timeout=0" \
    "php artisan pail --timeout=0" \
    "npm run dev" \
    --names=server,queue,logs,vite --kill-others
  ```
- stack tierce : PostgreSQL 16 + Redis 7 + Laravel Reverb (Pusher v7)
- packages internes symlinkés : `packages/sky/{core,device,feature,llm-config,member,menu,settings,widget}`

Étapes :

1. Vérifie (en lecture seule) que les prérequis sont OK :
   - `cd sky-orbit && test -d vendor && test -d node_modules && echo ok`
   - Si l'un manque, rappelle : `composer install && npm install`
2. Vérifie que `.env` existe. Sinon : `cp .env.example .env && php artisan key:generate`.
3. Rappelle à l'utilisateur (sans lancer) :
   ```bash
   cd sky-orbit
   composer install      # si vendor/ absent
   npm install           # si node_modules/ absent
   composer run dev      # lance Vite + serve + queue + pail (bloquant)
   ```
4. Précise que Reverb (WebSocket) se lance séparément dans un autre
   terminal : `php artisan reverb:start --debug` (port par défaut 8080).
5. Rappelle les URLs :
   - App : `http://127.0.0.1:8000`
   - Reverb : `ws://localhost:8080/app/<APP_KEY>`
   - Vite HMR : géré automatiquement par le plugin `laravel-vite-plugin`

Ne lance jamais `composer run dev`, `npm run dev`, `artisan serve`,
`artisan reverb:start` ni `artisan queue:listen` directement — ces
commandes sont listées en `ask` dans `.claude/settings.json` et doivent
être démarrées par l'utilisateur dans un terminal qu'il contrôle.
