# Étape 4 — Passage à SDL2 : fenêtre, boucle de jeu, buffer de pixels, 60 FPS

[← Étape précédente](03-raycasting-ascii.md) · [Sommaire](README.md) · [Étape suivante →](05-textures-sol-plafond.md)

## 🎯 Objectif

Quitter le terminal pour une vraie fenêtre graphique. Tu vas :

1. ouvrir une fenêtre avec SDL2 et la fermer proprement ;
2. écrire une **boucle de jeu** temps réel (le jeu tourne même si tu n'appuies sur rien) ;
3. dessiner dans **ton propre buffer de pixels** (un tableau de `uint32_t`), puis l'envoyer à l'écran ;
4. y porter ton raycaster de l'étape 3, avec des couleurs ;
5. gérer le temps pour que le jeu tourne à 60 FPS et que la vitesse du joueur ne dépende pas de la machine.

### Résultat attendu

- Une fenêtre de 640×480 (ou plus) titrée « 3DMOTOR ».
- Un plafond gris foncé, un sol gris clair, des murs de couleur unie (une couleur par type de mur),
  plus sombres sur une face que sur l'autre.
- Le joueur se déplace **en continu** tant qu'une touche est enfoncée (flèches ou ZQSD), de façon fluide.
- Le titre de la fenêtre (ou le terminal) affiche les FPS, autour de 60.
- Fermer la fenêtre (croix ou Échap) quitte le programme proprement, et `valgrind` ne signale pas de fuite venant de ton code.

---

## 📚 Notions de C abordées

### Utiliser une bibliothèque externe

Une bibliothèque, c'est : des **headers** (pour compiler) + un **fichier binaire** `.so` / `.a` (pour le link).
Il faut donc dire à gcc où sont les headers (`-I...`) et quelle bibliothèque lier (`-l...`).

SDL2 fournit un outil qui te donne les bonnes options :

```sh
sdl2-config --cflags     # options pour la compilation
sdl2-config --libs       # options pour le link
# ou, de façon équivalente :
pkg-config --cflags --libs sdl2
```

Dans un Makefile, on peut récupérer la sortie d'une commande avec `$(shell ...)`.
Question : dans quelle variable de ton Makefile va chaque partie ?

Le header à inclure : `#include <SDL2/SDL.h>` ou `#include <SDL.h>` selon les options `-I` utilisées.
Regarde ce qu'affiche `sdl2-config --cflags` pour comprendre lequel marche.

### Allocation dynamique : `malloc` et `free`

Un écran de 640×480 pixels de 4 octets, c'est 1,2 Mo. La **pile** (où vivent les variables locales)
fait typiquement 8 Mo sous Linux — et en 1920×1080, ton buffer ferait 8 Mo à lui seul. On alloue donc
ce genre de buffer sur le **tas** (heap) avec `malloc` :

```c
/* Exemple générique */
#include <stdlib.h>

int *notes = malloc(nb_eleves * sizeof(int));
if (notes == NULL)
{
    /* plus de mémoire : on gère l'erreur */
    return 1;
}
notes[0] = 15;
/* ... */
free(notes);
notes = NULL;   /* bonne habitude : évite d'utiliser un pointeur libéré */
```

Règles d'or :
- **Toujours** vérifier le retour de `malloc`.
- **Chaque** `malloc` a exactement **un** `free` correspondant.
- Ne **jamais** utiliser une zone après l'avoir libérée.
- `sizeof(*ptr)` plutôt que `sizeof(type)` : si le type change, pas besoin de corriger.

### Entiers à taille fixe : `<stdint.h>`

`int` n'a pas de taille garantie. Pour un pixel, on veut **exactement 32 bits non signés** : `uint32_t`.

### Couleurs et opérateurs bit à bit

Un pixel au format **ARGB8888** tient dans un `uint32_t` : 8 bits par canal.

```
 bits:  31      24 23      16 15       8 7        0
       ┌──────────┬──────────┬──────────┬──────────┐
       │  Alpha   │  Rouge   │   Vert   │   Bleu   │
       └──────────┴──────────┴──────────┴──────────┘
 hexa :   0xFF        0x33       0x66       0x99      → 0xFF336699
```

Les opérateurs à connaître :

| Opérateur | Nom | Exemple générique |
|-----------|-----|-------------------|
| `<<` | décalage à gauche | `1 << 4` vaut `16` (`0b10000`) |
| `>>` | décalage à droite | `0xF0 >> 4` vaut `0x0F` |
| `\|` | OU bit à bit | `0xF0 \| 0x0F` vaut `0xFF` |
| `&` | ET bit à bit (masque) | `0x1234 & 0x00FF` vaut `0x0034` |

Défi de réflexion : comment **construire** un `uint32_t` à partir de 4 composantes `r, g, b, a` ?
Et comment **extraire** la composante verte d'une couleur ? (Défi 4.4.)

### Une image 2D dans un tableau 1D

Ton buffer de pixels est un tableau **à une dimension** de `largeur × hauteur` cases.
Le pixel `(x, y)` est à l'indice :

```
index = y × largeur + x

        x →  0   1   2   3
   y = 0   [ 0][ 1][ 2][ 3]
   y = 1   [ 4][ 5][ 6][ 7]      ex : (2, 1) → 1 × 4 + 2 = 6
   y = 2   [ 8][ 9][10][11]
```

### Gestion d'erreurs et nettoyage

Avec SDL, beaucoup de fonctions peuvent échouer. Elles renvoient `NULL` ou une valeur négative,
et `SDL_GetError()` donne le message. Prends l'habitude :

- de **tester chaque appel** d'initialisation ;
- de **libérer dans l'ordre inverse** de la création (texture → renderer → fenêtre → `SDL_Quit`) ;
- de centraliser le nettoyage dans une fonction qui accepte des pointeurs `NULL` (pour pouvoir l'appeler même si l'init a échoué à mi-chemin).

---

## 🖥️ Le fonctionnement de SDL2 (ce qu'il faut savoir)

### Les objets

| Objet | Rôle |
|-------|------|
| `SDL_Window` | la fenêtre du système |
| `SDL_Renderer` | ce qui dessine dans la fenêtre (souvent via le GPU) |
| `SDL_Texture` | une image stockée côté GPU, que le renderer sait copier dans la fenêtre |
| ton `uint32_t *` | **ton** buffer en RAM, dans lequel ton raycaster écrit pixel par pixel |

### Le pipeline de chaque image

```
 ton raycaster ──écrit──► uint32_t *pixels (RAM)
                                  │
                       SDL_UpdateTexture
                                  ▼
                           SDL_Texture (GPU)
                                  │
                         SDL_RenderCopy
                                  ▼
                           le renderer
                                  │
                       SDL_RenderPresent
                                  ▼
                              l'écran
```

La texture doit être créée **une seule fois** au démarrage, avec l'accès `SDL_TEXTUREACCESS_STREAMING`
(elle est mise à jour à chaque image) et le format `SDL_PIXELFORMAT_ARGB8888`.

Fonctions à chercher sur le wiki SDL2 : `SDL_Init`, `SDL_CreateWindow`, `SDL_CreateRenderer`,
`SDL_CreateTexture`, `SDL_UpdateTexture`, `SDL_RenderClear`, `SDL_RenderCopy`, `SDL_RenderPresent`,
`SDL_DestroyTexture`, `SDL_DestroyRenderer`, `SDL_DestroyWindow`, `SDL_Quit`, `SDL_GetError`.

> 📝 Le paramètre `pitch` de `SDL_UpdateTexture` est la taille **en octets** d'une ligne de ton buffer.
> Ce n'est pas la largeur en pixels !

### Les événements et le clavier

Deux mécanismes complémentaires :

- **`SDL_PollEvent`** dans une boucle `while` : traite les événements ponctuels (fermeture de la fenêtre
  `SDL_QUIT`, appui sur une touche `SDL_KEYDOWN`…). Il faut **vider toute la file** à chaque image.
- **`SDL_GetKeyboardState`** : renvoie un tableau qui dit, à l'instant présent, quelles touches sont **enfoncées**.
  Idéal pour les déplacements continus.

Question : pour « Échap = quitter », lequel est le plus adapté ? Et pour « avancer tant que Z est enfoncé » ?

> ⚠️ Clavier AZERTY : `SDL_SCANCODE_W` désigne la touche **physique** en haut à gauche d'un clavier QWERTY,
> qui est le **Z** de ton clavier AZERTY. Les *scancodes* sont physiques, les *keycodes* (`SDLK_z`) dépendent
> de la disposition. Pour les déplacements, les scancodes sont en général ce qu'on veut.

### La boucle de jeu

```c
/* Squelette */
while (running)
{
    /* 1. TODO : mesurer le temps écoulé depuis l'image précédente (dt) */
    /* 2. TODO : traiter les événements (SDL_PollEvent) */
    /* 3. TODO : mettre à jour le jeu (déplacements × dt) */
    /* 4. TODO : dessiner dans le buffer de pixels (raycasting) */
    /* 5. TODO : envoyer le buffer à l'écran */
    /* 6. TODO : limiter à ~60 FPS si besoin */
}
```

### Le temps : 60 FPS et delta time

60 FPS, c'est une image toutes les `1000 / 60 ≈ 16,67 ms`.

**Problème** : si tu avances de `0.05` case par image, le joueur ira deux fois plus vite sur un écran à 120 Hz
qu'à 60 Hz, et ralentira quand le rendu rame. **Solution** : exprimer les vitesses **par seconde**
et les multiplier par le temps écoulé depuis l'image précédente, `dt` :

```
déplacement_cette_image = vitesse (cases/s) × dt (s)
```

Outils SDL : `SDL_GetTicks64()` (millisecondes), ou `SDL_GetPerformanceCounter()` / `SDL_GetPerformanceFrequency()`
(plus précis). Pour limiter le framerate : soit `SDL_Delay` du temps restant, soit le drapeau
`SDL_RENDERER_PRESENTVSYNC` à la création du renderer (l'image attend le rafraîchissement de l'écran).

Pour lire la doc en profondeur : l'article « Fix Your Timestep! » (voir les ressources du README).

---

## 🧩 Défis

### Défi 4.1 — Compiler avec SDL2

1. Installe `libsdl2-dev` si ce n'est pas fait.
2. Modifie ton Makefile pour ajouter les options de SDL2 à la compilation et au link.
3. Écris un `main` minimal qui appelle `SDL_Init(SDL_INIT_VIDEO)` puis `SDL_Quit()`, et vérifie que ça compile et se lance.

<details>
<summary>Indice 1</summary>

`$(shell sdl2-config --cflags)` dans `CFLAGS`, `$(shell sdl2-config --libs)` dans une variable utilisée **au link**
(souvent `LDLIBS` ou `LDFLAGS`), avec `-lm`.

</details>

<details>
<summary>Indice 2</summary>

Erreur `undefined reference to WinMain` ou `SDL_main` ? Ta fonction doit s'appeler exactement `main` et avoir la
signature `int main(int argc, char **argv)` : SDL peut redéfinir `main` sur certaines plateformes. Sous Linux, ça passe
en général, mais prends la bonne habitude.

</details>

### Défi 4.2 — Une fenêtre qui se ferme proprement

Ouvre une fenêtre, garde-la ouverte jusqu'à ce qu'on clique sur la croix ou qu'on appuie sur Échap, puis libère tout.

<details>
<summary>Indice 1</summary>

Sans boucle d'événements, la fenêtre ne s'affiche parfois même pas, ou le système la déclare « ne répond pas ».
Il faut appeler `SDL_PollEvent` régulièrement.

</details>

<details>
<summary>Indice 2</summary>

Regroupe fenêtre, renderer, texture et buffer dans une struct, avec une fonction `init` et une fonction `destroy`.
`destroy` doit pouvoir être appelée même si `init` a échoué au milieu : initialise tous les pointeurs à `NULL` d'abord.

</details>

### Défi 4.3 — Ton buffer de pixels

1. Alloue un buffer de `largeur × hauteur` `uint32_t`.
2. Crée une texture streaming de la même taille.
3. À chaque image, remplis le buffer d'une couleur unie, envoie-le à l'écran.
4. Écris `put_pixel(buffer, x, y, couleur)` (avec vérification des bornes) et dessine un dégradé : par exemple
   le rouge augmente avec `x` et le bleu avec `y`.

```c
void put_pixel(/* TODO : quoi d'autre que x, y, color ? */, int x, int y, uint32_t color);
```

<details>
<summary>Indice 1</summary>

Pour le dégradé, tu dois ramener `x ∈ [0, largeur[` à une composante `∈ [0, 255]`.

</details>

<details>
<summary>Indice 2</summary>

Si les couleurs sont fausses (rouge et bleu inversés), c'est que le format de ta texture ne correspond pas à la façon
dont tu fabriques tes couleurs. Vérifie le `SDL_PIXELFORMAT_...` choisi.

</details>

### Défi 4.4 — Manipuler les couleurs

Écris :
- une fonction qui fabrique un `uint32_t` à partir de `r, g, b` (alpha à 255) ;
- une fonction qui **assombrit** une couleur d'un facteur (par exemple 0.5).

<details>
<summary>Indice 1</summary>

Fabriquer : décaler chaque composante à sa place puis combiner avec `|`.
Extraire : décaler vers la droite puis masquer avec `& 0xFF`.

</details>

<details>
<summary>Indice 2</summary>

Pour assombrir de moitié il existe une astuce célèbre avec un seul décalage et un seul masque appliqués à la couleur
entière. Réfléchis à ce qui se passe si tu décales tout de 1 bit vers la droite : quel bit « déborde » d'un canal
vers le canal voisin ? Comment l'effacer ? (La méthode générale « extraire, multiplier, recombiner » marche aussi très bien.)

</details>

### Défi 4.5 — Porter le raycaster

1. Réutilise **sans le modifier** (si ton découpage était bon !) ton code de DDA de l'étape 3.
2. Remplace le dessin de caractères par le dessin de colonnes de pixels.
3. Une couleur par type de mur (`1` = rouge, `2` = vert…), assombrie si `side == 1`.

<details>
<summary>Indice 1</summary>

Si ton DDA dépend de `printf`, de constantes d'écran ASCII ou du buffer de `char`, c'est le moment de le découpler :
le DDA ne devrait connaître que la carte, la position et la direction du rayon. C'est le grand intérêt d'une fonction
`cast_ray` qui **renvoie** un résultat au lieu de dessiner.

</details>

### Défi 4.6 — Déplacements temps réel avec delta time

1. Calcule `dt` (en secondes, `double`) à chaque tour de boucle.
2. Utilise `SDL_GetKeyboardState` pour avancer/reculer/tourner tant que les touches sont enfoncées.
3. Exprime la vitesse de déplacement en cases/s et la vitesse de rotation en radians/s.
4. *(Bonus)* Ajoute le déplacement latéral (*strafe*) : quel vecteur faut-il utiliser ?

<details>
<summary>Indice 1</summary>

Garde l'instant de l'image précédente dans une variable déclarée **avant** la boucle. À chaque tour : `maintenant − avant`,
puis `avant = maintenant`. Attention à faire la division en `double`.

</details>

<details>
<summary>Indice 2</summary>

Pour le strafe : tu as déjà un vecteur perpendiculaire à `dir` sous la main…

</details>

### Défi 4.7 — Limiter et afficher les FPS

1. Fais tourner le jeu à environ 60 FPS (vsync **ou** `SDL_Delay`, essaie les deux).
2. Compte les images rendues pendant une seconde et affiche le résultat dans le titre de la fenêtre (`SDL_SetWindowTitle`)
   une fois par seconde.

<details>
<summary>Indice 1</summary>

Pour écrire un nombre dans une chaîne : `snprintf` (jamais `sprintf`, qui ne vérifie pas la taille du buffer).

</details>

<details>
<summary>Indice 2</summary>

`SDL_Delay` n'est pas précis à la milliseconde près : il peut dormir un peu plus. C'est normal d'obtenir 58–62 FPS.
Calcule le temps à attendre en fonction de ce que l'image a **déjà** consommé.

</details>

---

## 🐞 Erreurs classiques et débogage

| Symptôme | Cause probable | Piste |
|----------|----------------|-------|
| `fatal error: SDL.h: No such file or directory` | Options de compilation SDL manquantes | `sdl2-config --cflags` dans `CFLAGS` |
| `undefined reference to 'SDL_Init'` | Options de link manquantes | `sdl2-config --libs` au link, **après** les `.o` |
| Fenêtre noire | Texture non mise à jour, oubli de `SDL_RenderPresent` | Vérifie le pipeline dans l'ordre |
| Image en diagonale / cisaillée | Mauvais `pitch` | Le pitch est en **octets** |
| Couleurs inversées | Format de pixel | `SDL_PIXELFORMAT_ARGB8888` vs ordre de tes composantes |
| Segfault en dessinant | `put_pixel` hors limites, ou `y × largeur + x` avec `x`/`y` inversés | Bornes ; `-fsanitize=address` |
| Le joueur va trop vite / dépend de la machine | Pas de delta time | Défi 4.6 |
| Le joueur ne bouge pas avec `SDL_GetKeyboardState` | Plus d'appel à `SDL_PollEvent` (le tableau n'est mis à jour que quand les événements sont pompés) | Garde la boucle d'événements |
| FPS très bas | Compilé en `-O0`, ou `malloc`/`SDL_CreateTexture` **dans** la boucle | Allouer une fois au début ; essaie `-O2` |

### Valgrind et SDL

```sh
valgrind --leak-check=full --show-leak-kinds=definite ./3dmotor
```

Les pilotes graphiques et SDL eux-mêmes provoquent beaucoup de messages « still reachable » ou même « possibly lost »
qui **ne viennent pas de toi**. Concentre-toi sur :
- les **definitely lost** dont la pile d'appels contient **tes** fonctions ;
- les **Invalid read / Invalid write** (accès hors d'un bloc alloué) : ceux-là sont toujours graves.

Alternative souvent plus lisible : compile avec `-fsanitize=address` (et lance normalement). En cas de bruit des pilotes,
la variable d'environnement `ASAN_OPTIONS=detect_leaks=0` désactive la détection de fuites pour se concentrer sur les accès invalides.

### Profiler simplement

Si c'est lent, mesure avant d'optimiser : chronomètre séparément la partie raycasting et la partie envoi à l'écran
(avec `SDL_GetPerformanceCounter`). Tu sauras où chercher.

---

## ✅ Checklist de validation

- [ ] Mon Makefile utilise `sdl2-config` (ou `pkg-config`) et compile sans avertissement.
- [ ] Toutes les créations SDL sont vérifiées, et les messages d'erreur utilisent `SDL_GetError()`.
- [ ] Le buffer de pixels est alloué **une seule fois**, libéré à la fin, avec vérification de `malloc`.
- [ ] Je sais expliquer `y × largeur + x` et le rôle du `pitch`.
- [ ] Je sais construire une couleur ARGB et en extraire une composante avec des opérateurs bit à bit.
- [ ] Le raycaster s'affiche en couleurs, avec ombrage selon la face.
- [ ] Les déplacements sont continus et utilisent le delta time.
- [ ] Le jeu tourne à ~60 FPS et affiche son framerate.
- [ ] La fermeture (croix ou Échap) libère tout ; valgrind ou ASan ne signalent rien venant de mon code.
- [ ] J'ai fait un commit.

[← Étape précédente](03-raycasting-ascii.md) · [Sommaire](README.md) · [Étape suivante →](05-textures-sol-plafond.md)
