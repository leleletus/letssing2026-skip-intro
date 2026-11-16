# Let's Sing 2026: canción aleatoria, saltar el intro y saltar el final

Mod para **Let's Sing 2026** (Nintendo Switch, Atmosphère). Añade tres cosas al juego, todas con **ZL y ZR**:

| Qué | Dónde | Cómo |
|---|---|---|
| **Canción aleatoria** | En la lista de canciones | Pulsar **ZL y ZR a la vez** |
| **Saltar el intro** | Durante el intro de una canción | **Pulsar** ZR o ZL (SL/SR en un Joy-Con suelto) |
| **Saltar el final** | Cuando ya no quedan notas y el video sigue | **Mantener** ZR o ZL 3 segundos |

Funciona en la partida normal (**Classic**) y en **Feat**, con las canciones oficiales, las de los DLC y las añadidas
con [Let's Sing UltraStar Converter](https://github.com/leleletus/ultrastar2singIt). Nunca en los modos online (Fest)
ni en Carrera. No cambia canciones, puntuaciones ni partidas guardadas.

## Canción aleatoria

En la lista de canciones, **ZL y ZR a la vez** empiezan una canción al azar, igual que si se hubiera elegido con A:
el juego pasa a la selección de jugadores con esa canción. Si no es la que apetece, **B** vuelve a la lista y ZL+ZR
saca otra.

De dónde sale la canción depende de cómo esté ordenada la lista:

| Orden de la lista | La canción sale de |
|---|---|
| Recomendadas | la fila donde está el cursor: «Para ti», «Canciones desafiantes», «Top 10», «Todas las canciones»... |
| Idioma, Año, Tema, Género | la fila donde está el cursor: ese idioma, ese año, ese género... |
| Título, Artista, Favoritas | todas las canciones de la lista (ahí las filas son solo letras) |

- **Se queda en la misma fila al repetir.** Al volver con B, el juego deja el cursor sobre esa canción en la primera
  fila que la tiene, que puede ser otra (por ejemplo «Todas las canciones»). Mientras el cursor siga sobre la canción
  que sacó el mod, el siguiente ZL+ZR elige de la fila de la que salió; si mueves el cursor, de la fila donde lo
  pongas.
- No hace falta pulsar los dos botones exactamente a la vez: la fila se lee cuando baja el primero, antes de que el
  juego reaccione a ese botón.
- Nunca elige una canción bloqueada ni una cuyos datos no estén en la consola, y no repite la anterior.
- También funciona con el detalle de una canción abierto. Mientras se edita una lista de reproducción no hace nada.
- Las filas de Recomendadas («Para ti», los tops) las rehace el juego según se juega: la canción sale de lo que haya
  en la fila en ese momento.

## Saltar el intro

Durante el intro de una canción, **pulsar** ZR o ZL salta a 3 s antes de la primera nota.

- Solo si faltan más de 6 s para la primera nota (en canciones con intro corto no hace nada).
- Se puede volver a usar al reintentar la canción. Un botón que ya estaba pulsado al empezar la canción no cuenta.

## Saltar el final

Cuando ya no quedan notas y el video sigue, **mantener** ZR o ZL 3 s salta a 1 s antes del final. La canción termina
como siempre: pantalla de resultados y puntuación guardada.

- Solo después de la última nota de todas las voces y si quedan más de 2 s de canción. A mitad de canción, mantener
  el botón no hace nada.
- Hay que mantener el botón 3 s seguidos, para que no pase sin querer; al soltarlo, la cuenta empieza de nuevo. Los
  3 s se cuentan con el reloj de la canción y no hay aviso en pantalla: contando deprisa se suelta antes de tiempo.

Los saltos usan la función del propio juego para moverse en la canción (`InGameSimple.SetTime`): audio, video y
letra saltan juntos. La canción aleatoria hace lo mismo que el juego al elegir una portada con A.

## Compatibilidad

**El mod solo funciona con una versión exacta del juego.** Llama a funciones del juego por su dirección dentro del
ejecutable, y esas direcciones cambian con cada actualización.

| | Compatible |
|---|---|
| Juego | Let's Sing 2026 (Nintendo Switch), title ID `0100EE4020D18000` |
| Actualización | **v131072** (versión 0.2.0, title ID `0100EE4020D18800`), y ninguna otra |
| Build ID del ejecutable (`main`) | `1C344DD17322D4FA3E8F50063340C7D0` |
| Juego base sin actualizar (v0) | No |
| DLC | No hace falta. Funciona con y sin DLC, con canciones añadidas o sin ellas |
| Firmware personalizado | Atmosphère (carga `atmosphere/contents/<title ID>/exefs/`) |
| Emuladores | Citron (nightly `308bc267c`): carga el mod y el juego arranca hasta el menú. El salto en una canción no está probado en el emulador |
| Otras ediciones (2018 a 2025) | No: son otros juegos, con otro ejecutable |

Probado en una consola real con Atmosphère:

| Versión | Fecha | Qué se probó |
|---|---|---|
| v1 | 2026-11-01 | saltar el intro en Classic |
| v2 | 2026-11-12 | saltar el intro y el final, en Classic y en Feat; mantener el botón a mitad de canción no hace nada |
| v3 | 2026-11-15 | canción aleatoria en Favoritas |
| v4 | 2026-11-15 | canción aleatoria repetida muchas veces desde la misma fila: no se sale de la fila. Sin confirmar todavía: la canción aleatoria en Feat (por el código del juego, es el mismo camino que en Classic) |

### Cómo saber qué versión tienes

- **DBI** → *Browse installed applications* → Let's Sing 2026: la actualización debe ser `v131072`
  (versión 0.2.0).
- **Build ID**: si tienes un volcado del juego, `hactool -t nso main` muestra el *Build ID* del ejecutable de la
  actualización. También aparece en los registros de error de Atmosphère (`sd:/atmosphere/crash_reports/`).

### Si el juego se actualiza

Con otra versión, lo más probable es que el juego se cierre al abrirlo o al empezar una canción. Para volver a jugar,
quita el mod (ver abajo). Mientras no se publique una versión nueva del mod, no actualices el juego si quieres seguir
usándolo. Cómo adaptarlo a otra versión: [Adaptarlo a otra versión](#adaptarlo-a-otra-versión).

## Instalar / quitar

1. Descarga `letssing2026_saltar_intro_v4.zip` de [Releases](../../releases).
2. Descomprímelo en la raíz de la SD. Quedan estos dos archivos:
   ```
   sd:/atmosphere/contents/0100EE4020D18000/exefs/subsdk9
   sd:/atmosphere/contents/0100EE4020D18000/exefs/main.npdm
   ```
3. Abre el juego normalmente (con Atmosphère). Si estaba abierto, ciérralo del todo y vuelve a abrirlo: el mod se
   carga al arrancar el juego.

Para actualizar desde una versión anterior del mod basta con cambiar `subsdk9`; `main.npdm` es el mismo en todas.

Para quitarlo, borra la carpeta `sd:/atmosphere/contents/0100EE4020D18000`. No toca las canciones (el parche de
canciones va en la carpeta del DLC) ni la partida guardada.

### Por qué lleva `main.npdm`

`main.npdm` es el del juego (v131072) con **solo 4 llamadas al sistema añadidas**: `0x40`, `0x43`, `0x74` y `0x75`.
exlaunch las necesita para instalar sus ganchos. Sin ellas, el juego se cierra al abrirse con "Undefined System Call
0x74" en `atmosphere/crash_reports`. El resto de permisos son idénticos al original (comparado campo a campo).

## Compilar

Necesitas Linux (o WSL) con `git`, `make`, `python3` y [devkitPro](https://devkitpro.org/wiki/Getting_Started)
(devkitA64 y `switch-tools`, que trae `npdmtool`).

```
./build.sh
```

El script descarga [exlaunch](https://github.com/shadowninja108/exlaunch) en `build/` y lo fija en el commit
`f9f4b0d`. Después aplica `exlaunch.patch`, copia `source/main.cpp`, compila y genera
`dist/letssing2026_saltar_intro.zip` con `subsdk9` y `main.npdm`. Variables opcionales:

| Variable | Por defecto | Para qué |
|---|---|---|
| `DEVKITPRO` | `/opt/devkitpro` | dónde está devkitPro |
| `EXLAUNCH_URL` | el repositorio de exlaunch | usar un clon local de exlaunch, sin descargarlo |

La ruta de la carpeta no puede tener apóstrofes: el Makefile de exlaunch falla con ellos.

`exlaunch.patch` cambia tres cosas: el ID del juego en `config.mk`; un aviso de gcc 16 que no debe ser error; y el
símbolo `g_LookupGlobalManualFunctionPointer`, que pasa a débil porque el SDK de este juego no lo exporta.

### Generar `main.npdm` desde tu propio volcado

```
python3 tools/npdm2json.py "<exefs de la actualización>/main.npdm" > juego.json
# añadir "svc40", "svc43", "svc74" y "svc75" a "syscalls" (npdm/main_npdm.json ya los trae)
npdmtool juego.json main.npdm
```

Otra opción es `python3 tools/npdm_svc.py juego.npdm main.npdm 0x40,0x43,0x74,0x75`. Activa esas llamadas
directamente en el archivo binario y no toca nada más.

## Adaptarlo a otra versión

Los nombres de las funciones no cambian entre versiones, pero sus direcciones sí:

1. Saca `exefs/main` y `global-metadata.dat` de la versión nueva (nxdumptool en la consola, con la actualización
   instalada) y pasa los dos por [Il2CppDumper](https://github.com/Perfare/Il2CppDumper).
2. En `dump.cs`, busca las direcciones (RVA) de las funciones del bloque `offsets` de `source/main.cpp`:
   `InGameClassic.UpdateVx`, `InGameSimple.UpdateVx`, `InGameSimple.SetTime`, `KaraokeManager.get_Instance` y
   `KaraokeManager.get_KTime`. Comprueba también los campos (`InGame.songEnd`, `InGame.karaokeTime`,
   `KaraokeManager.KaraokeSong`, `KaraokeSong.KaraokeNotes`, `KaraokeSong.KaraokeNotesAll`,
   `KaraokeObject.StartTime`, `KaraokeObject.EndTime`).
   Para la canción aleatoria: `SongsScreen.UpdateVx`, los `get_Instance` de `GameManager`, `SortManager`,
   `SuggestionsManager` y `PlaylistManager`, `GameSong.get_IsLocked` y `get_CanRetrieveData`,
   `GridSongCover.get_Song`, los campos de `SongsScreen`, `SongsGrid`, `GridSongCategory` y `BaseHubCategory`, los
   puestos de `set_CurrentGameSong` y `GoTo` en la tabla virtual de `GameManager` (`Slot` en `dump.cs`: 0x138 +
   16 × puesto) y, desensamblando, la casilla del texto `"PlayerSelection4P"`, la función que inicializa esas
   casillas y la barrera de escritura (las tres se ven en `SongsScreen.UpdateVx`, donde el juego elige la canción).
3. Rehaz `main.npdm` con el de la versión nueva (ver arriba). Actualiza el build ID y la versión en este README.

## Pendiente

- Que el video salte directo: ahora, tras el salto, se pone al día en un momento (se ve avanzar rápido) y luego sigue
  sincronizado.

## Versiones del mod

| Versión | Juego | Cambios |
|---|---|---|
| v1 (2026-11-01) | Let's Sing 2026 v131072 | Primera versión: ZR/ZL durante el intro → 3 s antes de la primera nota |
| v2 (2026-11-12) | Let's Sing 2026 v131072 | También en Feat. Mantener ZR/ZL 3 s después de la última nota → 1 s antes del final (resultados y puntuación como siempre) |
| v3 (2026-11-15) | Let's Sing 2026 v131072 | ZL+ZR en la lista de canciones → canción al azar de la fila donde está el cursor |
| v4 (2026-11-15) | Let's Sing 2026 v131072 | La canción al azar sale de la fila actual en Recomendadas, Idioma, Año, Tema y Género, y de toda la lista en Título, Artista y Favoritas. Al repetir con B y ZL+ZR sigue en la misma fila aunque el juego mueva el cursor |

## Licencia

GPL-2.0, igual que exlaunch, en el que se basa (ver `LICENSE`).

---

**English.** Let's Sing 2026 (Switch, Atmosphère) mod, all with ZL/ZR. **Random song:** in the song list, press ZL
and ZR together to start a random song as if it had been chosen with A; B comes back to the list and ZL+ZR gives
another one. It is taken from the row the cursor is in when the list is ordered by Recommended, Language, Year,
Theme or Genre, and from the whole list in Title, Artist and Favorites. **Skip the intro:** press ZR/ZL (SL/SR on a
single Joy-Con) during a song's intro to jump to 3 s before the first note. **Skip the end:** hold it for 3 s after
the last note to jump to 1 s before the end (results and score as usual). Classic and Feat modes; never online
(Fest) or Career. **Only for update v131072 (version 0.2.0), build ID `1C344DD17322D4FA3E8F50063340C7D0`**. Install:
unzip the release to the root of the SD card. Remove: delete `atmosphere/contents/0100EE4020D18000`. Build:
`./build.sh` (devkitPro). GPL-2.0.
