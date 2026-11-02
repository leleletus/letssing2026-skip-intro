# Let's Sing 2026: saltar el intro de las canciones

Mod para **Let's Sing 2026** (Nintendo Switch, Atmosphère). En una partida normal, durante el intro de una canción,
**ZR o ZL** (SL/SR en un Joy-Con suelto) salta a 3 s antes de la primera nota. Usa la función del propio juego para
moverse en la canción (`InGameSimple.SetTime`): audio, video y letra saltan juntos.

- Solo funciona si faltan más de 6 s para la primera nota (en canciones con intro corto no hace nada).
- Solo en la partida normal (`InGameClassic`). Nunca en los modos online (Fest), Feat ni Carrera.
- Se puede volver a usar al reintentar la canción. Un botón que ya estaba pulsado al empezar la canción no cuenta.
- Funciona igual con las canciones oficiales, las de los DLC y las añadidas con
  [Let's Sing UltraStar Converter](https://github.com/leleletus/ultrastar2singIt).

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

Probado en una consola real con Atmosphère el 2026-11-01.

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

1. Descarga `letssing2026_saltar_intro_v1.zip` de [Releases](../../releases).
2. Descomprímelo en la raíz de la SD. Quedan estos dos archivos:
   ```
   sd:/atmosphere/contents/0100EE4020D18000/exefs/subsdk9
   sd:/atmosphere/contents/0100EE4020D18000/exefs/main.npdm
   ```
3. Abre el juego normalmente (con Atmosphère).

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
   `InGameClassic.UpdateVx`, `InGameSimple.SetTime`, `KaraokeManager.get_Instance` y `KaraokeManager.get_KTime`.
   Comprueba también los campos (`KaraokeManager.KaraokeSong`, `KaraokeSong.KaraokeNotes`,
   `KaraokeObject.StartTime`).
3. Rehaz `main.npdm` con el de la versión nueva (ver arriba). Actualiza el build ID y la versión en este README.

## Pendiente

- Que el video salte directo: ahora, tras el salto, se pone al día en un momento (se ve avanzar rápido) y luego sigue
  sincronizado.
- Terminar la canción al acabar la letra, para canciones cuyo video sigue mucho tiempo después de la última línea.

## Versiones del mod

| Versión | Juego | Cambios |
|---|---|---|
| v1 (2026-11-01) | Let's Sing 2026 v131072 | Primera versión: ZR/ZL durante el intro → 3 s antes de la primera nota |

## Licencia

GPL-2.0, igual que exlaunch, en el que se basa (ver `LICENSE`).

---

**English.** Let's Sing 2026 (Switch, Atmosphère) mod: press ZR/ZL (SL/SR on a single Joy-Con) during a song's intro
to jump to 3 s before the first note (normal mode only). **Only for update v131072 (version 0.2.0), build ID
`1C344DD17322D4FA3E8F50063340C7D0`**. Install: unzip the release to the root of the SD card. Remove: delete
`atmosphere/contents/0100EE4020D18000`. Build: `./build.sh` (devkitPro). GPL-2.0.
