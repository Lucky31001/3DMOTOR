# Étape 5 — Textures sur les murs, puis sol et plafond

[← Étape précédente](04-sdl2-fenetre-et-boucle.md) · [Sommaire](README.md) · [Étape suivante →](06-bonus.md)

## 🎯 Objectif

Remplacer les murs de couleur unie par des **murs texturés** (briques, pierre, bois…),
puis texturer aussi le **sol** et le **plafond**. C'est l'étape où ton moteur commence vraiment
à ressembler à un jeu.

Elle se fait en trois temps :
1. textures **générées par le code** (pas besoin de fichiers) ;
2. textures **chargées depuis des fichiers** `.bmp` ;
3. **sol et plafond** texturés.

### Résultat attendu

- Chaque type de mur (`1`, `2`, `3`…) a sa propre texture, qui « colle » au mur : quand tu te déplaces,
  les motifs restent fixes sur le mur (ils ne glissent pas).
- Quand tu t'approches d'un mur, la texture s'agrandit sans se déformer ; de très près, elle reste correcte
  (pas de bouillie ni de crash).
- Le sol et le plafond ont un motif qui recule correctement vers l'horizon, avec la perspective.
- Tout tourne encore à ~60 FPS en 640×480 (compilé en `-O2`).

---

## 📚 Notions de C abordées

### Une texture, c'est encore un tableau de pixels

Exactement comme ton buffer d'écran : un tableau 1D de `uint32_t` de `largeur × hauteur` cases,
avec l'indice `ty × largeur + tx`. Une bonne idée : une struct qui regroupe le pointeur et les dimensions.

```c
/* Suggestion */
typedef struct s_texture {
    uint32_t *pixels;
    int       width;
    int       height;
} t_texture;
```

Et pour gérer plusieurs textures : un **tableau de structs**, indexé par le type de mur (attention au décalage :
le type `1` correspond-il à l'indice `0` ou `1` ?).

### Tableaux de structs et pointeurs vers des éléments

```c
/* Exemple générique */
t_rectangle formes[3];
t_rectangle *r = &formes[1];    /* pointeur vers le 2e élément */
r->largeur = 3.0;               /* modifie formes[1] */
```

Passer `&textures[i]` à une fonction évite de copier la struct, et `const t_texture *` documente qu'on ne la modifie pas.

### Lire une image avec SDL : `SDL_Surface`

`SDL_LoadBMP("fichier.bmp")` renvoie un `SDL_Surface *` (ou `NULL`). Une surface contient notamment :

- `w`, `h` : dimensions ;
- `format` : le format des pixels — **pas forcément** celui que tu utilises !
- `pixels` : un `void *` vers les données ;
- `pitch` : nombre d'**octets** par ligne (peut être **plus grand** que `w × taille_d_un_pixel` à cause d'alignements).

Fonctions à chercher dans le wiki SDL2 : `SDL_LoadBMP`, `SDL_ConvertSurfaceFormat`, `SDL_LockSurface` (avec `SDL_MUSTLOCK`),
`SDL_FreeSurface`.

### `void *` et l'arithmétique de pointeurs

- `void *` est un « pointeur vers n'importe quoi ». On ne peut pas le déréférencer directement : il faut le **convertir**
  vers un type précis.
- L'arithmétique de pointeurs se fait **en nombre d'éléments** du type pointé : si `p` est un `uint32_t *`, `p + 1`
  avance de **4 octets**. Si `q` est un `uint8_t *`, `q + 1` avance de **1 octet**.

Question qui fera l'objet d'un défi : le `pitch` est en octets. Pour trouver le début de la ligne `y` d'une surface,
vers quel type de pointeur faut-il convertir `pixels` avant d'ajouter `y × pitch` ?

### `memcpy`

`memcpy(destination, source, nb_octets)` (dans `<string.h>`) copie un bloc de mémoire. Utile pour copier une ligne
entière de pixels d'un coup. Attention : les deux zones ne doivent pas se chevaucher.

### Puissances de deux et masque

Si la largeur d'une texture est une puissance de deux (64, 128, 256…), alors `x % largeur` peut s'écrire `x & (largeur − 1)`.
C'est plus rapide **et** ça se comporte bien avec les nombres négatifs (contrairement à `%` en C, qui peut renvoyer un
résultat négatif). Vérifie-le sur papier avec `largeur = 8` : que vaut `7` en binaire ?

---

## 📐 Les maths nécessaires

### 1. Où le rayon a-t-il touché le mur ? (`wallX`)

Pour choisir **quelle colonne de la texture** dessiner, il faut savoir **où**, sur la face du mur (entre 0 et 1), le rayon a frappé.

On connaît `t` à l'impact (c'est `perpWallDist`), donc le point d'impact exact est :

```
impact = pos + perpWallDist × rayDir
```

Mais **laquelle** des deux coordonnées de `impact` nous intéresse ?

```
     side == 0 : le rayon a franchi une ligne VERTICALE (x entier)
                 → la face du mur est verticale sur la carte
                 → la position le long de la face varie en … ?

            │▓▓▓▓▓
            │▓▓▓▓▓       ● impact : x est entier (on est SUR la ligne),
      ──────●▓▓▓▓▓         c'est l'autre coordonnée qui nous dit où
          ╱ │▓▓▓▓▓         on est le long du mur
        ╱   │▓▓▓▓▓
```

Une fois la bonne coordonnée trouvée, on ne garde que sa **partie fractionnaire** (ce qui est après la virgule) :
c'est `wallX ∈ [0, 1[`.

Puis : `texX = wallX × largeur_texture` (en entier).

⚠️ Selon le sens du rayon, la texture peut apparaître **en miroir** sur certaines faces (un texte écrit sur la texture
se lirait à l'envers). Il faut alors retourner `texX`. Pour le repérer, utilise une texture **asymétrique** (une flèche, une lettre).

### 2. Quelle ligne de la texture pour chaque pixel ? (`texY`)

La colonne de mur à l'écran fait `lineHeight` pixels, et la texture `texH` pixels de haut. On fait correspondre les deux
**linéairement** :

```
 écran                          texture
 drawStart  ┬  ─────────────►   ┬ 0
            │                   │
            │   chaque pixel    │   on avance de
            │   d'écran ...     │   texH / lineHeight
            │                   │   texels à chaque
 drawEnd    ┴  ─────────────►   ┴ texH   pixel d'écran
```

Donc on calcule un **pas** `step = texH / lineHeight` (en `double` !) et une position de départ, puis on avance
de `step` à chaque pixel.

⚠️ **Le piège** : quand le mur est très proche, `lineHeight` est plus grand que l'écran, et tu as **borné** `drawStart` à 0
à l'étape 3. Si tu commences la texture à 0 à cet endroit-là, la texture sera **écrasée** (on verra son haut alors qu'on
devrait voir son milieu). La position de départ dans la texture doit tenir compte de **là où la colonne aurait commencé**
sans le bornage. Dessine le cas sur papier.

### 3. Sol et plafond : le « floor casting » horizontal

Pour les murs, on balayait l'écran **colonne par colonne**. Pour le sol, il est bien plus efficace de balayer **ligne par
ligne** : sur une même ligne horizontale de l'écran, tous les pixels de sol sont **à la même distance** du joueur.

**Vue de côté** (le joueur, son œil et l'écran) :

```
                 écran
      œil  ●─────┬──────────────────────── horizon (milieu de l'écran)
           │ ╲   │
   posZ    │   ╲ │  p = (y − hauteur/2) : distance en pixels
 (hauteur  │     ┼  ← ligne y de l'écran, sous l'horizon
  de l'œil)│     │ ╲
           │     │    ╲
           │     │       ╲
  ─────────┴─────┴──────────●──────  sol
           ◄─────────────────►
               rowDistance
```

Par **triangles semblables** : le rayon qui passe par la ligne `y` de l'écran descend de `p` pixels sur la « distance écran »,
et de `posZ` jusqu'au sol. On obtient la distance (perpendiculaire) au sol pour cette ligne :

```
rowDistance = posZ / p
```

avec `posZ = hauteur_écran / 2` si l'œil est à mi-hauteur entre sol et plafond (le cas de Wolfenstein).

Ensuite, pour la ligne `y` :
- le rayon de la colonne de **gauche** a pour direction `dir − plane`, celui de **droite** `dir + plane` ;
- le point de sol vu par la colonne de gauche est `pos + rowDistance × (dir − plane)` ;
- en allant d'une colonne à la suivante, ce point se déplace d'un **pas constant** :
  `rowDistance × ((dir + plane) − (dir − plane)) / largeur_écran`.

Pour chaque pixel, la **partie entière** du point de sol donne la case, et la **partie fractionnaire** donne la position
dans la texture (comme `wallX`).

Le **plafond** est le symétrique du sol par rapport à l'horizon : la ligne `y` du sol et la ligne `hauteur − y − 1` du plafond
correspondent à la même distance.

---

## 🧩 Défis

### Défi 5.1 — Textures procédurales

Génère au démarrage (dans des buffers alloués avec `malloc`) au moins **trois** textures 64×64 calculées à partir de `x` et `y` :

- un **damier** ;
- un motif « XOR » (la couleur dépend de `x ^ y` — essaie, c'est joli) ;
- des **briques** (rangées décalées une sur deux, joints plus sombres) ;
- **une texture asymétrique** (une flèche, une lettre, un dégradé de gauche à droite) : indispensable pour déboguer les miroirs.

Bonus : affiche une texture en 2D dans un coin de l'écran (copie pixel par pixel) pour vérifier qu'elle est correcte
**avant** de l'appliquer sur les murs.

<details>
<summary>Indice 1</summary>

Pour un damier de cases de 8 pixels : divise `x` et `y` par 8, puis regarde la **parité** de la somme.

</details>

<details>
<summary>Indice 2</summary>

Pour les briques : chaque rangée fait par exemple 16 pixels de haut. Le numéro de rangée te dit si tu dois décaler
`x` d'une demi-brique. Ensuite, un pixel est un joint si sa position *dans* la brique (modulo) est sur un bord.

</details>

### Défi 5.2 — Murs texturés

1. Calcule `wallX`, puis `texX`, pour chaque colonne.
2. Calcule `step` et la position de départ dans la texture.
3. Pour chaque pixel de la colonne, lis le texel et écris-le dans le buffer.
4. Assombris toujours les faces `side == 1`.

```c
/* Signature suggérée */
void draw_textured_column(/* TODO : buffer d'écran */, int x,
                          const t_hit *hit, const t_texture *tex /* TODO : autre chose ? */);
```

<details>
<summary>Indice 1</summary>

Pour `wallX` : si `side == 0`, le rayon a franchi une ligne où **x** est entier. La coordonnée intéressante est donc l'autre.
Partie fractionnaire : `v − floor(v)`.

</details>

<details>
<summary>Indice 2</summary>

Ton `t_hit` de l'étape 3 contient peut-être déjà tout ce qu'il faut. Sinon, ajoute-y ce qui manque (`wallX` ?) : c'est
au moment du DDA que tu as les valeurs sous la main.

</details>

<details>
<summary>Indice 3 (texture qui glisse ou écrasée de près)</summary>

Si la texture « glisse » quand tu avances : `wallX` est calculé avec la mauvaise coordonnée ou la mauvaise distance.
Si elle est écrasée de près : relis le piège de la section 2. La position de départ doit valoir ce qu'elle aurait valu
**en haut de la colonne non bornée**, plus le nombre de pixels sautés multiplié par `step`.

</details>

<details>
<summary>Indice 4 (miroir)</summary>

Avec ta texture asymétrique, fais le tour d'un pilier isolé. Sur quelles faces est-elle à l'envers ? Ces faces ont en
commun le `side` **et** le signe d'une composante de `rayDir`. Dans ce cas : `texX = largeur − texX − 1`.

</details>

### Défi 5.3 — Charger des textures `.bmp`

1. Crée un dossier `assets/` et mets-y des `.bmp` 64×64 (fais-les toi-même dans GIMP/Krita, ou cherche des textures
   libres de droits — mentionne la source et la licence dans un `assets/CREDITS.md`).
2. Écris une fonction qui charge un `.bmp` dans ta struct `t_texture`.
3. Gère les erreurs : fichier absent, mauvaises dimensions…

```c
/* Renvoie 0 en cas de succès, -1 en cas d'erreur */
int texture_load_bmp(t_texture *tex, const char *path);   /* TODO */
void texture_destroy(t_texture *tex);                     /* TODO */
```

<details>
<summary>Indice 1</summary>

Ne suppose rien du format de la surface chargée (24 bits ? 32 bits ? palette ?). Convertis-la d'abord vers **ton** format
avec `SDL_ConvertSurfaceFormat`. Et n'oublie pas de libérer les **deux** surfaces.

</details>

<details>
<summary>Indice 2</summary>

Copie ligne par ligne : la ligne `y` de la surface commence à `(octet *)pixels + y × pitch`, et contient `w` pixels de
4 octets. Dans ton buffer, la ligne `y` commence à `y × w` (en éléments). `memcpy` est ton ami.

</details>

<details>
<summary>Indice 3</summary>

Pour tester la gestion d'erreurs : renomme temporairement un fichier, lance le jeu, il doit afficher un message clair
(avec le chemin et `SDL_GetError()`) et quitter **sans fuite** (vérifie avec valgrind).

</details>

### Défi 5.4 — Sol et plafond

1. Avant de dessiner les murs, remplis tout l'écran avec le sol (moitié basse) et le plafond (moitié haute), texturés,
   ligne par ligne.
2. Les murs dessinés ensuite recouvriront ce qui est derrière eux.

```c
void draw_floor_ceiling(/* TODO */);
```

<details>
<summary>Indice 1</summary>

Une boucle sur les lignes `y` de la moitié basse de l'écran. Pour chaque ligne : calcule `p`, `rowDistance`, le point de
sol de la colonne 0 et le pas. Puis une boucle sur les colonnes `x` où tu avances le point de sol du pas à chaque colonne.

</details>

<details>
<summary>Indice 2</summary>

Pour la position dans la texture : partie fractionnaire du point de sol × taille de la texture, puis le masque `& (taille − 1)`
pour être sûr de rester dans la texture même avec des erreurs d'arrondi.

</details>

<details>
<summary>Indice 3</summary>

Attention à la ligne exactement sur l'horizon : que vaut `p` ? Que vaut `rowDistance` ?

</details>

### Défi 5.5 — Brouillard (optionnel)

Assombris murs, sol et plafond en fonction de la distance, pour donner de la profondeur (un couloir qui s'enfonce dans le noir).
Réutilise ta fonction d'assombrissement de l'étape 4.

### Défi 5.6 — Performance (optionnel)

1. Mesure le temps de rendu d'une image (en ms) en `-O0` puis en `-O2`.
2. Le tutoriel de Lode mentionne que stocker les textures **transposées** (colonne par colonne) accélère le rendu des murs.
   Pourquoi ? Indice de réflexion : dans quel ordre lis-tu les texels quand tu dessines une colonne de mur ? Renseigne-toi sur
   le **cache du processeur**. Teste et mesure.

---

## 🐞 Erreurs classiques et débogage

| Symptôme | Cause probable | Piste |
|----------|----------------|-------|
| Texture qui « glisse » sur le mur quand on se déplace | `wallX` faux (mauvaise coordonnée) | Indice 1 du défi 5.2 |
| Texture écrasée / déformée de très près | Position de départ calculée après bornage | Section 2 des maths |
| Texture en miroir sur certaines faces | `texX` à retourner | Texture asymétrique + indice 4 |
| Segfault aléatoire, surtout de près | `texY` ou `texX` sort de `[0, taille[` (arrondi) | Masque `& (taille − 1)` ou borne ; `-fsanitize=address` |
| Image chargée décalée en diagonale | `pitch` ignoré lors de la copie | Copie ligne par ligne avec le pitch |
| Couleurs folles sur la texture chargée | Surface non convertie au bon format | `SDL_ConvertSurfaceFormat` |
| Le sol « tourne » à l'envers du mur | Mauvais signe dans `dir ± plane` | Teste avec un damier, tourne lentement |
| Ligne d'horizon étrange / FPS qui s'effondre | Division par `p = 0` | Indice 3 du défi 5.4 |
| Fuite mémoire signalée par valgrind | Surface ou buffer de texture non libéré | Chaque `load` a son `destroy` |

### Visualiser les valeurs intermédiaires

Une technique puissante : au lieu de la texture, dessine `wallX` sous forme de **couleur** (0 → noir, 1 → blanc).
Tu dois voir un dégradé régulier sur chaque face de mur. Pareil pour le sol : colorie avec la partie fractionnaire du point
de sol. Les erreurs sautent aux yeux.

---

## ✅ Checklist de validation

- [ ] Mes textures sont des structs `pixels + dimensions`, allouées et libérées proprement.
- [ ] J'ai au moins trois textures procédurales, dont une asymétrique.
- [ ] Je sais expliquer comment `wallX` est calculé et pourquoi on choisit telle ou telle coordonnée.
- [ ] Les textures restent fixes sur les murs quand je me déplace, et ne sont en miroir sur aucune face.
- [ ] Les textures sont correctes même collé contre un mur.
- [ ] Je charge des `.bmp` depuis `assets/` en gérant `pitch` et format ; les erreurs sont gérées sans fuite.
- [ ] Je sais expliquer la formule `rowDistance = posZ / p` avec un schéma de triangles semblables.
- [ ] Le sol et le plafond sont texturés avec une perspective correcte.
- [ ] ~60 FPS en 640×480 en `-O2`.
- [ ] valgrind / ASan propres ; j'ai fait un commit.

[← Étape précédente](04-sdl2-fenetre-et-boucle.md) · [Sommaire](README.md) · [Étape suivante →](06-bonus.md)
