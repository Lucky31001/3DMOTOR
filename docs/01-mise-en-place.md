# Étape 1 — Mise en place : gcc, Makefile, structure du projet

[← Sommaire](README.md) · [Étape suivante →](02-carte-ascii-et-joueur.md)

## 🎯 Objectif

Avoir un projet propre qui se compile avec une seule commande (`make`), avec les
avertissements du compilateur activés, et comprendre **ce qui se passe** quand on compile du C.

### Résultat attendu

```
$ make
gcc -Wall -Wextra -Werror -g -Iinclude -c src/main.c -o build/main.o
gcc -Wall -Wextra -Werror -g -Iinclude -c src/utils.c -o build/utils.o
gcc build/main.o build/utils.o -o 3dmotor
$ ./3dmotor
3DMOTOR v0.1 — bonjour !
2 + 3 = 5
$ make
make: '3dmotor' is up to date.
```

La seconde exécution de `make` ne recompile rien : c'est tout l'intérêt d'un Makefile.

---

## 📚 Notions de C abordées

### Du fichier `.c` à l'exécutable

Quand tu tapes `gcc fichier.c -o prog`, il se passe en réalité **quatre étapes** :

```
  fichier.c ──► [préprocesseur] ──► code « étendu » (les #include sont copiés-collés)
                     │
                     ▼
               [compilateur]   ──► assembleur
                     │
                     ▼
               [assembleur]    ──► fichier.o   (code machine, mais incomplet)
                     │
  autres .o ─────────┤
  bibliothèques ─────┤
                     ▼
               [éditeur de liens / linker] ──► exécutable
```

- **Préprocesseur** : traite les lignes qui commencent par `#` (`#include`, `#define`, `#ifndef`…). C'est du simple remplacement de texte.
- **Compilation** : chaque `.c` est compilé **séparément** en un `.o` (fichier objet). À ce stade, si `main.c` appelle une fonction définie dans `utils.c`, le compilateur a seulement besoin de connaître sa **déclaration** (son prototype).
- **Édition de liens (link)** : on assemble tous les `.o` et les bibliothèques, et on relie chaque appel de fonction à sa définition.

👉 Savoir à quelle étape une erreur se produit t'aide énormément à la corriger :
- `error: expected ';' before ...` → erreur de **compilation** (syntaxe).
- `undefined reference to 'ma_fonction'` → erreur de **link** : la fonction est déclarée mais introuvable (fichier `.o` oublié, ou bibliothèque manquante comme `-lm`).

### Déclaration vs définition, et fichiers d'en-tête

- Une **déclaration** (prototype) dit « cette fonction existe, voilà ses paramètres et son type de retour » :
  `int max(int a, int b);`
- Une **définition** contient le corps de la fonction (entre accolades).

On met les **déclarations** dans un fichier `.h` (header) et les **définitions** dans un `.c`.
Chaque `.c` qui veut utiliser ces fonctions fait `#include "le_header.h"`.

Un header doit être protégé contre la double inclusion avec une **include guard** :

```c
/* Exemple générique : geometrie.h */
#ifndef GEOMETRIE_H
# define GEOMETRIE_H

double aire_cercle(double rayon);

#endif
```

> `#include <stdio.h>` (chevrons) cherche dans les dossiers système.
> `#include "mon.h"` (guillemets) cherche d'abord à côté du fichier, puis dans les dossiers passés avec `-I`.

### La fonction `main`

C'est le point d'entrée. Elle renvoie un `int` : `0` signifie « tout s'est bien passé »,
une autre valeur signale une erreur. Tu peux vérifier dans le terminal avec `echo $?` juste après
avoir lancé ton programme.

### Les options de gcc à connaître

| Option | Rôle |
|--------|------|
| `-Wall -Wextra` | Active un grand nombre d'avertissements. **Toujours.** |
| `-Werror` | Transforme les avertissements en erreurs (t'oblige à les corriger). |
| `-g` | Ajoute les informations de débogage (indispensable pour `gdb` et `valgrind`). |
| `-O0` / `-O2` | Niveau d'optimisation (0 = aucune, pratique pour déboguer). |
| `-Iinclude` | Ajoute `include/` aux dossiers où chercher les `.h`. |
| `-c` | Compile seulement, produit un `.o`, sans faire le link. |
| `-o nom` | Nom du fichier de sortie. |
| `-lm` | Lie la bibliothèque mathématique (tu en auras besoin à l'étape 2/3). |
| `-fsanitize=address,undefined` | Ajoute des vérifications à l'exécution (débordements, comportements indéfinis). Excellent pour apprendre. |
| `-std=c11` ou `-std=c17` | Fixe la version du langage. |

---

## 🛠️ Le Makefile

`make` lit un fichier `Makefile` composé de **règles** :

```make
cible: dépendances
	commande        # ⚠️ cette ligne commence par une TABULATION, pas des espaces
```

`make` ne relance la commande **que si une dépendance est plus récente que la cible**.
C'est comme ça qu'il évite de tout recompiler.

### Variables et variables automatiques

```make
CC = gcc
CFLAGS = -Wall -Wextra
```

Dans une commande, on les utilise avec `$(CC)`, `$(CFLAGS)`.

Variables **automatiques**, disponibles dans une règle :

| Variable | Signification |
|----------|---------------|
| `$@` | le nom de la cible |
| `$<` | la **première** dépendance |
| `$^` | **toutes** les dépendances |

### Règles génériques (pattern rules)

Au lieu d'écrire une règle par fichier `.o`, on écrit une règle avec `%` :
« pour fabriquer `quelquechose.o`, il faut `quelquechose.c` ».

Exemple générique, pour un projet qui n'a rien à voir avec le tien :

```make
# Projet « calculatrice » : tout est dans le même dossier
calc: calc.o operations.o
	gcc $^ -o $@

%.o: %.c
	gcc -Wall -c $< -o $@
```

Ton Makefile, lui, devra gérer **des dossiers différents** (`src/` pour les sources, `build/` pour
les objets). C'est le premier défi.

### Fonctions utiles de make

- `$(wildcard src/*.c)` : liste les fichiers correspondant au motif.
- `$(patsubst motif,remplacement,liste)` ou la forme courte `$(VAR:.c=.o)` : transforme une liste de noms.
- `.PHONY: clean` : déclare que `clean` n'est pas un vrai fichier.

### Squelette à compléter

```make
NAME    = 3dmotor
CC      = gcc
CFLAGS  = # TODO : avertissements, debug, dossier des headers

SRC_DIR = src
OBJ_DIR = build

SRCS    = # TODO : la liste de tes .c (wildcard ou liste explicite)
OBJS    = # TODO : transformer src/xxx.c en build/xxx.o

all: $(NAME)

$(NAME): # TODO
	# TODO : édition de liens

# TODO : règle générique build/%.o à partir de src/%.c
#        (et pense à créer le dossier build/ s'il n'existe pas)

clean:
	# TODO : supprimer les .o

fclean: clean
	# TODO : supprimer aussi l'exécutable

re: fclean all

.PHONY: all clean fclean re
```

> 💬 Lister les fichiers à la main plutôt qu'avec `wildcard` est un choix tout à fait valable
> (c'est même imposé dans certaines écoles) : c'est plus explicite. À toi de choisir.

---

## 🧩 Défis

### Défi 1.1 — Le premier programme, à la main

1. Crée `src/main.c` qui affiche `3DMOTOR v0.1 — bonjour !`.
2. Compile-le **sans Makefile**, directement avec `gcc`, avec `-Wall -Wextra`.
3. Lance-le, puis affiche son code de retour avec `echo $?`.
4. Modifie `main` pour qu'elle renvoie `42`. Vérifie avec `echo $?`.

<details>
<summary>Indice 1</summary>

La forme générale est `gcc [options] fichiers_sources -o nom_executable`.
Pour lancer un exécutable du dossier courant, il faut préfixer son nom par `./`.

</details>

<details>
<summary>Indice 2</summary>

`printf` est déclarée dans `<stdio.h>`. Si tu oublies l'`#include`, lis bien l'avertissement de gcc :
il te le dit presque mot pour mot (« implicit declaration of function… »).

</details>

### Défi 1.2 — Découper en plusieurs fichiers

1. Crée `include/utils.h` avec le prototype d'une fonction `int add(int a, int b);` (oui, c'est trivial : le but est d'apprendre la mécanique).
2. Crée `src/utils.c` qui la définit.
3. Appelle-la depuis `main.c` pour afficher `2 + 3 = 5`.
4. Compile **chaque fichier séparément** avec `-c` pour obtenir deux `.o`, puis fais le link à la main.
5. Expérimente :
   - Que se passe-t-il si tu oublies `utils.o` au moment du link ? Quelle étape échoue ?
   - Que se passe-t-il si tu oublies l'`#include "utils.h"` dans `main.c` ?
   - Que se passe-t-il si tu oublies l'include guard et que tu inclus le header deux fois ?

<details>
<summary>Indice 1</summary>

Comme ton header est dans `include/` et pas à côté de `main.c`, il faut dire à gcc où le chercher.
Regarde le tableau des options ci-dessus.

</details>

<details>
<summary>Indice 2</summary>

Trois commandes : une compilation `-c` par fichier `.c`, puis une commande qui prend les deux `.o`
et produit l'exécutable. L'option `-I` n'est utile qu'aux deux premières (pourquoi ?).

</details>

### Défi 1.3 — Le Makefile

Complète le squelette ci-dessus pour que :

- `make` compile tout et produise `3dmotor`,
- un deuxième `make` ne recompile rien,
- si tu modifies seulement `utils.c`, seul `utils.o` est recompilé (vérifie !),
- `make clean`, `make fclean` et `make re` fonctionnent,
- les `.o` vont dans `build/`, et `build/` est créé automatiquement.

<details>
<summary>Indice 1</summary>

Pour transformer une liste de chemins, regarde `patsubst` dans le manuel de make. Tu veux remplacer
à la fois le dossier **et** l'extension.

</details>

<details>
<summary>Indice 2</summary>

Ta règle générique a la forme `$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c`. Pour le dossier, la commande `mkdir -p`
ne râle pas si le dossier existe déjà. Tu peux la lancer dans la règle elle-même (avant la compilation),
ou chercher ce qu'est un « order-only prerequisite » (`| dossier`) dans le manuel de make.

</details>

<details>
<summary>Indice 3 (piège classique)</summary>

Si tu modifies **un header**, make ne recompile rien par défaut, car les `.h` ne sont pas dans les
dépendances. Pour l'instant, `make re` suffit. Plus tard, cherche les options `-MMD -MP` de gcc et la
directive `-include` de make pour générer les dépendances automatiquement.

</details>

### Défi 1.4 — Hygiène du dépôt

1. Crée un `.gitignore` pour ne pas commiter `build/` ni l'exécutable.
2. Fais ton premier commit « propre ».

### Défi 1.5 (optionnel) — Mode debug avec sanitizers

Ajoute une cible `make debug` (ou une variable, par exemple `make DEBUG=1`) qui compile avec
`-fsanitize=address,undefined`. Écris volontairement un bug (accès à `tab[10]` dans un tableau de 5 cases)
et observe le rapport.

<details>
<summary>Indice 1</summary>

Les sanitizers doivent être passés **à la compilation et au link**. Une variable `LDFLAGS` est utile.

</details>

---

## 🐞 Erreurs classiques et débogage

| Symptôme | Cause probable | Piste |
|----------|----------------|-------|
| `Makefile:12: *** missing separator. Stop.` | Des espaces au lieu d'une tabulation devant une commande | Configure ton éditeur pour afficher les tabulations. `cat -A Makefile` affiche `^I` pour une tab. |
| `fatal error: utils.h: No such file or directory` | gcc ne trouve pas le header | Option `-I` |
| `undefined reference to 'add'` | Erreur de **link** : `.o` manquant | Est-ce que `utils.o` est bien dans la commande finale ? |
| `implicit declaration of function` | Prototype absent | Un `#include` manque |
| `multiple definition of ...` | Une **définition** (corps de fonction, variable globale) placée dans un `.h` inclus par plusieurs `.c` | Un `.h` ne contient que des déclarations |
| `make` ne recompile pas après modification d'un `.h` | Dépendance manquante | `make re`, ou `-MMD -MP` |

### Première prise en main de gdb

Même si ton programme est trivial, prends 5 minutes pour essayer :

```sh
gdb ./3dmotor
(gdb) break main        # pose un point d'arrêt sur main
(gdb) run               # lance le programme
(gdb) next              # exécute la ligne suivante (sans entrer dans les fonctions)
(gdb) step              # entre dans la fonction appelée
(gdb) print a           # affiche la valeur d'une variable
(gdb) bt                # backtrace : la pile d'appels
(gdb) continue
(gdb) quit
```

Astuce : `gdb -tui ./3dmotor` affiche le code source pendant le débogage.

---

## ✅ Checklist de validation

- [x] Je sais expliquer la différence entre compilation et édition de liens.
- [x] Je sais dire, en lisant un message d'erreur, à quelle étape il s'est produit.
- [x] Mon projet a la structure `src/`, `include/`, `build/`.
- [x] `make`, `make clean`, `make fclean`, `make re` fonctionnent.
- [x] Un second `make` ne recompile rien ; modifier un seul `.c` ne recompile que lui.
- [x] Je compile avec `-Wall -Wextra -Werror -g` et j'ai **zéro** avertissement.
- [ ] Mes headers ont des include guards.
- [x] J'ai lancé mon programme dans `gdb`, posé un breakpoint et affiché une variable.
- [x] `build/` et l'exécutable sont ignorés par git ; j'ai fait un commit.

[← Sommaire](README.md) · [Étape suivante →](02-carte-ascii-et-joueur.md)
