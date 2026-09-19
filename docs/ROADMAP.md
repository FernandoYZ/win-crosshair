# Crosshair Overlay — Roadmap

## 1. Propósito

Este documento define la evolución planificada de `crosshair`, un overlay de mira extremadamente minimalista para Windows.

El objetivo principal del proyecto es proporcionar una herramienta que permita mostrar una mira configurable sobre aplicaciones y videojuegos sin necesidad de una interfaz gráfica, servicios, instalación, integración con juegos ni componentes innecesarios.

El producto final de la primera versión estable debe poder reducirse conceptualmente a:

```text
crosshair.exe
config.toml
```

Ejecutar:

```powershell
.\crosshair.exe
```

debe ser suficiente para mostrar la mira.

---

# 2. Principios del proyecto

Todo el desarrollo debe respetar los siguientes principios.

## 2.1 Minimalismo

La aplicación debe hacer una cosa:

> Mostrar una mira como overlay.

No se deben añadir funcionalidades simplemente porque sean técnicamente posibles.

Cada nueva funcionalidad debe justificar:

* utilidad real;
* impacto en complejidad;
* impacto en rendimiento;
* impacto en tamaño;
* mantenimiento adicional.

---

## 2.2 Rendimiento

El proyecto debe priorizar:

* bajo consumo de RAM;
* consumo de CPU prácticamente nulo en estado idle;
* bajo consumo de GPU;
* ejecutable pequeño;
* ausencia de polling innecesario;
* ausencia de render loops constantes.

La mira es estática.

Por lo tanto, no existe ninguna razón para renderizarla continuamente a 60, 144 o 240 FPS.

---

## 2.3 Portabilidad

El programa debe ser portable.

Idealmente:

```text
crosshair/
+-- crosshair.exe
+-- config.toml
```

Sin instalación obligatoria.

Sin servicios.

Sin drivers.

Sin registro de Windows.

Sin AppData obligatorio.

Sin conexión a Internet.

---

## 2.4 Independencia del juego

El proyecto debe ser completamente independiente del videojuego que se esté ejecutando.

No debe:

* leer procesos;
* leer memoria;
* escribir memoria;
* inyectar DLL;
* modificar archivos del juego;
* interceptar funciones del juego;
* implementar bypass de anti-cheat;
* detectar automáticamente juegos.

La aplicación debe funcionar como un overlay externo de Windows.

---

## 2.5 Evolución controlada

Las funcionalidades deben introducirse progresivamente.

No intentar implementar toda la visión del proyecto desde la primera versión.

La primera versión debe resolver correctamente el problema fundamental:

```text
mostrar una crosshair
```

Las funcionalidades adicionales deben llegar mediante releases posteriores.

---

# 3. Estrategia de versiones

El proyecto utilizará aproximadamente las siguientes etapas:

```text
v0.1.0  Minimal Crosshair
v0.2.0  Real Overlay
v0.3.0  Configuration
v0.4.0  Visual Quality
v0.5.0  Robustness
v0.6.0  Optimization
v1.0.0  Stable
v1.x    Maintenance
v2.0.0  Crosshair Expansion
v2.x    Advanced Crosshair
v3.0.0  Profiles
v3.x    Optional Advanced Features
```

No todas las versiones futuras tienen que implementarse obligatoriamente.

El roadmap representa una dirección de desarrollo, no una obligación de agregar funcionalidades innecesarias.

---

# 4. v0.1.0 — Minimal Crosshair

## Objetivo

Crear el primer prototipo funcional.

Esta versión debe responder únicamente a:

> ¿Podemos crear una mira externa utilizando C + Win32 que aparezca sobre el escritorio?

---

## Funcionalidad

Implementar:

* C;
* Win32 API;
* GDI;
* ventana sin bordes;
* ventana transparente;
* cruz fija;
* color fijo;
* tamaño fijo;
* posición en el centro del monitor principal.

Ejemplo:

```text
       ¦
       ¦
-------+-------
       ¦
       ¦
```

---

## No implementar

En esta versión NO deben existir:

* `config.toml`;
* multi-monitor;
* outline;
* DPI avanzado;
* múltiples formas;
* hotkeys;
* perfiles;
* GUI;
* tray icon;
* Direct2D;
* DirectX;
* networking.

---

## Criterio de finalización

Debe ser posible ejecutar:

```powershell
.\crosshair.exe
```

y visualizar una cruz.

Esta versión es un prototipo técnico.

No pretende ser todavía una versión distribuible final.

---

# 5. v0.2.0 — Real Overlay

## Objetivo

Convertir el prototipo en un overlay real de Windows.

---

## Funcionalidad

Implementar correctamente:

* `WS_EX_LAYERED`;
* `WS_EX_TRANSPARENT`;
* `WS_EX_NOACTIVATE`;
* `WS_EX_TOOLWINDOW`;
* comportamiento TOPMOST;
* click-through;
* ausencia de foco;
* ausencia de Alt+Tab;
* fondo transparente.

La mira debe permanecer sobre las aplicaciones compatibles sin impedir la interacción con ellas.

---

## Comportamiento esperado

Al ejecutar:

```powershell
.\crosshair.exe
```

debe ocurrir:

```text
crosshair.exe
     ¦
     ?
overlay transparente
     ¦
     ?
crosshair visible
     ¦
     +-- mouse atraviesa
     +-- teclado no es capturado
     +-- aplicación inferior mantiene foco
     +-- overlay permanece encima
```

---

## Compatibilidad inicial

Probar:

* escritorio;
* aplicaciones normales;
* ventanas;
* juegos Windowed;
* juegos Borderless.

También probar un juego real como L4D2, pero sin introducir ninguna lógica específica para ese juego.

---

# 6. v0.3.0 — Configuration

## Objetivo

Eliminar los valores hardcodeados y permitir personalización mediante `config.toml`.

---

## Archivo

Crear:

```text
config.toml
```

Ejemplo:

```toml
[crosshair]
color = "#00FF00"
size = 12
thickness = 2
gap = 4
opacity = 255
```

---

## Propiedades

La primera versión configurable debe soportar:

| Propiedad   | Función                       |
| ----------- | ----------------------------- |
| `color`     | Color de la mira              |
| `size`      | Longitud de los segmentos     |
| `thickness` | Grosor                        |
| `gap`       | Separación respecto al centro |
| `opacity`   | Transparencia                 |

---

## Defaults

El programa debe funcionar incluso si:

```text
config.toml
```

no existe.

Debe utilizar valores predeterminados.

Por ejemplo:

```text
color     = #00FF00
size      = 12
thickness = 2
gap       = 4
opacity   = 255
```

---

## Restricción

La única forma de mira sigue siendo:

```text
cross
```

No agregar:

* dot;
* circle;
* T-shape;
* custom image;
* cross + dot.

Eso se reservará para releases posteriores.

---

# 7. v0.4.0 — Visual Quality

## Objetivo

Mejorar la calidad visual y la compatibilidad con diferentes configuraciones de pantalla.

---

## 7.1 Outline

Agregar:

```toml
outline = true
outline_color = "#000000"
outline_thickness = 1
```

El outline debe mejorar la visibilidad de la mira sobre fondos claros.

---

## 7.2 DPI awareness

Implementar correctamente DPI awareness.

Probar:

```text
100%
125%
150%
175%
200%
```

La mira debe permanecer centrada correctamente.

El tamaño configurado debe comportarse de forma consistente entre diferentes configuraciones de DPI.

---

## 7.3 Multi-monitor

Agregar:

```toml
[display]
monitor = 0
```

Ejemplo:

```text
monitor = 0
```

para el monitor principal.

```text
monitor = 1
```

para el segundo monitor.

Si el monitor seleccionado no existe:

1. detectar el error;
2. informar correctamente;
3. utilizar el monitor principal como fallback.

---

## Restricción

La forma continúa siendo únicamente:

```text
cross
```

---

# 8. v0.5.0 — Robustness

## Objetivo

Convertir el proyecto en una herramienta resistente a configuraciones incorrectas y diferentes entornos de Windows.

---

## Configuración

Validar:

```text
color
size
thickness
gap
opacity
outline
outline_color
outline_thickness
monitor
```

---

## Casos inválidos

Detectar correctamente:

```text
color = "green"
size = -10
thickness = 0
opacity = 999
monitor = 99
```

o cualquier otro valor inválido.

---

## Manejo de errores

Los errores deben ser comprensibles.

Ejemplo:

```text
Invalid value for 'opacity': expected a value between 0 and 255.
```

No mostrar mensajes genéricos que dificulten diagnosticar el problema.

---

## Compatibilidad

Probar:

### Windows

* Windows 10;
* Windows 11.

### Resoluciones

* 1280×720;
* 1920×1080;
* 2560×1440;
* 3840×2160.

### DPI

* 100%;
* 125%;
* 150%;
* 200%.

### Monitores

* uno;
* dos;
* tres.

### Modos de ventana

* Windowed;
* Borderless;
* Fullscreen cuando sea técnicamente compatible.

---

# 9. v0.6.0 — Optimization

## Objetivo

Optimizar la implementación después de que la funcionalidad esté completa.

No optimizar prematuramente durante el prototipo.

---

## Métricas

Medir:

```text
Executable size
RAM idle
CPU idle
GPU usage
Startup time
```

---

## CPU

La aplicación no debe utilizar polling continuo.

Evitar:

```c
while (running) {
    render();
    Sleep(1);
}
```

La aplicación debe utilizar el message loop de Windows.

---

## Rendering

La mira debe renderizarse solamente cuando sea necesario.

Al ser estática:

```text
crear
  ?
renderizar
  ?
esperar
```

No:

```text
renderizar
renderizar
renderizar
renderizar
...
```

---

## Ejecutable

Optimizar Release mediante:

* optimización del compilador;
* eliminación de símbolos;
* linker adecuado;
* eliminación de código no utilizado;
* reducción de dependencias.

No sacrificar mantenibilidad por conseguir un ejecutable unos KB más pequeño.

---

# 10. v1.0.0 — Stable

## Objetivo

Convertir el proyecto en una herramienta lista para uso diario.

Esta es la primera versión considerada oficialmente estable.

---

## Funcionalidad

Debe incluir:

```text
cross
color
size
thickness
gap
opacity
outline
outline_color
outline_thickness
monitor
DPI awareness
click-through
topmost
no-focus
no Alt+Tab
defaults
validación
manejo de errores
```

---

## Uso

La experiencia ideal:

```text
crosshair/
+-- crosshair.exe
+-- config.toml
```

Ejecutar:

```powershell
.\crosshair.exe
```

y obtener inmediatamente la mira.

Modificar:

```toml
color = "#FF0000"
```

cerrar y ejecutar nuevamente.

Resultado:

```text
mira roja
```

---

## Lo que NO debe tener v1.0.0

Aunque sea la primera versión estable, no es necesario incluir:

* GUI;
* tray;
* hotkeys;
* perfiles;
* múltiples formas;
* imágenes personalizadas;
* integración con juegos;
* auto-update;
* Internet;
* cuentas;
* sincronización.

La estabilidad es más importante que la cantidad de funcionalidades.

---

# 11. v1.x — Maintenance Releases

Después de `v1.0.0`, se pueden publicar versiones de mantenimiento.

Ejemplos:

```text
v1.0.1
v1.0.2
v1.0.3
v1.1.0
```

---

## v1.0.x

Utilizar principalmente para:

* bug fixes;
* correcciones de compatibilidad;
* correcciones de DPI;
* correcciones multi-monitor;
* problemas de rendering;
* errores de configuración.

---

## v1.1.x

Puede incorporar pequeñas mejoras que no cambien el concepto del producto.

Por ejemplo:

* mejoras del parser;
* nuevos parámetros de la cross;
* pequeñas mejoras de configuración;
* mejoras internas.

No introducir cambios grandes sin una razón clara.

---

# 12. v2.0.0 — Crosshair Expansion

## Objetivo

Expandir las formas disponibles.

Hasta este punto el proyecto se ha centrado exclusivamente en:

```text
cross
```

A partir de `v2.0.0` pueden agregarse nuevas formas.

---

## Primera expansión

Agregar:

```text
cross
dot
circle
```

Opcionalmente:

```text
cross_dot
```

---

## Configuración

Por ejemplo:

```toml
[crosshair]
shape = "dot"
```

o:

```toml
shape = "circle"
```

---

## Importante

La nueva arquitectura debe permitir agregar formas sin convertir `overlay.c` en una colección de condicionales inmanejable.

Mantener una separación clara entre:

```text
overlay
rendering
crosshair shape
configuration
```

---

# 13. v2.x — Advanced Crosshair

Después de introducir varias formas básicas, pueden incorporarse configuraciones más avanzadas.

---

## Segmentos independientes

Permitir:

```toml
top = true
bottom = true
left = true
right = true
```

---

## Longitudes

Permitir:

```toml
top_length = 10
bottom_length = 8
left_length = 12
right_length = 12
```

---

## Center dot

Permitir:

```toml
dot = true
dot_size = 2
```

---

## Offset

Permitir:

```toml
[position]
offset_x = 0
offset_y = 2
```

Esto permitiría ajustar ligeramente la posición de la mira respecto al centro de la pantalla.

---

## Transparencia avanzada

Permitir eventualmente:

```toml
color = "#00FF00"
opacity = 200

outline_color = "#000000"
outline_opacity = 255
```

---

# 14. v3.0.0 — Profiles

## Objetivo

Permitir múltiples configuraciones.

Ejemplo:

```text
profiles/
+-- default.toml
+-- game1.toml
+-- game2.toml
+-- game3.toml
```

Y opcionalmente:

```powershell
crosshair.exe --profile game1
```

---

## Importante

Los perfiles no deben implicar detección automática de juegos.

La aplicación sigue siendo independiente del juego.

No implementar:

```text
"si se abre L4D2, cargar l4d2.toml"
```

a menos que exista una necesidad real y se diseñe posteriormente como una funcionalidad separada.

---

# 15. v3.x — Advanced Optional Features

A partir de aquí se pueden evaluar funcionalidades opcionales.

Posibles características:

* hotkeys;
* cambio de perfil;
* recarga de configuración;
* selección avanzada de monitores;
* offsets;
* presets;
* configuración avanzada de segmentos;
* otros tipos de crosshair.

Estas funcionalidades no forman parte del núcleo del proyecto.

---

# 16. Funcionalidades deliberadamente fuera del alcance

El proyecto debe evitar convertirse en una aplicación de overlay compleja.

No se contempla como objetivo:

```text
GUI avanzada
Marketplace
Cloud sync
Accounts
Telemetry
Advertising
Game launcher
Game modification
DLL injection
Memory manipulation
Anti-cheat bypass
Kernel drivers
Game process hooks
```

Tampoco se pretende competir directamente con aplicaciones completas de personalización de overlays.

La identidad del proyecto es:

> **Minimalismo, eficiencia y simplicidad.**

---

# 17. Compatibilidad con fullscreen

La compatibilidad con fullscreen debe tratarse como una cuestión técnica.

El overlay puede funcionar correctamente en:

```text
Windowed
Borderless Windowed
Fullscreen Windowed
```

El fullscreen exclusivo puede impedir que una ventana Win32 externa aparezca encima del framebuffer presentado por el juego.

No solucionar esto mediante:

* inyección;
* hooks;
* modificación del juego;
* bypass de mecanismos de seguridad.

La limitación debe documentarse.

---

# 18. Versionado

Utilizar Semantic Versioning:

```text
MAJOR.MINOR.PATCH
```

Ejemplos:

```text
0.1.0
0.2.0
0.3.0
1.0.0
1.0.1
2.0.0
```

---

## MAJOR

Cambios incompatibles o una expansión importante del concepto.

Ejemplo:

```text
v1 ? v2
```

cuando se introduzcan múltiples tipos de crosshair.

---

## MINOR

Nuevas funcionalidades compatibles.

Ejemplo:

```text
v1.0 ? v1.1
```

---

## PATCH

Correcciones.

Ejemplo:

```text
v1.0.0 ? v1.0.1
```

---

# 19. Criterio para avanzar de versión

No avanzar automáticamente solo porque exista una lista de funcionalidades.

Cada release debe cumplir:

1. Funcionalidad implementada.
2. Código revisado.
3. Tests relevantes ejecutados.
4. Compatibilidad comprobada.
5. Rendimiento medido cuando corresponda.
6. README actualizado.
7. Cambios documentados.
8. No existen regresiones conocidas importantes.

---

# 20. Definición del producto en cada etapa

## v0.1.0

> "Una cruz aparece."

## v0.2.0

> "Una cruz funciona realmente como overlay."

## v0.3.0

> "Puedo configurarla."

## v0.4.0

> "Se ve correctamente en diferentes pantallas."

## v0.5.0

> "Es robusta."

## v0.6.0

> "Es eficiente."

## v1.0.0

> "Es una herramienta estable que puedo utilizar diariamente."

## v2.0.0

> "Puedo elegir diferentes tipos de mira."

## v3.0.0

> "Puedo administrar diferentes configuraciones."

---

# 21. Visión final

La visión completa del proyecto puede representarse como:

```text
                         crosshair
                             ¦
              +-----------------------------+
              ¦                             ¦
           Core v1                       Expansion
              ¦                             ¦
      +----------------+             +--------------+
      ¦                ¦             ¦              ¦
    Win32           Config        Shapes         Profiles
      ¦                ¦             ¦              ¦
   Overlay           TOML       Cross/Dot/etc.    Presets
      ¦
   GDI
      ¦
   Cross
```

El núcleo debe mantenerse pequeño incluso cuando se agreguen funcionalidades.

---

# 22. Regla principal del roadmap

Antes de agregar una característica, responder:

> **¿Esta característica mejora realmente una herramienta cuyo propósito es mostrar una mira?**

Si la respuesta es no, no implementarla.

El proyecto debe mantener siempre esta propiedad:

```text
Small
        ?
Fast
        ?
Portable
        ?
Predictable
        ?
No bloat
```

---

# 23. Estado inicial del proyecto

La primera implementación debe comenzar exclusivamente con:

```text
v0.1.0
```

y una sola forma:

```text
cross
```

No implementar anticipadamente funcionalidades de `v0.2.0+`.

El desarrollo debe ser incremental.

Primero:

```text
C
 ?
Win32
 ?
GDI
 ?
Crosshair
```

Después:

```text
Overlay
 ?
Configuration
 ?
Visual Quality
 ?
Robustness
 ?
Optimization
 ?
Stable
```

Solo después de que `v1.0.0` esté estable se debe comenzar la expansión hacia nuevas formas y funcionalidades.

---

# 24. Objetivo final

El proyecto no debe medirse por la cantidad de funcionalidades que posee.

Debe medirse por qué tan bien cumple esta frase:

> **"Quiero una mira en Windows. Ejecuto `crosshair.exe` y ya está."**

Ese debe seguir siendo el principio fundamental del proyecto incluso después de las versiones futuras.
