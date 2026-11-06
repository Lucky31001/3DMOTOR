# 3DMOTOR — Guide pour construire un moteur de raycasting en C

Ce guide va t'accompagner étape par étape dans l'écriture d'un moteur de rendu « pseudo-3D »
façon *Wolfenstein 3D*, en C, **en codant toi-même**.

Ce guide ne contient **pas** la solution. Il t'apporte :

- des **objectifs** clairs et le résultat que tu dois voir à l'écran,
- les **notions de C** et de **maths** dont tu auras besoin, expliquées simplement,
- des **défis** à relever, du plus simple au plus complet,
- des **indices progressifs** cachés dans des sections repliables (ouvre-les seulement quand tu bloques vraiment),
- les **erreurs classiques** et comment les traquer,
- une **checklist** pour savoir si tu peux passer à la suite.

> 💡 **Comment utiliser les indices** : cherche au moins 15–20 minutes avant d'ouvrir l'indice 1.
> Après l'indice 1, cherche encore avant d'ouvrir l'indice 2. Tu apprendras beaucoup plus
> en te trompant qu'en lisant la réponse.

---

## Sommaire

| Étape | Fichier | Ce que tu vas construire | Notions de C principales |
|------:|---------|--------------------------|--------------------------|
| 1 | [01-mise-en-place.md](01-mise-en-place.md) | Environnement, Makefile, premier programme | compilation, `main`, headers, `printf` |
| 2 | [02-carte-ascii-et-joueur.md](02-carte-ascii-et-joueur.md) | Une carte 2D affichée dans le terminal, un joueur qui bouge | tableaux 2D, `struct`, pointeurs, `#define`, `enum` |
| 3 | [03-raycasting-ascii.md](03-raycasting-ascii.md) | Une vue « 3D » en caractères ASCII | `math.h`, `double` vs `int`, casts, buffers de `char` |
| 4 | [04-sdl2-fenetre-et-boucle.md](04-sdl2-fenetre-et-boucle.md) | Une vraie fenêtre, 60 FPS, buffer de pixels | bibliothèque externe, `malloc`/`free`, `uint32_t`, opérateurs bit à bit |
| 5 | [05-textures-sol-plafond.md](05-textures-sol-plafond.md) | Murs texturés, sol et plafond | tableaux de pixels, chargement de fichiers, `pitch` |
| 6 | [06-bonus.md](06-bonus.md) | Sprites, minimap, portes, niveaux depuis un fichier | `qsort`, pointeurs de fonctions, `fopen`/`fgets`, allocation dynamique 2D |

Chaque étape **réutilise** le travail de la précédente : ne saute pas d'étape, et fais un commit
git à chaque checklist validée.

---

## Prérequis

### Ce que tu dois savoir faire (ou être prêt à apprendre)

- Utiliser un terminal : `cd`, `ls`, `mkdir`, lancer un programme avec `./nom`.
- Utiliser un éditeur de texte (VS Code, Vim, Neovim, Emacs… peu importe).
- Les bases de git : `git add`, `git commit`, `git log`.
- Des maths de collège/lycée : théorème de Pythagore, sinus/cosinus. Le reste sera expliqué.

### Installation des outils

Tu es sous **Ubuntu 24.04** :

```sh
sudo apt update
sudo apt install build-essential gdb valgrind libsdl2-dev
```

- `build-essential` : `gcc`, `make` et la bibliothèque standard C.
- `gdb` : le débogueur.
- `valgrind` : détecte les fuites mémoire et les accès mémoire invalides.
- `libsdl2-dev` : la bibliothèque graphique SDL2 **et** ses en-têtes (`.h`) — tu n'en auras besoin qu'à l'étape 4.
- *(Optionnel, étape 5)* `libsdl2-image-dev` si tu veux charger des PNG. Sinon, SDL2 sait déjà lire les `.bmp`.

Vérifie l'installation :

```sh
gcc --version
make --version
gdb --version
valgrind --version
sdl2-config --version      # ou : pkg-config --modversion sdl2
```

<details>
<summary>Autres systèmes (pour référence)</summary>

- **Fedora** : `sudo dnf install gcc make gdb valgrind SDL2-devel`
- **Arch** : `sudo pacman -S base-devel gdb valgrind sdl2`
- **macOS** : `xcode-select --install` puis `brew install sdl2` (valgrind ne marche pas bien sur macOS récent : utilise `-fsanitize=address` à la place, et `lldb` au lieu de `gdb`).
- **Windows** : le plus simple est **WSL2** avec Ubuntu, puis les commandes Ubuntu ci-dessus.

</details>

### Structure de projet visée

Tu la mettras en place à l'étape 1. Elle évoluera, mais voici l'idée générale :

```
3DMOTOR/
├── Makefile
├── docs/          ← ce guide
├── include/       ← tes fichiers .h
├── src/           ← tes fichiers .c
├── assets/        ← textures, niveaux (à partir de l'étape 5)
└── build/         ← fichiers .o générés (à ignorer dans git)
```

---

## Ressources de référence

### Raycasting

- **Lode Vandevenne — Raycasting** (la référence absolue, en anglais) :
  - Partie 1, murs non texturés et texturés : <https://lodev.org/cgtutor/raycasting.html>
  - Partie 2, sol et plafond : <https://lodev.org/cgtutor/raycasting2.html>
  - Partie 3, sprites : <https://lodev.org/cgtutor/raycasting3.html>
  - Partie 4, portes, murs fins, etc. : <https://lodev.org/cgtutor/raycasting4.html>

  ⚠️ Ce tutoriel contient du code complet (en C++). **Lis les explications et les schémas,
  pas le code** — sinon tu vas recopier au lieu d'apprendre. Une bonne méthode : lis une
  section, ferme l'onglet, et code de mémoire.
- **F. Permadi — Ray-Casting Tutorial** (approche par angles, très visuelle) :
  <https://permadi.com/1996/05/ray-casting-tutorial-table-of-contents/>
- **Fabien Sanglard — *Game Engine Black Book: Wolfenstein 3D*** : l'histoire et l'architecture du vrai moteur. Gratuit en PDF sur <https://fabiensanglard.net/gebbwolf3d/>.
- **Red Blob Games** — très bons articles interactifs sur les grilles et la géométrie : <https://www.redblobgames.com/>

### Langage C

- **Beej's Guide to C Programming** (gratuit, clair, en anglais) : <https://beej.us/guide/bgc/>
- **Jens Gustedt — *Modern C*** (gratuit, plus exigeant) : <https://gustedt.gitlabpages.inria.fr/modern-c/>
- **cppreference, partie C** (la doc de référence de la bibliothèque standard) : <https://en.cppreference.com/w/c>
- **Les pages `man`**, déjà sur ta machine : `man 3 printf`, `man 3 malloc`, `man 3 fopen`…
  (installe `manpages-dev` si elles manquent).

### SDL2

- **Wiki officiel SDL2** (la doc de chaque fonction) : <https://wiki.libsdl.org/SDL2/>
- **Lazy Foo' — tutoriels SDL2** (lis les concepts, écris ton propre code) : <https://lazyfoo.net/tutorials/SDL/>

### Boucle de jeu et temps

- **Game Programming Patterns — Game Loop** : <https://gameprogrammingpatterns.com/game-loop.html>
- **Glenn Fiedler — Fix Your Timestep!** : <https://gafferongames.com/post/fix_your_timestep/>

### Outils

- **Make** — manuel GNU : <https://www.gnu.org/software/make/manual/make.html>
- **GDB** — tutoriel rapide : <https://www.sourceware.org/gdb/documentation/>
- **Valgrind** — Quick Start : <https://valgrind.org/docs/manual/quick-start.html>

---

## Conseils généraux

1. **Compile souvent.** Après chaque petite modification. Une erreur à la fois, c'est facile ; trente, c'est l'enfer.
2. **Lis les messages d'erreur en entier, en commençant par le premier.** Les suivants sont souvent des conséquences du premier.
3. **Affiche des valeurs** (`printf`) ou utilise `gdb` quand ça ne fait pas ce que tu attends. Ne devine pas : vérifie.
4. **Dessine sur papier.** Le raycasting est de la géométrie : un schéma vaut mille `printf`.
5. **Un commit par sous-étape qui marche.** Tu pourras toujours revenir en arrière.
6. **Pose-moi des questions** : explique-moi ce que tu as essayé, ce que tu attendais et ce que tu as obtenu. Je te guiderai sans te donner la réponse.
