# Étape 3 — Raycasting en ASCII

[← Étape précédente](02-carte-ascii-et-joueur.md) · [Sommaire](README.md) · [Étape suivante →](04-sdl2-fenetre-et-boucle.md)

## 🎯 Objectif

Remplacer la vue de dessus par une **vue à la première personne**, toujours dans le terminal,
en caractères ASCII. Pour chaque colonne de l'écran, tu vas lancer **un rayon** depuis le joueur,
trouver le premier mur qu'il touche grâce à l'**algorithme DDA**, et dessiner une colonne de mur
d'autant plus haute que le mur est proche.

C'est **le cœur du projet**. Prends ton temps : tout ce qui suit (SDL, textures, sprites) repose
sur ce que tu vas comprendre ici.

### Résultat attendu

Un « écran » de, par exemple, 80×24 caractères :

```
                                                                                
                                                                                
@@@@                                                                       %%%%%
@@@@@@                                                                 %%%%%%%%%
@@@@@@@@                    ..........                             %%%%%%%%%%%%%
@@@@@@@@@@              ::::::::::::::::::                     %%%%%%%%%%%%%%%%%
@@@@@@@@@@@@         ======================                %%%%%%%%%%%%%%%%%%%%%
@@@@@@@@@@@@@@    ++++++++++++++++++++++++++++          %%%%%%%%%%%%%%%%%%%%%%%%
@@@@@@@@@@@@@@    ++++++++++++++++++++++++++++          %%%%%%%%%%%%%%%%%%%%%%%%
@@@@@@@@@@@@@@    ++++++++++++++++++++++++++++          %%%%%%%%%%%%%%%%%%%%%%%%
@@@@@@@@@@@@         ======================                %%%%%%%%%%%%%%%%%%%%%
@@@@@@@@@@              ::::::::::::::::::                     %%%%%%%%%%%%%%%%%
...
```

(Ceci est une illustration de l'idée, ta vue ne ressemblera pas exactement à ça.)

- Les murs proches sont **hauts** et dessinés avec des caractères **denses** (`@`, `#`, `%`).
- Les murs lointains sont **petits** et dessinés avec des caractères **légers** (`.`, `:`).
- Quand tu tournes avec `q`/`d`, la vue pivote ; quand tu avances, les murs grandissent.
- Les murs droits restent **droits** (pas d'effet « œil de poisson »).

---

## 📚 Notions de C abordées

### `int` et `double` : conversions et pièges

- Une division entre deux `int` est une **division entière** : `3 / 2` vaut `1`, et `1 / 2` vaut `0`.
  Si l'un des deux est un `double`, la division est réelle : `3 / 2.0` vaut `1.5`.
- `(int)x` **tronque vers zéro** : `(int)2.7` vaut `2`, mais `(int)-0.5` vaut `0` (pas `-1`).
  `floor()` de `<math.h>` arrondit toujours vers le bas.
- Une division d'un `double` par `0.0` ne plante pas : elle donne `inf` (infini). Selon le contexte,
  c'est un bug… ou un comportement exploitable. Une division d'un `int` par `0`, elle, fait planter le programme.

Pose-toi la question « int ou double ? » pour **chaque** variable de ton raycaster.

### Fonctions de `<math.h>` utiles

`fabs` (valeur absolue d'un double — **pas** `abs`, qui est pour les `int` !), `sqrt`, `floor`,
`cos`, `sin`, `tan`. Et toujours `-lm` au link.

### Un buffer d'écran

Le raycasting calcule l'image **colonne par colonne**, mais le terminal affiche **ligne par ligne**.
Tu ne peux donc pas afficher directement pendant le calcul. Solution : dessiner d'abord dans un
**buffer** (un tableau de `char` de la taille de l'écran), puis l'afficher d'un coup.

C'est exactement ce que tu feras à l'étape 4 avec des pixels au lieu de caractères.

Réflexions :
- Si chaque ligne du buffer est une chaîne à passer à `printf("%s")` ou `puts`, de quoi a-t-elle besoin à la fin ?
- Un tableau local de 80×24 `char` tient sans problème sur la pile. Un tableau de 1920×1080 `uint32_t`, non (tu verras pourquoi à l'étape 4).

### Constantes nommées

Tu vas avoir beaucoup de « nombres magiques » : largeur, hauteur de l'écran, champ de vision, palette
de caractères… Mets-les dans des `#define` ou des `const` en haut d'un header. Ton toi du futur te remerciera.

Une palette de caractères peut tout simplement être une chaîne : `const char *shades = " .:-=+*#%@";`
On accède au i-ème caractère avec `shades[i]`.

---

## 📐 Les maths nécessaires

Lis cette partie **avec une feuille et un crayon**. Refais les schémas.

### 1. Le plan de caméra

On a déjà le vecteur `dir` (où regarde le joueur). On ajoute un deuxième vecteur, `plane`,
**perpendiculaire** à `dir`. Il représente l'écran, vu de dessus :

```
                 plane (gauche)           plane (droite)
        rayon col. 0   ◄───────────●───────────►   rayon dernière col.
                   ╲               │               ╱
                    ╲              │ dir          ╱
                     ╲             │             ╱
                      ╲            │            ╱
                       ╲           │           ╱
                        ╲          │          ╱
                         ╲         │         ╱
                          ╲        │        ╱
                           ╲       │       ╱
                                   ● joueur (pos)
```

L'extrémité de `dir` est le centre de l'écran. L'écran va de `dir − plane` (bord gauche) à `dir + plane` (bord droit).

Pour chaque colonne `x` de l'écran (de `0` à `largeur − 1`), on calcule une coordonnée `cameraX`
qui va de **−1** (colonne de gauche) à **+1** (colonne de droite), en passant par 0 au centre.
La direction du rayon est alors :

```
rayDir = dir + plane × cameraX
```

**Le champ de vision (FOV)** dépend du rapport des longueurs de `plane` et de `dir` :

```
                    |plane|
         ┌────────────────────┐
         │                   ╱
   |dir| │                 ╱        tan(FOV / 2) = |plane| / |dir|
   = 1   │               ╱
         │  FOV/2      ╱
         │   ╲       ╱
         │    ╲    ╱
         │     ╲ ╱
         ●───────
```

Avec `|dir| = 1` et `|plane| = 0.66`, on obtient un FOV d'environ 66°, un bon point de départ.

Questions à te poser (pas de réponse ici, c'est le défi 3.1) :
- Quelle formule donne `cameraX` à partir de `x` et de la largeur ? (Vérifie-la pour `x = 0`, `x = largeur/2`, `x = largeur − 1`.)
- Comment obtenir un vecteur perpendiculaire à `dir` ? Dans quel sens (rappelle-toi que y pointe vers le bas) ?
- Que doit-il arriver à `plane` quand le joueur tourne ?

### 2. Le rayon comme une droite paramétrée

Un point du rayon s'écrit :

```
P(t) = pos + t × rayDir        avec t ≥ 0
```

`t` est le « temps » de parcours le long du rayon. On va tout mesurer en `t`.

### 3. L'algorithme DDA (Digital Differential Analyzer)

**Idée naïve** : avancer le long du rayon par petits pas (0.01) et tester la case à chaque pas.
C'est lent **et** imprécis (on peut rater un coin de mur).

**Idée du DDA** : un rayon ne peut changer de case qu'en **traversant une ligne de la grille**.
On saute donc directement d'une intersection avec la grille à la suivante, en choisissant à chaque
fois la plus proche : une ligne verticale (x entier) ou une ligne horizontale (y entier).

```
       │       │       │       │
  ─────┼───────┼───────┼───────┼─────  y = 1
       │       │      ╱│       │
       │       │    ╱  │       │
       │       │  ●B   │       │        ● A, C : croisements de lignes VERTICALES   (x entier)
  ─────┼───────┼╱──────┼───────┼─────   ● B    : croisement de ligne HORIZONTALE   (y entier)
       │      ╱│       │       │
       │   ●A  │       │       │
       │ ╱     │       │       │
       ●pos    │       │       │
  ─────┼───────┼───────┼───────┼─────
     x = 0   x = 1   x = 2   x = 3
```

Deux observations clés :

**(a) La distance entre deux lignes verticales consécutives est constante.**
Pour passer de `x = k` à `x = k + 1`, la coordonnée x du rayon doit augmenter de 1. Comme
`x(t) = posX + t × rayDirX`, il faut que `t` augmente de :

```
deltaDistX = | 1 / rayDirX |
deltaDistY = | 1 / rayDirY |     (même raisonnement pour les lignes horizontales)
```

**(b) Seule la première distance est différente.** Le joueur est quelque part *à l'intérieur* de sa case.
La distance `t` jusqu'à la **première** ligne verticale dépend de sa position dans la case et du sens du rayon :

```
   case mapX                   posX - mapX
   ┌───────────────┐          ◄────►
   │      ●────────┼──►       rayon vers la droite :
   │     pos       │          il faut parcourir (mapX + 1 − posX) en x
   └───────────────┘
   mapX         mapX+1
```

On appelle `sideDistX` (resp. `sideDistY`) la valeur de `t` à laquelle le rayon atteint la
**prochaine** ligne verticale (resp. horizontale). Il te faut aussi `stepX` / `stepY` : `+1` ou `−1`
selon le sens du rayon, pour savoir dans quelle case on entre.

**Le principe de la boucle** : tant qu'on n'a pas touché de mur,
on compare `sideDistX` et `sideDistY`, on franchit la ligne la plus proche
(on change de case en x **ou** en y), on met à jour la `sideDist` correspondante, et on note
**quel type de ligne** on vient de franchir (verticale ou horizontale — appelons ça `side`).
Puis on regarde si la nouvelle case est un mur.

### 4. La distance au mur… et l'effet fish-eye

Si tu utilises la **distance euclidienne** (la vraie longueur entre le joueur et le point d'impact),
un mur droit en face de toi apparaîtra **bombé** : les rayons sur les bords sont plus longs que celui
du centre, donc les bords du mur semblent plus loin. C'est l'effet *fish-eye*.

```
         mur
  ───────────────────────
   ╲      │      ╱
    ╲     │     ╱    rayons des bords : plus longs
     ╲    │    ╱     → murs plus petits sur les côtés
      ╲   │   ╱      → effet bombé ❌
       ╲  │  ╱
        ╲ │ ╱
          ●
```

Ce qu'il faut, c'est la distance **perpendiculaire au plan de caméra** (projetée sur `dir`), notée `perpWallDist`.

Et c'est là que mesurer en `t` est magique : comme `rayDir = dir + plane × cameraX`, que `dir`
est de longueur 1 et que `plane` est perpendiculaire à `dir`, la composante de `t × rayDir` le long
de `dir` vaut… exactement `t`. Autrement dit : **la valeur de `t` au point d'impact *est* la distance perpendiculaire**.

Question : au moment où la boucle DDA trouve un mur, tu as déjà fait un pas de trop sur la `sideDist`
concernée. Comment retrouver le `t` de l'impact ?

### 5. De la distance à la hauteur à l'écran

Un mur à distance 1 doit remplir toute la hauteur de l'écran ; à distance 2, la moitié ; etc. :

```
hauteur_ligne = hauteur_écran / perpWallDist
```

La colonne est centrée verticalement :

```
         ┌──────────────┐ 0
         │   plafond    │
 début ──┤··············│ = centre − hauteur_ligne / 2
         │     MUR      │
   fin ──┤··············│ = centre + hauteur_ligne / 2
         │     sol      │
         └──────────────┘ hauteur_écran − 1
```

Si le mur est très proche, `début` peut être négatif et `fin` dépasser l'écran : **il faut borner** (clamp).

> 📝 Les caractères d'un terminal sont environ **deux fois plus hauts que larges**. Tes murs risquent de
> paraître étirés : un facteur correctif sur la hauteur arrangera ça. Expérimente.

### 6. L'ombrage

- **Selon la distance** : choisis un caractère de ta palette en fonction de `perpWallDist`.
  Il faut transformer une distance (de 0 à « loin ») en un indice (de `len − 1` à `0`) sans sortir de la chaîne.
- **Selon la face** : les murs touchés sur une ligne verticale (`side == 0`) et ceux touchés sur une ligne
  horizontale (`side == 1`) peuvent avoir des palettes différentes. Ça donne immédiatement du relief aux coins.
- **Sol et plafond** : un caractère fixe (espace pour le plafond, `.` ou `-` pour le sol), ou mieux, un dégradé selon la ligne.

---

## 🧩 Défis

### Défi 3.1 — La caméra

1. Ajoute le vecteur `plane` à ton joueur et initialise-le correctement par rapport à `dir`.
2. Fais-le tourner en même temps que `dir`.
3. Écris la fonction qui, pour une colonne `x`, calcule `rayDir`.
4. **Test sans graphisme** : pour un joueur qui regarde vers l'est, affiche avec `printf` les `rayDir` des
   colonnes 0, milieu et dernière. Vérifie à la main que c'est cohérent avec le schéma (le rayon de gauche
   doit pointer « vers l'avant et vers la gauche » du joueur).

<details>
<summary>Indice 1</summary>

Pour `cameraX` : tu veux une fonction affine de `x` qui vaut −1 en 0 et (presque) +1 en `largeur`.
Commence par ramener `x` entre 0 et 1, puis transforme cet intervalle en −1..1.
Attention à la division entière !

</details>

<details>
<summary>Indice 2</summary>

Le vecteur `(a, b)` tourné de 90° donne `(−b, a)` ou `(b, −a)` selon le sens. Teste les deux et regarde
si la gauche de l'écran correspond bien à la gauche du joueur (sinon, l'image sera en miroir : en tournant
à gauche, la vue partira à droite).

</details>

<details>
<summary>Indice 3</summary>

`plane` subit **exactement la même rotation** que `dir`. Factorise : une fonction qui tourne un vecteur
quelconque, appelée deux fois.

</details>

### Défi 3.2 — Le DDA, un seul rayon

Avant de faire tout l'écran, fais **un seul rayon** : celui du centre. Affiche dans le terminal :
la case du mur touché, `side`, et la distance perpendiculaire. Puis compare avec ta carte vue de dessus
(garde l'affichage de l'étape 2 sous la main, ça vaut de l'or pour déboguer).

```c
/* Une suggestion de structure pour le résultat d'un rayon */
typedef struct s_hit {
    int    map_x;      /* case du mur touché */
    int    map_y;
    int    side;       /* 0 : ligne verticale franchie, 1 : ligne horizontale */
    double dist;       /* distance perpendiculaire */
} t_hit;

/* TODO : quels paramètres ? */
t_hit cast_ray(/* ... */);
```

<details>
<summary>Indice 1</summary>

Découpe en trois phases :
1. initialisation (case de départ, `deltaDist`, `step`, `sideDist` initiales) ;
2. la boucle qui avance de ligne en ligne jusqu'à un mur ;
3. le calcul de la distance perpendiculaire.

Fais les phases une par une, et affiche les valeurs après chacune.

</details>

<details>
<summary>Indice 2</summary>

Pour `sideDistX` initiale : relis le schéma du (b). Si le rayon va vers la droite, la distance **en x**
jusqu'à la ligne suivante est `mapX + 1 − posX`. Combien vaut-elle s'il va vers la gauche ?
Ensuite, il faut la convertir en `t` : par quoi multiplier une distance en x pour obtenir un `t` ?
(Regarde la définition de `deltaDistX`.)

</details>

<details>
<summary>Indice 3</summary>

Dans la boucle, après avoir franchi une ligne verticale, la **prochaine** ligne verticale est à `deltaDistX`
plus loin. Et à la sortie de la boucle, la `sideDist` du côté touché a déjà été augmentée de son `delta`
une fois de trop.

</details>

<details>
<summary>Indice 4 (cas limite)</summary>

Que vaut `deltaDistX` quand `rayDirX == 0` (rayon parfaitement vertical) ? Avec des `double`, `1 / 0.0` donne
`inf`… et un `sideDistX` infini ne sera jamais le plus petit, ce qui est exactement le comportement voulu.
Mais certains préfèrent remplacer par une très grande valeur (`1e30`) pour éviter de manipuler `inf`.
Dans les deux cas : **pose-toi la question**, et teste un joueur qui regarde pile vers le nord.

</details>

### Défi 3.3 — Le buffer d'écran

1. Déclare un buffer de caractères pour un écran de `SCREEN_W × SCREEN_H`.
2. Écris une fonction qui le remplit de plafond (moitié haute) et de sol (moitié basse).
3. Écris une fonction qui l'affiche.
4. Écris une fonction qui dessine une colonne verticale de caractères entre deux lignes.

```c
void screen_clear(/* TODO */);
void screen_draw_column(/* TODO */, int x, int y_start, int y_end, char c);
void screen_present(/* TODO */);
```

<details>
<summary>Indice 1</summary>

Si tu stockes `SCREEN_H` lignes de `SCREEN_W + 1` caractères, tu as la place pour un `'\0'` à la fin de chaque
ligne. Sinon, tu peux aussi afficher avec `fwrite` (voir `man 3 fwrite`) qui n'a pas besoin de `'\0'`.

</details>

<details>
<summary>Indice 2</summary>

`screen_draw_column` doit être **défensive** : si `y_start < 0` ou `y_end >= SCREEN_H`, elle borne.
Comme ça, le reste du code n'a pas à s'en soucier.

</details>

### Défi 3.4 — Tout l'écran

Assemble : pour chaque colonne, lance un rayon, calcule la hauteur de la ligne, dessine-la.
Utilise un seul caractère (`#`) pour commencer.

<details>
<summary>Indice 1</summary>

Si tu ne vois **rien** ou un écran plein : affiche `dist` et la hauteur calculée pour quelques colonnes.
Une hauteur de 0 partout ? Division entière. Une hauteur gigantesque ? Distance quasi nulle : le joueur
est-il dans un mur ?

</details>

<details>
<summary>Indice 2</summary>

Si l'image est en **miroir** (tu tournes à gauche, la vue part à droite), c'est le signe de `plane`
(ou le sens de `cameraX`) qui est inversé.

</details>

### Défi 3.5 — Ombrage et finitions

1. Choisis le caractère selon la distance.
2. Distingue `side == 0` et `side == 1`.
3. Corrige le rapport hauteur/largeur des caractères.
4. Garde la boucle de jeu de l'étape 2 : la vue doit se mettre à jour à chaque touche.
5. *(Optionnel)* Affiche la minimap de l'étape 2 **à côté** ou **en dessous** de la vue 3D.

<details>
<summary>Indice 1</summary>

Si ta palette a `n` caractères et que tu considères qu'au-delà d'une distance `d_max` tout est « très loin »,
cherche une formule qui transforme `dist ∈ [0, d_max]` en indice `∈ [n − 1, 0]`, puis borne le résultat.

</details>

### Défi 3.6 — Le test de l'œil de poisson

Place le joueur face à un long mur droit et compare :
- l'affichage avec la distance perpendiculaire (le mur doit être un rectangle) ;
- l'affichage avec la distance euclidienne (calcule-la temporairement avec `sqrt`).

Explique avec tes mots, à voix haute ou dans un commentaire, pourquoi la deuxième version est bombée.

---

## 🐞 Erreurs classiques et débogage

| Symptôme | Cause probable | Piste |
|----------|----------------|-------|
| Écran entièrement vide ou entièrement plein | Hauteur de ligne fausse | `printf` de `dist` et de la hauteur pour 3 colonnes |
| Murs bombés | Distance euclidienne | Distance perpendiculaire (section 4) |
| Image en miroir | Signe de `plane` ou de `cameraX` | Indice 2 du défi 3.4 |
| Boucle infinie / programme figé | Le rayon ne rencontre jamais de mur, ou `step` vaut 0 | Carte bien fermée ? `step` toujours ±1 ? Ajoute un garde-fou (nombre max d'itérations) |
| Crash quand on regarde pile au nord/est | Division par zéro mal gérée | Indice 4 du défi 3.2 |
| Murs « troués » ou qui scintillent aux coins | `side` mal mis à jour, ou mauvaise `sideDist` initiale | Teste un rayon unique dans chaque direction (4 quadrants) |
| Toutes les distances valent 0 | `abs()` au lieu de `fabs()` | `-Wall` te l'aurait peut-être signalé : relis les avertissements |
| `Segmentation fault` en dessinant | `y_start`/`y_end` non bornés | `screen_draw_column` défensive |

### Méthode : tester les 4 quadrants

Un rayon peut aller vers `(+x, +y)`, `(+x, −y)`, `(−x, +y)`, `(−x, −y)`. Beaucoup de bugs DDA n'apparaissent
que dans un ou deux quadrants. Écris une petite fonction de test qui lance un rayon dans chaque quadrant depuis
une position connue, et compare avec ce que tu calcules **à la main sur papier**.

### gdb : les points d'arrêt conditionnels

Tu veux inspecter seulement la colonne 40 ?

```
(gdb) break cast_ray if x == 40
(gdb) run
(gdb) display side_dist_x      # affichée automatiquement à chaque pas
(gdb) display side_dist_y
(gdb) next
```

### Valgrind

Même sans `malloc`, lance au moins une fois `valgrind ./3dmotor` : il détecte les lectures de variables
**non initialisées** (« Conditional jump or move depends on uninitialised value »), une cause fréquente de
comportements aléatoires.

---

## ✅ Checklist de validation

- [ ] Je sais expliquer avec un schéma ce que représentent `dir`, `plane` et `cameraX`.
- [ ] Je sais expliquer pourquoi `deltaDistX = |1 / rayDirX|`.
- [ ] Je sais expliquer l'effet fish-eye et pourquoi la distance perpendiculaire le corrige.
- [ ] Mon DDA fonctionne dans les 4 quadrants (testé), et quand le joueur regarde pile dans un axe.
- [ ] Un long mur droit apparaît droit à l'écran.
- [ ] Tourner à gauche fait tourner la vue à gauche.
- [ ] Les murs sont ombrés selon la distance et selon la face.
- [ ] Aucun avertissement, aucune erreur valgrind / sanitizer.
- [ ] Le code est organisé : le raycasting dans son propre fichier, séparé de l'affichage.
- [ ] J'ai fait un commit.

> 🎉 Si tu en es là, tu as compris le principe de Wolfenstein 3D. La suite consiste surtout à le rendre joli.

[← Étape précédente](02-carte-ascii-et-joueur.md) · [Sommaire](README.md) · [Étape suivante →](04-sdl2-fenetre-et-boucle.md)
