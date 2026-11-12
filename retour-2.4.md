# Retour sur le défi 2.4 — Tourner et avancer

Il y a deux ou trois vrais bugs, mais la base est bonne. Ce retour indique **où** regarder et **pourquoi** : la correction est pour toi.

## ✅ Ce qui est juste

- **La rotation (`src/player.c:10-14`)** : la formule est correcte et tu as évité le piège de l'énoncé. Tu calcules `new_y` avec l'ancien `dir_x` avant de l'écraser. 👍
- **L'avancée (`src/player.c:36-37`)** : `pos + dir × déplacement`, c'est exactement la formule du guide.

## 🐛 Bug 1 : `dir` part de `(0, 0)` (`src/main.c:9`)

Applique la formule de rotation au vecteur `(0, 0)` : qu'est-ce que tu obtiens ? Et pour l'avancée, que vaut `pos + (0,0) × 1` ?

👉 Relis dans le guide les propriétés que `dir` doit avoir. Quelle valeur de départ correspond à l'état initial attendu (la flèche `>` dans le résultat attendu) ?

## 🐛 Bug 2 : `deplacement` n'est pas initialisée (`src/player.c:17`)

Quand tu appuies sur Q ou D, quelle valeur a `deplacement` à la ligne 36 ? Et pour une touche inconnue ?

👉 Compile avec `-Wall -Wextra` : le compilateur devrait peut-être t'avertir. Pose-toi aussi cette question : quand on tourne, est-ce qu'on doit seulement exécuter les lignes 36 à 46 ?

## 🐛 Bug 3 : tu réécris `dir` après le déplacement (`src/player.c:41-42`)

`player_pos_x - pos_x`, ça vaut quoi, en remplaçant par la formule de la ligne 36 ?

👉 Fais le calcul à la main pour la touche S (`deplacement = -MOVE`). Où regarde le joueur après avoir reculé ? Et que devient la longueur de `dir` si un jour `MOVE` vaut 0.5 ?

**Question clé : avancer doit-il changer la direction du regard ?**

## ⚠️ Le sens de rotation

Le guide te prévient que l'axe y pointe vers le bas. Fais le calcul à la main : `dir = (1, 0)` (vers l'est), tu appuies sur Q (gauche), donc rotation de `+π/2`. Tu obtiens quel vecteur ? Avec y vers le bas, est-ce que ça pointe vers le nord ou vers le sud ?

## 🔧 Points plus mineurs

- **`#define ROTATE M_PI/2`** : le préprocesseur fait un simple copier-coller de texte. Que donnerait `1 / ROTATE` une fois remplacé ? Quelle est la bonne habitude pour les `#define` qui contiennent une expression ?
- **`M_PI` dans `include/utils.h`** : ce header n'inclut pas `<math.h>`. Ça compile seulement parce que `player.c` l'inclut *avant*. Que se passe-t-il si un autre `.c` utilise `ROTATE` sans inclure math.h ?
- **Le point 3 de la consigne n'est pas fait** : afficher `sqrt(dir_x² + dir_y²)` après 100 rotations. Fais-le, c'est un excellent test pour vérifier ta rotation.

## 💭 Conception (non bloquant)

Le guide propose :

```c
void player_rotate(t_player *p, double angle);
void player_move(t_player *p, double dist /* + ? */);
```

Toi, ta `move_player` reçoit une **touche** et mélange deux rôles : interpréter le clavier et faire les maths. Et si l'étape 4 (SDL) change les touches, ou demande d'avancer de 0.05 à chaque frame ?

Le TODO « de quoi d'autre a-t-on besoin ? » a sa réponse dans ta ligne 39.

## 📋 Ordre conseillé

1. [ ] Bug 1 : valeur initiale de `dir`
2. [ ] Bug 3 : `dir` modifié par le déplacement
3. [ ] Bug 2 : `deplacement` non initialisée / chemin de la rotation
4. [ ] Vérifier le sens de rotation à la main
5. [ ] Test de la longueur de `dir` après 100 rotations
6. [ ] Points mineurs + réflexion sur la conception
