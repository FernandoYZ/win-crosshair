# Proyecto: Crosshair Overlay minimalista para Windows

## Objetivo

Crear una aplicación nativa para Windows llamada:

```text
crosshair.exe
```

Su única responsabilidad es mostrar una mira (`crosshair`) estática en el centro de la pantalla como un overlay transparente sobre otras aplicaciones y videojuegos.

El proyecto debe priorizar:

1. Mínimo consumo de RAM.
2. Mínimo consumo de CPU cuando la mira está estática.
3. Binario pequeño.
4. Cero interfaz gráfica innecesaria.
5. Cero dependencias externas en tiempo de ejecución.
6. No interactuar con el proceso del juego.
7. No inyectar DLLs.
8. No utilizar hooks del juego.
9. No leer ni modificar memoria del juego.
10. Ser portable: ejecutar `crosshair.exe` directamente.
11. Configuración mediante `config.toml`.
12. Código sencillo, mantenible y correctamente separado.
13. Compatibilidad con la mayor cantidad posible de juegos y modos de ventana de Windows.

El producto final debe ser simplemente:

```text
crosshair.exe
config.toml
```

No crear instalador, servicio de Windows, launcher, tray icon ni aplicación de configuración.

---

# 1. Lenguaje y tecnologías

Utilizar:

* C
* Win32 API nativa de Windows
* GDI inicialmente para renderizar la mira
* CMake para el build system
* TOML para configuración

No utilizar:

* C++
* Rust
* Go
* .NET
* C#
* Qt
* GTK
* SDL
* GLFW
* OpenGL
* DirectX 11/12 salvo que posteriormente exista una justificación técnica clara
* Dear ImGui
* Electron
* WebView
* Python
* Java
* Node.js

La aplicación debe ser lo más cercana posible a una aplicación Win32 nativa mínima.

---

# 2. Filosofía del proyecto

La aplicación debe seguir una filosofía similar a una herramienta Unix minimalista:

```text
crosshair.exe
```

Ejecutarla y obtener inmediatamente la mira.

No debe existir una ventana visible tradicional.

No debe aparecer:

* ventana de configuración;
* splash screen;
* diálogo;
* menú;
* tray icon;
* notificaciones;
* publicidad;
* telemetría;
* conexión a Internet.

El programa solamente debe:

```text
leer configuración
        ?
crear overlay
        ?
dibujar crosshair
        ?
mantenerlo visible
        ?
esperar
```

No implementar funcionalidades que no sean necesarias para este objetivo.

---

# 3. Arquitectura

Utilizar una estructura similar a:

```text
crosshair/
¦
+-- src/
¦   +-- main.c
¦   +-- main.h
¦   ¦
¦   +-- overlay.c
¦   +-- overlay.h
¦   ¦
¦   +-- config.c
¦   +-- config.h
¦
+-- config.toml
+-- CMakeLists.txt
+-- README.md
+-- LICENSE
+-- .gitignore
```

Si una separación adicional mejora claramente la arquitectura, puede hacerse, pero evitar sobrearquitectura.

No crear decenas de archivos para una aplicación tan pequeña.

---

# 4. Overlay Win32

Implementar el overlay utilizando una ventana Win32.

La ventana debe:

* ser invisible como ventana convencional;
* no tener bordes;
* no tener título;
* no aparecer en Alt+Tab;
* no recibir foco;
* no activar la aplicación;
* permanecer encima de las ventanas normales;
* permitir interacción con el juego debajo;
* ser transparente fuera de la mira;
* permanecer centrada en el monitor seleccionado.

Investigar y utilizar correctamente los estilos Win32 necesarios, incluyendo cuando corresponda:

```c
WS_EX_LAYERED
WS_EX_TRANSPARENT
WS_EX_NOACTIVATE
WS_EX_TOOLWINDOW
```

y el comportamiento `TOPMOST`.

Evaluar correctamente el uso de:

```c
WS_EX_NOREDIRECTIONBITMAP
```

solo si aporta algún beneficio real y es compatible con la estrategia de rendering seleccionada.

No agregar estilos simplemente por costumbre.

---

# 5. Click-through

El overlay debe ser completamente `click-through`.

Los clics del mouse deben pasar hacia la aplicación que está debajo.

La mira no debe bloquear:

* mouse;
* teclado;
* interacción con el juego;
* cambio de ventanas.

La ventana tampoco debe convertirse en la ventana activa.

Investigar correctamente la interacción entre:

```text
WS_EX_TRANSPARENT
WS_EX_NOACTIVATE
WM_NCHITTEST
```

y utilizar la solución técnicamente más apropiada.

Por ejemplo, evaluar devolver:

```c
HTTRANSPARENT
```

desde `WM_NCHITTEST` si corresponde.

---

# 6. Always-on-top

La mira debe permanecer sobre otras ventanas mediante una ventana topmost.

Utilizar Win32 correctamente para esto.

No implementar un bucle que esté llamando constantemente a `SetWindowPos()`.

La aplicación no debe estar haciendo polling continuo simplemente para mantener la ventana encima.

La posición y el estado de la ventana deben actualizarse solamente cuando sea necesario.

---

# 7. Posición

Por defecto, la mira debe aparecer exactamente en el centro del monitor principal.

Obtener dinámicamente:

```text
monitor width
monitor height
```

y calcular:

```text
centerX = monitorX + monitorWidth / 2
centerY = monitorY + monitorHeight / 2
```

No asumir:

```text
1920x1080
```

ni ninguna resolución concreta.

Debe funcionar con:

* 1280×720
* 1920×1080
* 2560×1440
* 3840×2160
* etc.

También considerar correctamente DPI scaling y coordenadas de escritorio cuando sea necesario.

---

# 8. Multi-monitor

El diseño debe permitir seleccionar el monitor mediante configuración.

Ejemplo:

```toml
[display]
monitor = 0
```

Donde:

```text
0 = monitor principal
1 = segundo monitor
2 = tercer monitor
...
```

Si el monitor especificado no existe:

* mostrar un error claro;
* hacer fallback al monitor principal;
* no terminar abruptamente.

Si implementar selección de monitor aumenta demasiado la complejidad del MVP, primero implementar correctamente el monitor principal y dejar la arquitectura preparada para añadirlo.

---

# 9. Rendering

Para la primera implementación utilizar GDI.

No utilizar DirectX ni Direct2D inicialmente.

La mira debe estar compuesta por primitivas simples.

Por ejemplo:

```text
      ¦
      ¦
      ¦
------+------
      ¦
      ¦
      ¦
```

Implementar como mínimo:

* línea vertical;
* línea horizontal.

La mira debe poder configurarse mediante:

* tamaño;
* grosor;
* separación central;
* color;
* opacidad;
* outline opcional.

---

# 10. Tipos de mira

Implementar inicialmente:

```toml
shape = "cross"
```

Como mínimo soportar:

```text
cross
```

La arquitectura debe permitir posteriormente agregar:

```text
dot
circle
cross_dot
t_shape
custom
```

pero NO implementar todos estos tipos en el MVP si no son necesarios.

Priorizar una implementación sólida de `cross`.

---

# 11. Configuración

Utilizar:

```text
config.toml
```

Ejemplo:

```toml
[crosshair]
shape = "cross"

color = "#00FF00"

size = 12
thickness = 2
gap = 4

opacity = 255

outline = true
outline_color = "#000000"
outline_thickness = 1

[display]
monitor = 0
```

La configuración debe ser fácil de leer y modificar manualmente.

---

# 12. Parser TOML

Utilizar una implementación de TOML adecuada para C solamente si esto no introduce una dependencia excesivamente pesada.

Evaluar cuidadosamente las opciones.

Prioridad:

1. simplicidad;
2. seguridad;
3. estabilidad;
4. tamaño;
5. facilidad de compilación.

No incluir un parser TOML gigantesco si únicamente se necesitan aproximadamente 10 propiedades.

Si una biblioteca TOML de C introduce demasiado peso o complejidad, considerar implementar un parser mínimo y explícitamente limitado para el formato de configuración utilizado.

En ese caso:

* documentar claramente qué sintaxis se soporta;
* rechazar valores inválidos;
* no intentar implementar TOML completo;
* no llamar a ese parser "TOML completo".

La configuración del usuario debe poder contener comentarios si el parser utilizado lo permite.

---

# 13. Validación de configuración

Validar todos los parámetros.

Ejemplos:

```text
size > 0
thickness > 0
gap >= 0
opacity entre 0 y 255
outline_thickness >= 0
```

Validar colores:

```text
#RRGGBB
#RRGGBBAA
```

Si un valor es inválido:

* mostrar un mensaje de error;
* explicar qué propiedad es incorrecta;
* terminar limpiamente.

No continuar con valores potencialmente peligrosos o inconsistentes.

---

# 14. Color

Permitir:

```toml
color = "#00FF00"
```

y convertirlo correctamente a:

```text
R
G
B
```

La transparencia debe poder configurarse independientemente mediante:

```toml
opacity = 255
```

Si se utiliza `#RRGGBBAA`, definir claramente cómo interactúa con `opacity`.

No permitir ambigüedad entre ambos sistemas.

---

# 15. Outline

Implementar opcionalmente:

```toml
outline = true
outline_color = "#000000"
outline_thickness = 1
```

El outline existe para mejorar la visibilidad de la mira sobre fondos claros.

La implementación debe evitar artefactos visibles.

---

# 16. Anti-aliasing

La primera versión debe priorizar simplicidad.

Determinar si GDI produce suficiente calidad visual para:

* líneas de 1 px;
* líneas de 2 px;
* diferentes resoluciones;
* diferentes escalas DPI.

Si GDI resulta suficiente, mantener GDI.

No introducir Direct2D solamente por motivos teóricos.

Si posteriormente se demuestra que GDI produce resultados visuales deficientes, documentar el problema y evaluar Direct2D como segunda implementación.

---

# 17. Fullscreen

El programa debe ser un overlay externo y genérico.

No asumir que todos los juegos utilizan el mismo modo de presentación.

Debe funcionar cuando sea técnicamente posible en:

* Windowed;
* Borderless Windowed;
* Fullscreen Windowed.

Respecto a Fullscreen Exclusive:

No implementar hacks, DLL injection ni modificaciones del juego para forzar compatibilidad.

Documentar claramente que un overlay Win32 externo puede no aparecer sobre aplicaciones que utilizan fullscreen exclusivo dependiendo de cómo el juego y Windows presenten los frames.

Esto debe considerarse una limitación técnica, no un bug que deba solucionarse mediante técnicas invasivas.

---

# 18. No interactuar con videojuegos

La aplicación debe ser completamente independiente del juego.

No:

* abrir procesos de juegos;
* buscar procesos por nombre;
* leer memoria;
* escribir memoria;
* inyectar DLL;
* instalar hooks;
* modificar archivos del juego;
* interceptar DirectX;
* interceptar OpenGL;
* interceptar Vulkan;
* detectar anti-cheat;
* intentar evadir anti-cheat.

El overlay solamente debe existir como una ventana independiente de Windows.

---

# 19. Rendimiento

El objetivo es minimizar el consumo.

Una vez creada y dibujada la mira, no debe existir un loop como:

```c
while (running) {
    redraw();
}
```

a alta frecuencia.

No utilizar:

```text
Sleep(1)
Sleep(10)
Sleep(16)
```

para crear un falso frame loop.

El programa debe utilizar el message loop de Windows:

```c
GetMessage()
TranslateMessage()
DispatchMessage()
```

y redibujar solamente cuando sea necesario.

La mira es estática, por lo tanto no existe razón para renderizarla 60/144/240 veces por segundo.

---

# 20. Uso de CPU

En estado idle:

```text
CPU ˜ 0%
```

o tan bajo como razonablemente permita Windows.

No utilizar polling innecesario.

No utilizar timers periódicos salvo que exista una necesidad técnica real.

---

# 21. Uso de memoria

Intentar mantener el consumo de memoria extremadamente bajo.

No utilizar:

* frameworks GUI;
* motores gráficos;
* runtime externo;
* bases de datos;
* networking;
* logging permanente.

El objetivo es que el proceso sea comparable en filosofía a herramientas pequeñas como `gocrosshair`.

No establecer una cifra artificial como "exactamente 3 MB", ya que Windows administra memoria del proceso y las métricas dependen de la versión de Windows y del método de medición.

En su lugar, optimizar razonablemente y medir.

---

# 22. DPI awareness

La aplicación debe declararse correctamente DPI-aware.

Investigar y utilizar:

```text
Per-Monitor DPI Awareness
```

cuando corresponda.

Evitar que Windows escale artificialmente la posición o el tamaño de la mira.

La mira debe permanecer visualmente centrada y con un tamaño coherente independientemente de:

* DPI scaling;
* resolución;
* monitor.

---

# 23. Cierre

Implementar un mecanismo limpio de cierre.

Por ejemplo:

```text
Ctrl+C
```

desde la consola si se ejecuta desde terminal.

También debe poder cerrarse desde el Administrador de tareas.

No es necesario implementar:

* tray icon;
* botón de cierre;
* ventana de configuración.

---

# 24. Consola

Decidir cuidadosamente si el ejecutable final debe ser:

```text
SUBSYSTEM:WINDOWS
```

para evitar mostrar una consola.

Durante desarrollo debe existir una opción que facilite debugging.

Por ejemplo:

```text
crosshair-debug.exe
```

o una configuración CMake para Debug.

El release final debe generar:

```text
crosshair.exe
```

sin una consola visible.

No utilizar logging constante en producción.

---

# 25. Errores

Los errores de inicialización deben ser claros.

Ejemplos:

```text
Failed to create overlay window.
Failed to initialize rendering.
Invalid configuration.
Monitor not found.
Invalid color.
Invalid crosshair size.
```

Durante desarrollo se puede utilizar:

```text
OutputDebugString()
```

y/o mensajes de error Win32.

El release debe evitar generar archivos de log innecesarios.

---

# 26. Build

Crear un `CMakeLists.txt` limpio.

Debe ser posible construir con:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

El resultado debe ser:

```text
crosshair.exe
```

Idealmente soportar tanto:

```text
MSVC
```

como:

```text
MinGW-w64
```

si esto no complica significativamente el proyecto.

Priorizar primero una toolchain estable y documentar la utilizada.

---

# 27. Optimización del ejecutable

Para Release:

* habilitar optimización;
* eliminar símbolos de debugging;
* evitar dependencias innecesarias;
* evitar CRT dinámica si una configuración estática resulta apropiada;
* evaluar tamaño del ejecutable.

No realizar optimizaciones que reduzcan significativamente la mantenibilidad.

El objetivo no es ganar un benchmark de tamaño a cualquier precio.

---

# 28. Arquitectura de código

Separar responsabilidades.

### `main.c`

Responsable de:

* inicialización;
* cargar configuración;
* inicializar overlay;
* message loop;
* shutdown.

### `config.c`

Responsable de:

* leer `config.toml`;
* parsear propiedades;
* validar configuración;
* proporcionar defaults.

### `overlay.c`

Responsable de:

* registrar clase Win32;
* crear ventana;
* configurar estilos;
* posicionar ventana;
* crear superficie de rendering;
* dibujar crosshair;
* gestionar mensajes Win32.

No mezclar parsing TOML con código Win32.

No colocar toda la aplicación en `main.c`.

---

# 29. Configuración por defecto

Si no existe:

```text
config.toml
```

la aplicación debe utilizar valores por defecto razonables.

Por ejemplo:

```text
shape       = cross
color       = #00FF00
size        = 12
thickness   = 2
gap         = 4
opacity     = 255
outline     = true
outlineColor = #000000
outlineThickness = 1
monitor     = 0
```

No debe ser obligatorio crear manualmente el archivo para poder ejecutar:

```powershell
crosshair.exe
```

La aplicación debe funcionar inmediatamente con defaults.

---

# 30. Localización del archivo

Buscar `config.toml` de manera determinista.

Preferiblemente:

```text
directorio donde está crosshair.exe/config.toml
```

Esto permite que la aplicación sea portable:

```text
C:\Tools\Crosshair\
    crosshair.exe
    config.toml
```

No utilizar AppData para la configuración en la primera versión.

No escribir archivos automáticamente en:

```text
C:\Windows
```

ni en el registro.

---

# 31. Compatibilidad

Objetivo:

```text
Windows 10
Windows 11
```

64-bit como plataforma principal.

No implementar soporte 32-bit salvo que sea trivial.

No asumir una versión concreta de Windows 11.

---

# 32. Seguridad

No solicitar privilegios de administrador.

No utilizar:

```text
requireAdministrator
```

en el manifest.

La aplicación debe ejecutarse con permisos normales.

No modificar:

```text
HKLM
HKCU
```

No instalar servicios.

No instalar drivers.

No ejecutar código remoto.

No realizar conexiones de red.

---

# 33. Anti-cheat y compatibilidad

El proyecto debe permanecer estrictamente como overlay externo.

No intentar ocultarse de anti-cheat.

No intentar evadir mecanismos de detección.

No inyectarse en el proceso del juego.

No acceder a memoria del juego.

Documentar que diferentes juegos pueden tener diferentes políticas respecto a overlays externos.

El objetivo es proporcionar una herramienta gráfica genérica, no modificar el comportamiento del juego.

---

# 34. README

Crear un README claro que explique:

## Qué es

Un overlay de mira extremadamente minimalista para Windows.

## Uso

```powershell
crosshair.exe
```

## Configuración

Mostrar ejemplo:

```toml
[crosshair]
shape = "cross"
color = "#00FF00"
size = 12
thickness = 2
gap = 4
opacity = 255

outline = true
outline_color = "#000000"
outline_thickness = 1

[display]
monitor = 0
```

## Limitaciones

Explicar:

* fullscreen exclusivo puede no ser compatible;
* borderless/windowed es el escenario recomendado;
* algunos juegos pueden impedir overlays externos;
* la herramienta no modifica ni inyecta código en juegos.

## Rendimiento

Documentar metodología de medición.

No inventar cifras.

Medir realmente:

* RAM;
* CPU idle;
* tamaño del `.exe`.

---

# 35. Testing

Crear tests donde tengan sentido.

Como mínimo probar:

### Configuración

* configuración inexistente;
* configuración válida;
* color inválido;
* tamaño inválido;
* grosor inválido;
* opacity inválida;
* monitor inexistente;
* propiedades desconocidas.

### Rendering

Verificar:

* mira centrada;
* grosor correcto;
* gap correcto;
* tamaño correcto;
* color correcto;
* outline correcto.

### Window behavior

Verificar:

* topmost;
* click-through;
* no activate;
* no Alt+Tab;
* transparencia.

No crear una suite de testing excesivamente compleja para un proyecto tan pequeño.

---

# 36. Criterios de aceptación

El proyecto se considera terminado cuando:

1. `crosshair.exe` compila correctamente en Release.
2. Se ejecuta sin instalación.
3. No requiere DLLs externas adicionales.
4. No requiere administrador.
5. No muestra una ventana convencional.
6. Aparece una mira en el centro del monitor.
7. La mira es click-through.
8. La mira no roba el foco.
9. La mira permanece topmost.
10. La mira utiliza configuración desde `config.toml`.
11. Funciona sin `config.toml` utilizando defaults.
12. Cambiar el color requiere únicamente editar `config.toml` y reiniciar el programa.
13. No existe un render loop innecesario.
14. El consumo de CPU en idle es prácticamente nulo.
15. El consumo de memoria es razonablemente bajo.
16. No existe networking.
17. No existe interacción con procesos de videojuegos.
18. No existe DLL injection.
19. No existe modificación de memoria.
20. El proyecto está documentado.
21. El código es C nativo y utiliza Win32.
22. El ejecutable final se llama exactamente:

```text
crosshair.exe
```

---

# 37. Prioridad de implementación

Implementar en este orden:

### Fase 1 — Prototipo

Crear una ventana Win32 transparente y click-through.

Dibujar una cruz verde fija.

No implementar TOML todavía.

Objetivo:

```text
crosshair.exe
```

? aparece una cruz.

### Fase 2 — Posicionamiento

Implementar:

* centro del monitor;
* DPI awareness;
* topmost;
* multi-monitor básico.

### Fase 3 — Configuración

Agregar:

```text
config.toml
```

Implementar:

* color;
* size;
* thickness;
* gap;
* opacity.

### Fase 4 — Rendering

Agregar:

* outline;
* diferentes tamaños;
* validación de parámetros.

### Fase 5 — Optimización

Medir:

* RAM;
* CPU;
* tamaño del ejecutable.

Eliminar cualquier componente innecesario.

### Fase 6 — Testing

Probar:

* escritorio;
* ventana;
* borderless;
* fullscreen;
* múltiples monitores;
* diferentes resoluciones;
* diferentes DPI.

### Fase 7 — Release

Generar:

```text
crosshair.exe
config.toml
README.md
```

---

# 38. Restricción importante

No sobreingenierizar el proyecto.

Este proyecto tiene deliberadamente un alcance pequeño.

Si una característica no es necesaria para:

> "mostrar una mira estática configurable sobre otras aplicaciones"

no implementarla.

Evitar crear:

```text
plugin systems
dependency injection
event buses
configuration frameworks
GUI frameworks
database layers
logging frameworks
network layers
```

La simplicidad es una característica del producto.

---

# 39. Resultado esperado

El resultado final debe sentirse como una herramienta de sistema pequeña:

```text
crosshair/
+-- crosshair.exe
+-- config.toml
```

Ejecutar:

```powershell
.\crosshair.exe
```

y obtener inmediatamente una mira.

Modificar:

```toml
color = "#FF0000"
```

cerrar y volver a ejecutar:

```powershell
.\crosshair.exe
```

y obtener una mira roja.

No debe existir ninguna otra interacción necesaria.

---

# 40. Antes de implementar

Primero analiza técnicamente:

1. Qué combinación de estilos Win32 es necesaria para obtener:

   * transparencia;
   * click-through;
   * no activation;
   * topmost;
   * ausencia de Alt+Tab.

2. Qué técnica de rendering GDI es más apropiada.

3. Cómo implementar correctamente una ventana `WS_EX_LAYERED`.

4. Cómo evitar redibujados innecesarios.

5. Cómo gestionar DPI y múltiples monitores.

6. Cómo obtener el centro físico/lógico correcto del monitor.

7. Qué estrategia TOML resulta más adecuada para un proyecto C tan pequeño.

8. Cómo generar un Release realmente portable.

Después de ese análisis, implementar el MVP.

No introducir Direct2D, DirectX ni otras tecnologías hasta demostrar que GDI no satisface los requisitos.

Al finalizar, explicar brevemente:

* arquitectura utilizada;
* decisiones técnicas importantes;
* cómo compilar;
* cómo ejecutar;
* cómo configurar;
* limitaciones conocidas;
* tamaño del ejecutable generado;
* consumo de RAM/CPU medido, si es posible.

El objetivo no es construir un competidor completo de HudSight.

El objetivo es construir:

> **el crosshair overlay nativo para Windows más simple y eficiente que sea razonablemente posible, manteniendo el código limpio y mantenible.**
