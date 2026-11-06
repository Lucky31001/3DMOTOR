# Étape 2 — Carte 2D en ASCII et déplacement du joueur

[← Étape précédente](01-mise-en-place.md) · [Sommaire](README.md) · [Étape suivante →](03-raycasting-ascii.md)

## 🎯 Objectif

Représenter le monde du jeu (une grille de murs et de cases vides), y placer un joueur avec
une **position** et une **direction**, l'afficher vue de dessus dans le terminal, et le déplacer
au clavier en respectant les murs.

C'est la fondation de tout le moteur : le raycasting de l'étape 3 ne fera « que » regarder cette
même carte depuis les yeux du joueur.

### Résultat attendu

```
################
#..............#
#..##......#...#
#..##......#...#
#..........#...#
#......>.......#
#..............#
#....####......#
#..............#
################
pos = (7.50, 5.50)  dir = (1.00, 0.00)
[z] avancer [s] reculer [q] tourner à gauche [d] tourner à droite [x] quitter
>
```

- Le joueur est affiché par une flèche (`^`, `>`, `v`, `<`) selon la direction la plus proche.
- `z`/`s` le font avancer/reculer **dans la direction où il regarde**, pas forcément en ligne droite sur la grille.
- `q`/`d` le font **pivoter** d'un petit angle (pas d'un quart de tour).
- Il ne peut pas traverser un `#`.

---

## 📚 Notions de C abordées

### Tableaux à deux dimensions

Un tableau 2D de taille fixe se déclare ainsi :

```c
/* Exemple générique : un plateau de morpion */
char plateau[3][3] = {
    {'X', '.', 'O'},
    {'.', 'X', '.'},
    {'O', '.', 'X'},
};
```

`plateau[ligne][colonne]` : **le premier indice est la ligne (y), le second la colonne (x)**.
C'est une source de bugs inépuisable dans les moteurs de jeu, où on raisonne souvent en `(x, y)`.
Choisis une convention et tiens-t'y.

En mémoire, un tableau 2D est en fait **un seul bloc contigu**, ligne après ligne :

```
plateau[0][0] plateau[0][1] plateau[0][2] plateau[1][0] plateau[1][1] ...
```

Retiens-le : à l'étape 4, ton buffer de pixels sera un tableau 1D et tu calculeras toi-même
l'indice `y * largeur + x`.

**Deux façons de stocker ta carte** :
- un tableau de chaînes (`const char *map[] = { "#####", "#...#", ... }`) : facile à lire et à écrire ;
- un tableau d'entiers (`int map[H][W]`) avec `0` = vide, `1` = mur, `2` = autre type de mur… : c'est ce qu'utilise le tutoriel de Lode, et c'est plus pratique quand tu auras plusieurs textures.

Les deux sont valables. Réfléchis : laquelle sera la plus simple à charger depuis un fichier à l'étape 6 ?

### Les structures (`struct`)

Une `struct` regroupe plusieurs variables sous un même nom :

```c
/* Exemple générique */
typedef struct s_rectangle {
    double largeur;
    double hauteur;
} t_rectangle;

t_rectangle r = { .largeur = 4.0, .hauteur = 2.5 };
printf("%f\n", r.largeur);
```

`typedef` permet d'écrire `t_rectangle` au lieu de `struct s_rectangle`. Les préfixes `s_`/`t_`
sont une convention, pas une obligation.

Ton joueur sera une struct. Ton « état du jeu » (carte + joueur + ...) aussi, probablement.

### Les pointeurs, et pourquoi tu en as besoin ici

En C, **les arguments sont passés par copie**. Si tu écris une fonction qui reçoit une struct et
modifie ses champs, tu modifies… la copie. L'original ne bouge pas.

Pour modifier une variable depuis une fonction, on lui passe son **adresse** :

```c
/* Exemple générique, le grand classique */
void echanger(int *a, int *b)
{
    int tmp = *a;   /* *a = « la valeur à l'adresse a » */
    *a = *b;
    *b = tmp;
}

int x = 1, y = 2;
echanger(&x, &y);   /* &x = « l'adresse de x » */
```

Avec une struct pointée, on accède aux champs avec `->` :
`p->largeur` est un raccourci pour `(*p).largeur`.

👉 Question à te poser pour chaque fonction : **est-ce qu'elle doit modifier ce qu'on lui donne ?**
- Oui → pointeur (`t_player *p`).
- Non, mais la struct est grosse → pointeur constant (`const t_game *g`) pour éviter une copie et documenter qu'on ne modifie rien.

### `#define`, `const` et `enum`

- `#define MAP_W 16` : constante remplacée par le préprocesseur. Pratique pour les tailles de tableaux.
- `const double speed = 0.2;` : variable non modifiable, typée.
- `enum` : un ensemble de constantes entières nommées. Idéal pour les types de cases :

```c
/* Exemple générique */
enum e_feu { FEU_ROUGE, FEU_ORANGE, FEU_VERT };   /* valent 0, 1, 2 */
```

### Lire le clavier dans le terminal

Par défaut, le terminal est en **mode canonique** : il bufferise ce que tu tapes et ne l'envoie au
programme qu'après **Entrée**. Pour cette étape, c'est **parfaitement acceptable** : tape `z` puis Entrée.

- `getchar()` lit un caractère. Attention : le `\n` de la touche Entrée est aussi un caractère !
- `scanf(" %c", &c)` — note l'espace avant `%c` : il saute les blancs (dont `\n`).

Bonus (défi 2.6) : passer le terminal en **mode brut** pour réagir sans Entrée, avec `tcgetattr`/`tcsetattr`
(`man 3 termios`). On abandonnera de toute façon le terminal à l'étape 4.

### Effacer l'écran

Les terminaux modernes comprennent les **séquences d'échappement ANSI**. `"\033[2J"` efface l'écran et
`"\033[H"` remet le curseur en haut à gauche. Envoie-les avec `printf` avant chaque affichage.

---

## 📐 Les maths nécessaires

### Coordonnées continues vs cases de la grille

Le joueur n'est **pas** confiné au centre des cases. Sa position est un couple de `double` :

```
     x →   0     1     2     3
  y      ┌─────┬─────┬─────┬─────┐
  ↓   0  │  #  │  #  │  #  │  #  │
         ├─────┼─────┼─────┼─────┤
      1  │  #  │     │  ●  │  #  │   ● joueur en (2.3, 1.6)
         ├─────┼─────┼─────┼─────┤        → il est dans la case (2, 1)
      2  │  #  │     │     │  #  │
         └─────┴─────┴─────┴─────┘
```

La case dans laquelle se trouve un point `(x, y)` s'obtient en prenant la **partie entière** de chaque coordonnée.
Le centre de la case `(2, 1)` est en `(2.5, 1.5)`.

⚠️ L'axe **y pointe vers le bas** (comme les lignes de ton tableau et comme les pixels d'un écran).
Ça a une conséquence sur le sens de rotation : garde-le en tête quand « gauche » et « droite » semblent inversés.

### La direction : un vecteur plutôt qu'un angle

On pourrait stocker un angle `a` et calculer `cos(a)`, `sin(a)` à chaque fois. L'approche de Lode
(et celle que ce guide te recommande) est de stocker directement un **vecteur direction** `dir = (dirX, dirY)`,
de **longueur 1** (vecteur unitaire).

```
            dir = (0, -1)   ← regarde vers le « haut » (y diminue)
                 ▲
                 │
  (-1, 0) ◄──────●──────► dir = (1, 0)
                 │
                 ▼
             (0, 1)
```

**Avancer** d'une distance `v` dans la direction regardée :

```
nouvelle_pos = pos + dir × v
c'est-à-dire :  x' = x + dirX · v
                y' = y + dirY · v
```

Comme `dir` est de longueur 1, le joueur avance exactement de `v` cases.

### Tourner : la matrice de rotation

Pour faire tourner un vecteur `(x, y)` d'un angle `θ` :

```
x' = x · cos θ − y · sin θ
y' = x · sin θ + y · cos θ
```

C'est la formule à appliquer à `dir` quand le joueur tourne. `θ` positif ou négatif selon le sens.

⚠️ **Piège** : en calculant `y'`, il te faut l'**ancienne** valeur de `x`. Si tu as déjà écrasé `x`
avec `x'`, le résultat est faux (et le vecteur rétrécit ou grandit petit à petit !).

Les fonctions `cos` et `sin` de `<math.h>` prennent des **radians** : `π rad = 180°`.
Une rotation de 0.1 rad ≈ 5,7°. Et il faut lier avec `-lm`.

### Choisir la flèche à afficher

Tu as un vecteur `(dirX, dirY)` et tu veux savoir s'il pointe plutôt vers le haut, le bas, la gauche
ou la droite. Indice de réflexion : compare les **valeurs absolues** des deux composantes (`fabs`),
puis regarde le **signe** de la plus grande.

---

## 🧩 Défis

### Défi 2.1 — Afficher une carte

1. Choisis une représentation pour la carte (chaînes ou entiers) et écris une carte d'environ 16×10
   entourée de murs, avec quelques obstacles à l'intérieur.
2. Écris une fonction qui l'affiche dans le terminal.

```c
/* Signature indicative */
void map_print(/* TODO : quels paramètres ? */);
```

<details>
<summary>Indice 1</summary>

Deux boucles imbriquées : une sur les lignes, une sur les colonnes. N'oublie pas le retour à la ligne
à la fin de chaque ligne.

</details>

<details>
<summary>Indice 2</summary>

Passer un tableau 2D à une fonction en C est piégeux : la fonction doit connaître la **largeur** pour
calculer les adresses (`void f(int map[][MAP_W])`). Une solution simple : mettre la carte dans une struct,
et passer un pointeur vers cette struct.

</details>

### Défi 2.2 — Le joueur

1. Crée une struct joueur avec au minimum une position et une direction.
2. Initialise le joueur au centre d'une case vide, regardant vers l'est.
3. Modifie l'affichage pour dessiner le joueur à sa place (sans modifier la carte elle-même !).
4. Affiche aussi `pos` et `dir` sous la carte avec `printf("%.2f", ...)`.

<details>
<summary>Indice 1</summary>

Pendant l'affichage, pour chaque case `(x, y)`, demande-toi : « est-ce que le joueur est dans cette case ? ».
Rappelle-toi comment on passe d'une position continue à une case.

</details>

<details>
<summary>Indice 2</summary>

Une conversion `(int)` sur un `double` tronque vers zéro. Pour des positions positives, c'est la même chose
qu'arrondir à l'inférieur. Compare l'entier obtenu avec tes indices de boucle.

</details>

### Défi 2.3 — La boucle de jeu (version terminal)

Écris la boucle principale : afficher → lire une touche → mettre à jour → recommencer, jusqu'à `x`.

```c
/* Squelette */
int main(void)
{
    /* TODO : initialiser la carte et le joueur */
    while (/* TODO : condition de fin */)
    {
        /* TODO : effacer l'écran et afficher */
        /* TODO : lire une touche */
        /* TODO : appliquer l'action */
    }
    return 0;
}
```

<details>
<summary>Indice 1</summary>

Une variable booléenne `running` (avec `<stdbool.h>`, tu as `bool`, `true`, `false`).
Un `switch` sur le caractère lu est plus lisible qu'une suite de `if`.

</details>

### Défi 2.4 — Tourner et avancer

1. Écris une fonction qui fait tourner le joueur d'un angle donné.
2. Écris une fonction qui le fait avancer (ou reculer) d'une distance donnée.
3. Vérifie que la longueur de `dir` reste à 1 après 100 rotations (affiche `sqrt(dirX² + dirY²)`).

```c
void player_rotate(t_player *p, double angle);   /* TODO */
void player_move(t_player *p, double dist /* TODO : de quoi d'autre a-t-on besoin ? */);
```

<details>
<summary>Indice 1</summary>

Pour la rotation : une variable temporaire pour sauvegarder l'ancienne composante x. Exactement le
même principe que la fonction `echanger` plus haut.

</details>

<details>
<summary>Indice 2</summary>

Reculer, c'est avancer d'une distance négative. Une seule fonction suffit.

</details>

### Défi 2.5 — Les collisions

Le joueur ne doit pas pouvoir entrer dans un mur.

<details>
<summary>Indice 1</summary>

Avant de déplacer, calcule la position **où il irait**, et regarde quelle case c'est.

</details>

<details>
<summary>Indice 2</summary>

Si tu refuses le déplacement en bloc quand la destination est un mur, le joueur « colle » aux murs quand il
arrive en biais. Essaie plutôt de tester **séparément** le déplacement en x et le déplacement en y : le joueur
glissera le long du mur. Pourquoi ça marche ?

</details>

<details>
<summary>Indice 3</summary>

Que se passe-t-il si la position calculée sort de la carte (indice négatif ou trop grand) ? Pour l'instant
ta bordure de murs te protège… mais ajoute quand même une fonction « est-ce que cette case est un mur ? »
qui considère « hors carte » comme un mur. Tu la réutiliseras à l'étape 3.

</details>

### Défi 2.6 (optionnel) — Mode brut du terminal

Fais en sorte que les touches soient prises en compte **sans appuyer sur Entrée**, et que le terminal soit
remis dans son état normal en quittant.

<details>
<summary>Indice 1</summary>

`man 3 termios`. Lis la description de `tcgetattr`, `tcsetattr`, et des drapeaux `ICANON` et `ECHO`
dans le champ `c_lflag`.

</details>

<details>
<summary>Indice 2</summary>

Sauvegarde la configuration d'origine dans une variable **avant** de la modifier, pour la restaurer à la fin.
Si ton programme plante, ton terminal restera bizarre : la commande `reset` le répare.

</details>

### Défi 2.7 — Organisation du code

Si tout est dans `main.c`, découpe : par exemple `map.c`, `player.c`, `render_ascii.c`, avec leurs headers.
Ton Makefile de l'étape 1 doit fonctionner sans modification majeure.

---

## 🐞 Erreurs classiques et débogage

| Symptôme | Cause probable | Piste |
|----------|----------------|-------|
| `undefined reference to 'cos'` | Bibliothèque mathématique non liée | `-lm`, **à la fin** de la commande de link |
| Le joueur ne bouge pas alors que la fonction est appelée | Struct passée par copie | Pointeur ! |
| Le joueur se déplace « en diagonale bizarre » ou la carte est transposée | `map[x][y]` au lieu de `map[y][x]` | Vérifie ta convention partout |
| La direction rétrécit après plusieurs rotations | `dirX` écrasé avant le calcul de `dirY` | Variable temporaire |
| Chaque touche est traitée deux fois | Le `\n` de Entrée est lu comme une touche | `scanf(" %c")` ou ignorer `'\n'` |
| `Segmentation fault` | Accès hors du tableau | `gdb` puis `bt`, ou compile avec `-fsanitize=address` |
| Avertissement `comparison of integer expressions of different signedness` | Comparaison `int` / `size_t` | Utilise le même type pour tes indices |

### Déboguer un segfault avec gdb

```sh
gdb ./3dmotor
(gdb) run
# ... Program received signal SIGSEGV
(gdb) bt                  # dans quelle fonction, appelée par qui ?
(gdb) frame 1             # remonte d'un cran dans la pile
(gdb) info locals         # toutes les variables locales
(gdb) print p->pos_x      # inspecter une valeur précise
```

### Un `printf` de débogage efficace

Affiche toujours **le nom et la valeur**, et envoie-le sur `stderr` (non bufferisé) :
`fprintf(stderr, "[dbg] case=(%d,%d) mur=%d\n", cx, cy, w);`

---

## ✅ Checklist de validation

- [ ] Ma carte s'affiche correctement, entourée de murs.
- [ ] Mon joueur est une `struct` avec une position en `double` et un vecteur direction.
- [ ] Je sais expliquer pourquoi `player_move` prend un pointeur.
- [ ] Le joueur tourne par petits angles, et la flèche affichée suit sa direction.
- [ ] Après 100 rotations, la longueur de `dir` vaut toujours ~1.000.
- [ ] Le joueur avance dans la direction regardée, et glisse le long des murs sans les traverser.
- [ ] Aucune lecture hors de la carte, même en forçant le joueur contre un bord (testé avec `-fsanitize=address`).
- [ ] Le code est découpé en plusieurs fichiers ; zéro avertissement avec `-Wall -Wextra`.
- [ ] J'ai fait un commit.

[← Étape précédente](01-mise-en-place.md) · [Sommaire](README.md) · [Étape suivante →](03-raycasting-ascii.md)
