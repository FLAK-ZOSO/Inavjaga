# Inävjaga

**Inävjaga** is a [Sista](https://github.com/FLAK-ZOSO/Sista)-based C++ terminal videogame.

![alt text](banner.png)

## Description

### Purpose

Your character is represented by the `$` symbol and is surviving day by day hunting for food and protecting its home from the dangers of the tunnels.

![Home area](home.png)

If the enemies will reach home, no hopes will be left, so you need to defend it at all costs.

### Worms

The tunnels are haunted by worms such as the one represented below.

```bash
>>>>
   v
   v>H
```

Vulnerable to headshots, worms leave behind clay that can be used to craft mines.

### Archers

The archers are smart enemies who know well the tunnels and can move through the breaches in the walls.

```bash
A
```

They can shoot you from a distance, so you need to be careful when they are around. If you manage to kill them, you can harvest their arrows.

### Resources

The game features various resources that you can collect:

- **Clay**: can be used to craft mines; it is left behind by worms;
- **Bullets**: can be fired; as such both archers' arrows and worms' scales can be used as bullets;
- **Meat**: is eaten to avoid starving; it is dropped by worms and archers;

### Portals

The game features portals that can be used exclusively by the player to travel between different areas of the tunnels.

```bash
&
```

## Instructions

### Controls

The controls for motion are:
- `W`, `A`, `S`, `D`: move the character; both uppercase and lowercase letters work;

The controls for executing actions are:
- `I`, `J`, `K`, `L`: interact with the world; both uppercase and lowercase letters work;

The controls for switching modes are:
- `c`: switch to the ***collect*** mode; both uppercase and lowercase letters work;
- `b`: switch to the *shoot **bullet*** mode; both uppercase and lowercase letters work;
- `m`: switch to the *build **mine*** mode; both uppercase and lowercase letters work;
- `e`: switch to the *dump **chest*** mode; both uppercase and lowercase letters work; lowercase `q` works too;

The controls for game actions are:
- `Q`: quit the game; only uppercase letter works;
- `p`: pause the game and force reprint of the screen; both uppercase and lowercase letters work; `.` works too;
- `+`, `-`: speed up/down the game by a factor of 4; both `+` and `-` work;
