# Étape 6 — Bonus : minimap, chargement de niveaux, sprites, portes

[← Étape précédente](05-textures-sol-plafond.md) · [Sommaire](README.md)

## 🎯 Objectif

Transformer ton moteur en base de jeu. Les quatre bonus sont **indépendants**, mais voici l'ordre conseillé,
du plus simple au plus complexe :

1. **Minimap** — réutilise ce que tu sais déjà, et t'aidera à déboguer la suite.
2. **Chargement de niveaux depuis un fichier** — notions de C importantes (fichiers, allocation 2D).
3. **Sprites** — objets et ennemis dans le monde (maths un cran au-dessus).
4. **Portes** — modifie le DDA lui-même.

### Résultat attendu (une fois tout fait)

- `./3dmotor assets/levels/niveau1.map` charge un niveau ; un fichier mal formé produit un message d'erreur clair.
- Une minimap dans un coin montre la carte, le joueur, sa direction et (option) le cône des rayons.
- Des objets (tonneaux, lampes, plantes…) se tiennent dans le monde, sont correctement cachés par les murs et entre eux,
  et ont des parties transparentes.
- Des portes coulissent quand on appuie sur une touche devant elles, puis se referment toutes seules.

---

## 📚 Notions de C abordées

### `argc` / `argv`

`int main(int argc, char **argv)` : `argc` est le nombre d'arguments (le nom du programme compris), `argv` un tableau
de chaînes. `argv[0]` est le nom du programme, `argv[1]` le premier argument. Vérifie toujours `argc` avant de lire `argv[1]`.

### Lire un fichier texte

```c
/* Exemple générique : compter les lignes d'un fichier */
#include <stdio.h>

FILE *f = fopen(chemin, "r");
if (f == NULL)
{
    perror(chemin);   /* affiche « chemin: No such file or directory » par exemple */
    return -1;
}
char ligne[256];
int  n = 0;
while (fgets(ligne, sizeof(ligne), f) != NULL)
    n++;
fclose(f);
```

À savoir :
- `fgets` **garde** le `'\n'` final (s'il tenait dans le buffer). Pense à l'enlever (`strcspn` est pratique pour ça).
- Une ligne plus longue que le buffer est lue **en plusieurs fois** : est-ce un problème pour toi ? Comment le détecter ?
- Alternative POSIX : `getline` (`man 3 getline`) qui alloue elle-même un buffer de la bonne taille (à libérer avec `free`).
- Le dernier caractère du fichier n'est pas forcément un `'\n'`.
- Fichiers créés sous Windows : les lignes finissent par `"\r\n"`.
- **Toujours** `fclose`, y compris dans les chemins d'erreur.

### Allocation dynamique d'une grille 2D

Ta carte n'a plus une taille connue à la compilation. Deux approches :

```
 (A) Un seul bloc                    (B) Un tableau de lignes
 ┌──────────────────────────┐       rows ─► ┌───┐   ┌──────────┐
 │ l0 │ l1 │ l2 │ ... │ ln  │               │ ● ├──►│  ligne 0 │
 └──────────────────────────┘               ├───┤   └──────────┘
  accès : cells[y * w + x]                  │ ● ├──►│  ligne 1 │
  1 malloc, 1 free                          ├───┤   └──────────┘
                                            │...│   n + 1 malloc, n + 1 free
                                            └───┘   accès : rows[y][x]
```

(A) est plus simple à libérer et plus rapide (mémoire contiguë) ; c'est la même logique que ton buffer de pixels.
(B) permet la syntaxe `[y][x]` mais demande plus de rigueur. Choisis, et justifie ton choix dans un commentaire.

### `realloc` : un tableau qui grandit

Tu ne sais pas à l'avance combien de sprites contient le niveau. `realloc(ptr, nouvelle_taille)` agrandit un bloc
(en le déplaçant si besoin). Piège classique :

```c
/* Exemple générique — À NE PAS FAIRE */
tab = realloc(tab, nouvelle_taille);   /* si realloc échoue, renvoie NULL... et l'ancien bloc est perdu (fuite) */
```

Question : comment écrire ça correctement ? (Indice : une variable temporaire.)
Astuce de performance : doubler la capacité à chaque fois plutôt que d'ajouter 1.

### Pointeurs de fonctions et `qsort`

`qsort` (de `<stdlib.h>`) trie n'importe quel tableau. Comme il ne connaît pas tes types, tu lui donnes une **fonction
de comparaison** :

```c
/* Exemple générique : trier des élèves par note décroissante */
typedef struct s_eleve { const char *nom; double note; } t_eleve;

int compare_notes(const void *a, const void *b)
{
    const t_eleve *ea = a;
    const t_eleve *eb = b;
    if (ea->note < eb->note) return 1;    /* a après b */
    if (ea->note > eb->note) return -1;   /* a avant b */
    return 0;
}

qsort(eleves, nb_eleves, sizeof(eleves[0]), compare_notes);
```

`compare_notes` est passé **sans parenthèses** : c'est un **pointeur vers la fonction**, que `qsort` appellera lui-même.

⚠️ Ne fais pas `return ea->note - eb->note;` avec des `double` : le résultat est converti en `int`, et `0.3` devient `0`.

### `enum` pour des machines à états

Une porte est *fermée*, *en train de s'ouvrir*, *ouverte*, *en train de se fermer*. Un `enum` + un `switch` rend
ça lisible. Active `-Wswitch` (inclus dans `-Wall`) : gcc te prévient si tu oublies un cas.

---

## 🗺️ Bonus A — Minimap

### Ce qu'il faut

- Chaque case de la carte devient un petit carré de `N×N` pixels dans un coin de l'écran.
- Le joueur est un point (ou petit carré), et sa direction un segment.
- Option : les rayons du raycaster (le cône de vision), en utilisant les points d'impact calculés pendant le rendu.

### Défis

1. Écris une fonction `draw_rect` (avec bornage) et une fonction `draw_minimap`.
2. Dessine le joueur à la bonne position (attention : sa position est en `double`, en **cases**).
3. Dessine la direction : un segment de `pos` à `pos + dir × longueur`.
4. *(Option)* Dessine les rayons : mémorise le point d'impact de chaque colonne pendant le raycasting.
5. *(Option)* Rends la minimap semi-transparente : mélange la couleur de la minimap avec celle de l'image.

<details>
<summary>Indice 1</summary>

Conversion monde → minimap : `pixel = origine_minimap + coordonnée_monde × N`.

</details>

<details>
<summary>Indice 2 (segments)</summary>

Pour tracer un segment simplement : échantillonne-le. Avance de 0 à 1 par petits pas, calcule le point intermédiaire,
dessine un pixel. Combien de pas faut-il pour ne pas avoir de trous ? (Recherche aussi « algorithme de Bresenham » si
tu veux faire plus élégant.)

</details>

<details>
<summary>Indice 3 (transparence)</summary>

Mélange à 50 % : pour chaque canal, la moyenne des deux couleurs. Mélange quelconque : `c = a × α + b × (1 − α)` canal par canal.

</details>

---

## 📄 Bonus B — Chargement de niveaux depuis un fichier

### Un format proposé (tu peux inventer le tien)

```
# Ceci est un commentaire
# Les textures des types de murs
1 assets/textures/brique.bmp
2 assets/textures/pierre.bmp
MAP
1111111111
1000000001
10N0002001
1000000001
1111111111
```

Convention suggérée : `0` vide, chiffres = types de murs, `N`/`S`/`E`/`W` = position et orientation de départ du joueur
(à remplacer par `0` dans la grille), `D` = porte, des lettres pour les sprites… À toi de jouer.

### Ce qu'un bon chargeur vérifie

- le fichier existe et s'ouvre ;
- il y a **exactement un** point de départ ;
- toutes les lignes ont la même longueur (ou bien tu complètes avec du vide — décide) ;
- la carte est **fermée** : le joueur ne peut pas sortir de la grille (sinon ton DDA boucle à l'infini ou lit hors du tableau) ;
- tous les caractères sont connus ;
- les textures référencées existent.

Et à chaque erreur : un message qui dit **quoi** et **où** (`niveau1.map:7: caractère inconnu 'X'`), puis libération
de tout ce qui a déjà été alloué.

### Défis

1. Lis `argv[1]` (avec un niveau par défaut si absent).
2. Fais une première passe pour compter les lignes et trouver la largeur, ou utilise un tableau qui grandit avec `realloc`.
3. Alloue la grille, remplis-la.
4. Valide. Écris **un fichier de test par erreur** (`tests/maps/pas_de_joueur.map`, `tests/maps/carte_ouverte.map`…)
   et vérifie que chacun produit le bon message, sans fuite (valgrind).

```c
/* Renvoie 0 si OK, -1 sinon (et la carte est laissée vide/libérée) */
int  level_load(t_level *lvl, const char *path);   /* TODO */
void level_destroy(t_level *lvl);                  /* TODO */
```

<details>
<summary>Indice 1</summary>

Lire tout le fichier une première fois pour mesurer, puis `rewind(f)` (`man 3 rewind`) et le relire pour remplir,
est une approche simple et robuste.

</details>

<details>
<summary>Indice 2 (carte fermée)</summary>

Version simple : toutes les cases du bord sont des murs. Version tolérante (cartes non rectangulaires) : aucune case
**vide** ne doit être adjacente à une case hors-carte ou à un « trou ». Pour aller plus loin : un remplissage par
diffusion (*flood fill*) depuis la position du joueur ne doit jamais atteindre le bord.

</details>

<details>
<summary>Indice 3 (nettoyage en cas d'erreur)</summary>

Le motif `goto cleanup;` est courant et accepté en C pour centraliser la libération en cas d'erreur. Ton `level_destroy`
doit accepter un niveau à moitié rempli (pointeurs à `NULL` = rien à libérer ; `free(NULL)` ne fait rien).

</details>

---

## 🛢️ Bonus C — Sprites

Un sprite est une image 2D toujours **face à la caméra**, placée à une position `(x, y)` du monde.

### Les idées clés

**1. Le Z-buffer.** Pendant le rendu des murs, mémorise pour chaque colonne la distance perpendiculaire du mur
(`double zbuffer[largeur_écran]`). Une bande verticale de sprite n'est dessinée que si elle est **plus proche** que le mur
de cette colonne.

**2. Trier du plus loin au plus proche.** Si on dessine les sprites lointains d'abord, les proches les recouvrent
naturellement (« algorithme du peintre »). Pour trier par distance, la distance **au carré** suffit (pas besoin de `sqrt` :
pourquoi ?).

**3. Passer du monde à la caméra.** C'est le cœur mathématique. On a la position du sprite **relative** au joueur :
`s = sprite − pos`. On veut l'exprimer dans le repère de la caméra, c'est-à-dire trouver `(u, v)` tels que :

```
s = u × plane + v × dir
```

- `v` est la **profondeur** (distance devant le joueur, le long de `dir`) ;
- `u` dit à quel point le sprite est à gauche ou à droite (en unités de `plane`).

Écrit sous forme matricielle, ça donne un système 2×2 :

```
 ┌      ┐   ┌                ┐ ┌   ┐
 │ s.x  │   │ planeX   dirX  │ │ u │
 │      │ = │                │ │   │
 │ s.y  │   │ planeY   dirY  │ │ v │
 └      ┘   └                ┘ └   ┘
```

Pour retrouver `(u, v)`, il faut **l'inverse** de cette matrice. Rappel général : pour une matrice 2×2

```
     ┌      ┐                    ┌         ┐
 M = │ a  b │      M⁻¹ = 1/det × │  d  −b  │      avec det = a·d − b·c
     │ c  d │                    │ −c   a  │
     └      ┘                    └         ┘
```

**4. Projection à l'écran.** Une fois `(u, v)` connus :
- si `v ≤ 0`, le sprite est **derrière** le joueur : on ne le dessine pas ;
- sa colonne centrale à l'écran se déduit de `u / v` (rappelle-toi : `cameraX` va de −1 à +1 quand `u / v` va de… ?) ;
- sa hauteur (et sa largeur, s'il est carré) à l'écran est `hauteur_écran / v`, exactement comme un mur.

**5. Transparence.** Une couleur « magique » (souvent magenta `0xFF00FF`) ou un alpha nul signifie « ne pas dessiner ce pixel ».

### Défis

1. Une struct sprite (position, texture) et un tableau de sprites (fixe pour commencer, chargé depuis le niveau ensuite).
2. Remplis le Z-buffer pendant le rendu des murs.
3. Calcule la distance de chaque sprite et trie-les avec `qsort`.
4. Pour chaque sprite : passage dans le repère caméra, projection, puis dessin bande par bande avec test du Z-buffer
   et de la transparence.
5. Teste : un sprite derrière un mur doit être caché ; un sprite à moitié derrière un coin, à moitié caché ;
   deux sprites alignés, le plus proche devant.

<details>
<summary>Indice 1</summary>

Commence par dessiner un simple **rectangle de couleur** à la place du sprite. Quand sa position, sa taille et son
occultation sont bonnes, remplace par la texture.

</details>

<details>
<summary>Indice 2 (repère caméra)</summary>

Applique la formule de l'inverse 2×2 avec `a = planeX`, `b = dirX`, `c = planeY`, `d = dirY`. Le résultat de
`M⁻¹ × s` te donne `u` (première ligne) et `v` (seconde ligne). Vérifie sur un cas simple : un sprite pile devant le
joueur doit donner `u = 0`.

</details>

<details>
<summary>Indice 3 (colonne à l'écran)</summary>

Pour un rayon de l'étape 3, `rayDir = dir + plane × cameraX`. Pour un point du monde, `s = v × dir + u × plane`,
donc `s / v = dir + plane × (u / v)`. Le point est sur le rayon de la colonne dont le `cameraX` vaut… À toi d'inverser
ta formule `x → cameraX`.

</details>

<details>
<summary>Indice 4 (texture du sprite)</summary>

Même principe que les murs : pour chaque bande `x` à l'écran entre le bord gauche et le bord droit du sprite, calcule
la colonne de texture correspondante (proportionnellement), puis pour chaque `y` la ligne de texture. Pense à
borner à l'écran **sans** décaler la texture (même piège que pour les murs très proches).

</details>

---

## 🚪 Bonus D — Portes

### Le concept

Une porte occupe une case, mais au lieu d'être un bloc plein, c'est un **panneau fin au milieu de la case**, qui coulisse
latéralement. Chaque porte a un état et un « taux d'ouverture » (de 0 = fermée à 1 = ouverte).

```
 Vue de dessus d'une case porte :

  ┌───────────┐         ┌───────────┐         ┌───────────┐
  │           │         │           │         │           │
  │███████████│  fermée │██████     │  à 50 % │           │  ouverte
  │           │         │           │         │           │
  └───────────┘         └───────────┘         └───────────┘
        ▲ panneau au milieu de la case (décalé de 0,5)
```

### Ce qu'il faut modifier

1. **Le stockage** : en plus du type de case, il faut l'état de chaque porte. Un tableau parallèle à la grille, ou une
   liste de portes avec leurs coordonnées. Lequel est le plus simple pour retrouver « la porte de la case (x, y) » pendant le DDA ?
2. **Le DDA** : quand le rayon entre dans une case porte, il ne touche pas le bord de la case mais le **milieu**. Il faut
   calculer où il croise ce plan au milieu, vérifier qu'il le croise bien **avant de sortir de la case**, puis vérifier si
   le point d'impact tombe sur la **partie fermée** du panneau (compare `wallX` et le taux d'ouverture). Sinon, le rayon
   continue son chemin.
3. **La texture** : décalée selon le taux d'ouverture, pour qu'elle coulisse avec la porte.
4. **Les collisions** : on ne traverse une porte que si elle est (presque) ouverte.
5. **L'interaction** : une touche (par ex. `E`) ouvre la porte de la case située **devant** le joueur.
6. **L'animation** : le taux d'ouverture évolue avec `dt` ; la porte se referme après quelques secondes… sauf si le joueur
   est dedans !
7. **Les montants** : les murs adjacents à une porte utilisent souvent une texture spéciale (le « chambranle »).

### Défis

Fais-les dans cet ordre, en testant à chaque fois :

1. Une case `D` dessinée comme un mur normal mais au **milieu** de la case (sans ouverture).
2. Un taux d'ouverture fixe à 0,5 dans le code : la moitié du panneau a disparu et on voit derrière.
3. La touche d'interaction et l'animation avec `dt`.
4. Les collisions et la fermeture automatique.

<details>
<summary>Indice 1 (panneau au milieu)</summary>

Si le rayon vient de franchir une ligne verticale (`side == 0`) pour entrer dans la case porte, le panneau est à une
demi-case plus loin en x. En `t`, ça représente une demi `deltaDistX`. Le nouveau `t` d'impact est-il encore plus petit
que `sideDistY` (sinon le rayon sort par le côté de la case avant de toucher le panneau) ?

</details>

<details>
<summary>Indice 2 (partie ouverte)</summary>

Calcule `wallX` au point d'impact sur le panneau (comme à l'étape 5). Si la porte est ouverte à `o ∈ [0, 1]`, quelle
plage de `wallX` correspond à la partie encore fermée ?

</details>

<details>
<summary>Indice 3 (case devant le joueur)</summary>

`pos + dir × (un peu moins d'une case)`, puis partie entière. Ou, plus robuste : lancer un rayon depuis le centre de
l'écran et regarder si la première case touchée est une porte à moins d'une certaine distance.

</details>

Lis la [partie 4 du tutoriel de Lode](https://lodev.org/cgtutor/raycasting4.html) pour d'autres variantes (murs fins,
murs « poussables » façon passages secrets).

---

## 💡 Encore plus loin (idées libres)

- Regarder à la souris : `SDL_SetRelativeMouseMode` et les événements `SDL_MOUSEMOTION`.
- Une arme au premier plan (une image dessinée au centre-bas après tout le reste) qui se balance quand on marche.
- Du son : la bibliothèque `SDL2_mixer`.
- Des ennemis qui se déplacent vers le joueur (et ne traversent pas les murs : tu as déjà les collisions).
- Des sprites à 8 directions (l'image dépend de l'angle entre le regard de l'ennemi et le joueur).
- Un éditeur de niveaux minimal (cliquer sur la minimap pour poser/enlever des murs, puis sauvegarder avec `fprintf`).
- Paralléliser le rendu des colonnes avec des threads (`pthread`) : chaque thread gère une partie de l'écran.

---

## 🐞 Erreurs classiques et débogage

| Symptôme | Cause probable | Piste |
|----------|----------------|-------|
| Le niveau se charge mais la dernière ligne manque ou est trop courte | Fichier sans `'\n'` final, ou `'\r'` Windows | Affiche chaque ligne lue entre crochets : `[%s]` |
| Fuite signalée seulement quand le chargement échoue | Libération oubliée dans un chemin d'erreur | Tester chaque fichier d'erreur sous valgrind |
| Crash aléatoire après `realloc` | Pointeur vers l'ancien bloc encore utilisé | Après `realloc`, l'ancien pointeur est **invalide** |
| Sprites visibles à travers les murs | Z-buffer non rempli ou test inversé | Affiche le Z-buffer en niveaux de gris ! |
| Sprites qui « clignotent » ou mauvais ordre | Comparateur `qsort` faux | Test unitaire : trie 3 distances connues |
| Sprite affiché quand on lui tourne le dos | Pas de test `v > 0` | Section 4 des sprites |
| Sprite qui glisse latéralement quand on tourne | Erreur dans l'inverse de la matrice | Vérifie `det` et les signes avec un sprite pile devant |
| Une porte « mange » le mur d'à côté | Impact au milieu calculé alors que le rayon sort par le côté | Indice 1 des portes |
| Le joueur reste coincé dans une porte qui se ferme | Pas de test « joueur dans la case » | Interdire la fermeture |

---

## ✅ Checklist de validation

**Minimap**
- [ ] La minimap affiche la carte, le joueur et sa direction, à jour à chaque image.

**Niveaux**
- [ ] Mon programme charge un niveau passé en argument, avec un niveau par défaut.
- [ ] Chaque erreur de fichier produit un message précis (fichier + ligne) et aucune fuite (valgrind).
- [ ] J'ai un dossier de fichiers de test pour les cas d'erreur.

**Sprites**
- [ ] Je sais expliquer le passage monde → caméra avec l'inverse d'une matrice 2×2.
- [ ] Les sprites sont triés avec `qsort` et un comparateur correct.
- [ ] Les sprites sont cachés par les murs (Z-buffer) et entre eux, et ont des pixels transparents.

**Portes**
- [ ] Les portes s'ouvrent avec une touche, s'animent avec `dt`, se referment seules.
- [ ] La texture coulisse avec la porte, et on voit correctement à travers l'ouverture.
- [ ] Les collisions tiennent compte de l'état des portes.

**Global**
- [ ] Zéro avertissement, zéro erreur valgrind/ASan venant de mon code.
- [ ] Le code est découpé en modules cohérents, chacun avec son header.
- [ ] J'ai fait un commit… et je suis fier de mon moteur. 🎉

[← Étape précédente](05-textures-sol-plafond.md) · [Sommaire](README.md)
