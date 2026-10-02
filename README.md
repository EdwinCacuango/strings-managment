# Auditoría de seguridad de `exampleStrings.c` — Parte I

Práctica de programación segura en C centrada en el tratamiento de cadenas de caracteres. Este documento recoge el entorno de trabajo, el proceso de compilación iterativo, los errores y warnings detectados por el compilador, la auditoría de seguridad con referencia a SEI CERT C y las correcciones aplicadas.

## Estructura del repositorio

|Ruta|Contenido|
|---|---|
|`exampleStrings.c`|Código original proporcionado, sin modificaciones|
|`exampleStrings_fixed.c`|Versión corregida|
|`errores/`|Salida del compilador en las versiones que todavía generaban errores (`e1.txt`, `e2.txt`)|
|`warnings/`|Salida del compilador en las versiones que solo generaban warnings (`w1.txt`, `w2.txt`, `w3.txt`)|

## 1. Entorno y herramientas

|Elemento|Valor|
|---|---|
|Sistema operativo|macOS 26.5.2 (build 25F84)|
|Arquitectura|arm64 (Apple Silicon)|
|Compilador|Apple clang 21.0.0 (clang-2100.1.1.101)|
|Target|arm64-apple-darwin25.5.0|
|Estándar de C|C11 (`-std=c11`)|
|Análisis en ejecución|AddressSanitizer y UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`)|
|Control de versiones|Git (un commit por corrección)|

**Nota sobre el compilador:** en macOS, el comando `gcc` no invoca GCC de GNU, sino que es un alias de Apple Clang. Por eso, aunque el comando de compilación usa `gcc`, todos los mensajes recogidos en este documento tienen el formato de Clang, como confirma la salida de `gcc --version`:

```
Apple clang version 21.0.0 (clang-2100.1.1.101)
Target: arm64-apple-darwin25.5.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

## 2. Comando de compilación

```
gcc -std=c11 -Wall -Wextra -Wpedantic exampleStrings.c -o exampleStrings
```

|Opción|Función|
|---|---|
|`-std=c11`|Compila según el estándar ISO C11, sin extensiones de C++ ni de GNU|
|`-Wall`|Activa el conjunto principal de warnings|
|`-Wextra`|Añade warnings adicionales no incluidos en `-Wall` (por ejemplo, parámetros no usados)|
|`-Wpedantic`|Emite warnings ante cualquier construcción que no sea conforme al estándar seleccionado|

## 3. Metodología

La corrección se realizó de forma iterativa:

1. Compilar la versión actual y guardar la salida del compilador en un archivo de texto.
2. Analizar el primer problema reportado, entender el fragmento de código y su finalidad.
3. Aplicar una única corrección y registrarla en un commit propio, junto con la salida del compilador que la motivó.
4. Volver a compilar y repetir hasta no obtener errores ni warnings.

Cada archivo en `errores/` y `warnings/` corresponde a la salida del compilador **antes** de aplicar la corrección del commit que lo incluye. Los números de línea que aparecen en esos archivos corresponden a cada versión intermedia: a partir del paso 1 se desplazan 3 líneas respecto al original, porque el literal de 4 líneas pasó a ocupar 1. En este documento se indican siempre las líneas del **archivo original**.

El uso de herramientas de IA durante el proceso se describe en la sección 9.

## 4. Compilación inicial

La primera compilación del código original (`errores/e1.txt`) produjo **3 errores y 4 warnings**:

|Tipo|Línea|Mensaje|Flag|
|---|---|---|---|
|Error|22|`use of undeclared identifier 'R'`|—|
|Warning|22|`missing terminating '"' character`|`-Winvalid-pp-token`|
|Warning|25|`missing terminating '"' character`|`-Winvalid-pp-token`|
|Error|32|`void function 'gets_example_func' should not return a value`|`-Wreturn-mismatch`|
|Warning|51|`'gets' is deprecated`|`-Wdeprecated-declarations`|
|Error|94|`use of undeclared identifier 's2'`|—|
|Warning|60|`unused parameter 'argc'`|`-Wunused-parameter`|

En compilaciones posteriores aparecieron 3 warnings adicionales de variables no usadas, que no se mostraban en esta primera salida (ver paso 3).

## 5. Historial de correcciones (Fase 1)

### Paso 1 — Raw string literal de C++ (commit `fb08a4f`)

**Salida asociada:** `errores/e1.txt`

**Código original (líneas 22–25):**

```c
const char* s1 = R"foo(
Hello
World
)foo";
```

**Problema:** la sintaxis `R"delimitador(...)delimitador"` es un _raw string literal_, una característica de C++11 que no existe en C. Al compilar con `-std=c11`, el compilador interpreta `R` como un identificador no declarado y la comilla siguiente como el inicio de una cadena que nunca se cierra en la misma línea. Esto produce el error en `R` y los dos warnings de comilla sin terminar (líneas 22 y 25).

**Error derivado:** el literal mal interpretado desordena el análisis del código que le sigue, de modo que la declaración de `s2` (línea 26) no se reconoce. Por eso aparece el error `use of undeclared identifier 's2'` en la línea 94, aunque `s2` sí está declarada. No es un error independiente, sino una consecuencia del primero.

**Corrección:** se reescribe el literal con secuencias de escape de C, conservando el mismo contenido (salto de línea, `Hello`, salto de línea, `World`, salto de línea):

```c
const char* s1 = "\nHello\nWorld\n";
```

**Resultado** (`errores/e2.txt`): desaparecen el error de `R`, los dos warnings de comilla y el error de `s2`. Quedan **1 error y 5 warnings**.

### Paso 2 — Función `void` que devuelve un valor (commit `5f370b9`)

**Salida asociada:** `errores/e2.txt`

**Código original (líneas 28–35):**

```c
void gets_example_func(void) {
  char buf[BUFFER_MAX_SIZE];

  if (fgets(buf, sizeof(buf), stdin) == NULL) {
        return 1;
  }
  buf[strlen(buf) - 1] = '\0';
}
```

**Problema:** la función está declarada con tipo de retorno `void`, es decir, no debe devolver ningún valor. Sin embargo, si `fgets` falla, la condición ejecuta `return 1;`. Clang trata esta incoherencia como error (`-Wreturn-mismatch`).

**Corrección:** se elimina el valor devuelto y se conserva la salida anticipada de la función, que es el comportamiento que se interpreta como esperado (abandonar la función si no se pudo leer la entrada):

```c
return;
```

**Resultado** (`warnings/w1.txt`): **0 errores y 5 warnings**. Esta es la primera versión del código que compila y genera un ejecutable.

### Paso 3 — Variables declaradas y no utilizadas (commit `256f13e`)

**Salida asociada:** `warnings/w1.txt`

**Código original (líneas 68–73):**

```c
int size_array1 = strlen("аналитик");
int size_array2 = 100;

// char analitic1[size_array1]="аналитик";
// char analitic2[size_array2]="аналитик";
char analitic3[100]="аналитик";
```

**Problema:** `size_array1`, `size_array2` y `analitic3` se declaran e inicializan, pero no se usan en ninguna parte del programa (`-Wunused-variable`). Las dos primeras solo tenían sentido como tamaño de `analitic1` y `analitic2`, que ya venían comentadas en el original.

**Por qué no aparecían en la compilación inicial:** estos warnings se hicieron visibles una vez corregidos los errores del paso 1. Una explicación probable es que Clang omite los avisos de variables no usadas dentro de un ámbito en el que ya se produjo un error, y `main` contenía el error derivado de `s2`.

**Corrección:** se comentan las tres variables en lugar de eliminarlas, dejando pendiente consultar si su presencia tiene una finalidad en el ejercicio:

```c
// int size_array1 = strlen("аналитик");
// int size_array2 = 100;
// char analitic3[100]="аналитик";
```

**Resultado** (`warnings/w2.txt`): **2 warnings**.

### Paso 4 — Uso de `gets`, función deprecada (commit `147c0e6`)

**Salida asociada:** `warnings/w2.txt`

**Código original (líneas 47–57):**

```c
void get_y_or_n(void) {
	char response[8];

	printf("Continue? [y] n: ");
	gets(response);

	if (response[0] == 'n')
		exit(0);

	return;
}
```

**Problema:** `gets` lee una línea de la entrada estándar sin recibir el tamaño del buffer de destino, por lo que no puede limitar cuántos caracteres escribe. Si el usuario introduce más caracteres de los que caben en `response` (8 bytes), se escribe fuera del array. Por este motivo `gets` fue eliminada del estándar en C11. La biblioteca de macOS la mantiene solo por compatibilidad y la marca como deprecada, lo que genera el warning `-Wdeprecated-declarations`.

**Corrección:** se sustituye por `fgets`, que recibe tres parámetros: el buffer de destino, el tamaño máximo a leer (incluido el `'\0'` final) y el flujo de origen:

```c
fgets(response, sizeof(response), stdin);
```

`fgets` lee como máximo `sizeof(response) - 1` caracteres y siempre termina la cadena con `'\0'`, de modo que no puede desbordar el buffer.

**Resultado** (`warnings/w3.txt`): **1 warning**.

### Paso 5 — Parámetro `argc` sin usar (commit `92ff526`)

**Salida asociada:** `warnings/w3.txt`

**Código original (línea 60 y líneas 78–80):**

```c
int main(int argc, char *argv[])
...
strcpy(key, argv[1]);
strcat(key, " = ");
strcat(key, argv[2]);
```

**Problema:** `main` recibe `argc`, que indica cuántos argumentos se pasaron al programa (incluido el nombre del ejecutable en `argv[0]`), pero nunca lo consulta (`-Wunused-parameter`). Esto no es solo un aviso de estilo: el programa accede directamente a `argv[1]` y `argv[2]` sin comprobar que existan. Si se ejecuta con menos de dos argumentos, se accede a posiciones de `argv` que no contienen un argumento válido.

**Corrección:** en lugar de eliminar `argc`, se usa para validar el número de argumentos al inicio de `main` y terminar con un mensaje de uso si no es el esperado:

```c
if (argc != 3)
{
	printf("Error: Faltan argumentos.\n");
	printf("Uso correcto: %s <argumento1> <argumento2>\n", argv[0]);
	return 1;
}
```

**Resultado:** compilación sin errores ni warnings (código de salida 0).

## 6. Resumen de la fase de compilación

|Paso|Commit|Línea (original)|Tipo|Problema|Corrección|
|---|---|---|---|---|---|
|1|`fb08a4f`|22–25, 94|1 error + 2 warnings + 1 error derivado|Raw string literal de C++ no válido en C|Literal con secuencias `\n`|
|2|`5f370b9`|32|Error|`return 1` en función `void`|`return;`|
|3|`256f13e`|68, 69, 73|3 warnings|Variables declaradas y no usadas|Comentadas, pendiente de consulta|
|4|`147c0e6`|51|Warning|`gets` deprecada, sin límite de tamaño|`fgets(response, sizeof(response), stdin)`|
|5|`92ff526`|60|Warning|`argc` no usado, acceso a `argv` sin validar|Validación `argc != 3`|

|Compilación|Archivo|Errores|Warnings|
|---|---|---|---|
|Original|`errores/e1.txt`|3|4|
|Tras paso 1|`errores/e2.txt`|1|5|
|Tras paso 2|`warnings/w1.txt`|0|5|
|Tras paso 3|`warnings/w2.txt`|0|2|
|Tras paso 4|`warnings/w3.txt`|0|1|
|Tras paso 5|—|0|0|

**Compilación al cierre de la Fase 1:**

```
$ gcc -std=c11 -Wall -Wextra -Wpedantic exampleStrings.c -o exampleStrings
$ echo "exit code: $?"
exit code: 0
```

**Primeras observaciones en ejecución:**

- Sin argumentos suficientes, el programa muestra el mensaje de uso y termina con código 1.
- Con dos argumentos, el programa queda a la espera de entrada por teclado: la llamada a `fgets` de `main` (línea 83 del original) lee de `stdin` sin mostrar ningún mensaje previo. Tras introducir texto, continúa con la pregunta `Continue? [y] n:` de `get_y_or_n`.

### 6.1 Relación de los problemas de compilación con SEI CERT C

|Paso|Problema|CERT|Rule / Recommendation|Relación|
|---|---|---|---|---|
|1|Raw string literal de C++|—|—|Error de conformidad con el lenguaje C, sin regla CERT asociada|
|2|`return 1` en función `void`|—|—|Error de conformidad con el lenguaje C, sin regla CERT asociada|
|3|Variables declaradas y no usadas|MSC13-C|Recommendation|_Detect and remove unused values_: los valores que nunca se usan pueden ocultar errores de lógica|
|4|Uso de `gets`|MSC24-C, STR31-C|Rule|MSC24-C prohíbe usar funciones deprecadas u obsoletas. `gets` aparece además como ejemplo no conforme de STR31-C, porque no puede garantizar que los datos quepan en el destino|
|5|`argc` sin usar, acceso a `argv[1]` y `argv[2]` sin validar|EXP34-C, ARR30-C|Rule|El estándar garantiza que `argv[argc]` es un puntero nulo. Si `argc == 2`, `argv[2]` es nulo y `strcat` lo desreferencia (EXP34-C). Si `argc == 1`, `argv[1]` es nulo y `argv[2]` queda fuera de los límites del array (ARR30-C)|

## 7. Auditoría de seguridad (SEI CERT C)

### 7.1 Metodología (Fase 2)

Partiendo de la versión que compila sin errores ni warnings, la auditoría se realizó en cuatro pasos:

1. **Identificación en un entorno controlado.** Se utilizó un cuaderno de Gemini cuya única fuente de información es el documento _SEI CERT C Coding Standard_. Se le entregó el código completo para que identificara las construcciones no conformes. Restringir la fuente al propio estándar reduce el riesgo de que las reglas citadas sean inventadas o provengan de otras guías.
2. **Contraste con el estándar.** Cada hallazgo se buscó en el texto de la regla correspondiente, para comprobar que la regla existe, que su descripción coincide y que realmente aplica al fragmento señalado.
3. **Consultas puntuales sobre C.** Para cada problema se siguió el mismo método de la Fase 1: preguntas aisladas sobre las funciones y conceptos implicados, sin delegar en la IA la decisión final.
4. **Propuesta de solución por el autor.** La corrección no se tomó de la herramienta. Se planteó a partir de lo entendido en los pasos anteriores y de los ejemplos conformes del estándar, para evitar dar por buenos falsos positivos o correcciones mecánicas.

Cada corrección se registró en un commit propio cuyo título es el identificador de la regla CERT.

### 7.2 Tabla de problemas

Las líneas indicadas corresponden al archivo original.

|#|Localización|Problema|CERT|Rule / Recommendation|Consecuencia|Corrección|
|---|---|---|---|---|---|---|
|P1|`get_dirname`, l. 41 (llamada en l. 75)|Escritura de `'\0'` sobre el literal `__FILE__` a través del puntero que devuelve `strrchr`|STR30-C (principal), EXP40-C (relacionada)|Rule|Comportamiento indefinido; normalmente, fallo de segmentación|La función copia el directorio en un buffer del llamador, con tamaño controlado (`1504471`)|
|P2|`main`, l. 101 (declaración en l. 67)|`ptr_char[0] = 'N'` modifica un literal de cadena|STR30-C|Rule|Comportamiento indefinido; normalmente, fallo de segmentación|`ptr_char` pasa a ser un array modificable (`330456b`)|
|P3|`gets_example_func`, l. 34|`buf[strlen(buf) - 1]` asume que `fgets` devuelve una cadena no vacía|FIO37-C|Rule|Escritura fuera de los límites del array|Búsqueda de `'\n'` con `strchr` antes de sustituirlo (`4710ffb`)|
|P4|`main`, l. 97–98|`strncpy` deja `array3` y `array4` sin terminador nulo, y luego `strlen(array3)` lo recorre|STR32-C|Rule|Lectura fuera de límites; comportamiento indefinido|Copia limitada a `tamaño - 1` y terminación explícita inmediata (`f7202a8`, `0f357cc`, `ff1fc32`)|
|P5|`main`, l. 62 y 78–80|`strcpy`/`strcat` sobre `key[24]` con `argv[1]` y `argv[2]` sin validar longitud|STR31-C|Rule|Desbordamiento de buffer en la pila|Reserva dinámica del tamaño exacto y construcción con `snprintf` (`258b5a5`, `ff1fc32`)|
|P6|`get_y_or_n`, l. 51; `main`, l. 83|No se comprueba el valor de retorno de `fgets`|ERR33-C, FIO40-C|Rule|Uso de contenido indeterminado del buffer|Comprobación de `NULL` y terminación del programa (`27182eb`)|

### 7.3 Análisis detallado

#### P1 — Modificación de un literal de cadena en `get_dirname`

```c
const char *get_dirname(const char *pathname) {
  char *slash;
  slash = strrchr(pathname, '/');
  if (slash) {
    *slash = '\0'; /* Undefined behavior */
  }
  return pathname;
}
...
puts(get_dirname(__FILE__));
```

**Instrucción que lo origina:** `*slash = '\0';`. La macro `__FILE__` se expande a un literal de cadena. `strrchr` recibe un `const char *` pero devuelve un `char *`, de modo que la calificación `const` se pierde sin ningún aviso del compilador. Esa pérdida es la que permite escribir a través de `slash`.

**Condiciones:** la escritura solo se produce si `__FILE__` contiene el carácter `'/'`. El valor de `__FILE__` depende de cómo se invoque al compilador. Con el comando documentado (`gcc ... exampleStrings.c`), el valor es `"exampleStrings.c"`, no hay `'/'` y la escritura no llega a ejecutarse. Si se compila con una ruta (`./exampleStrings.c` o una ruta absoluta), sí se ejecuta. El defecto existe en ambos casos, aunque solo se manifieste en el segundo.

**Consecuencia:** modificar un literal de cadena es comportamiento indefinido. En la práctica, los literales se almacenan en una sección de solo lectura (`.rodata`) y el programa termina con un fallo de segmentación.

**Reglas aplicables:** STR30-C (_Do not attempt to modify string literals_) es la regla principal. El código de `get_dirname` coincide con su ejemplo no conforme. EXP40-C (_Do not modify constant objects_) se considera relacionada, porque el puntero recibido es `const char *` y se escribe a través de un puntero derivado de él. Formalmente, sin embargo, un literal de cadena en C no es un objeto declarado `const`, por lo que STR30-C describe el problema con más precisión.

**Corrección aplicada (`1504471`):** dado que el código del ejercicio coincide con el ejemplo no conforme de STR30-C, se adopta su solución conforme, que no requería adaptaciones:

```c
char *get_dirname(const char *pathname, char *dirname, size_t size)
{
  const char *slash;
  slash = strrchr(pathname, '/');
  if (slash) {
    ptrdiff_t slash_idx = slash - pathname;
    if ((size_t)slash_idx < size) {
      memcpy(dirname, pathname, slash_idx);
      dirname[slash_idx] = '\0';
      return dirname;
    }
  }
  return 0;
}
...
char dirname[260];
if (get_dirname(__FILE__, dirname, sizeof(dirname))) {
  puts(dirname);
}
```

**Justificación:** la función ya no modifica su argumento. El puntero intermedio se declara `const char *`, de modo que cualquier intento de escribir a través de él produciría un error de compilación. El directorio se copia en un buffer proporcionado por el llamador, y se comprueba antes que quepa en él. Si no hay `'/'` o el resultado no cabe, la función devuelve un puntero nulo y `main` no imprime nada. Se añade `<stddef.h>` por el tipo `ptrdiff_t`.

**Cambio de comportamiento:** con el comando de compilación documentado, `__FILE__` no contiene `'/'` y el programa ya no imprime ninguna línea en este punto. El original imprimía la cadena completa.

#### P2 — Modificación de un literal a través de `ptr_char`

```c
char array5 []  = "01234567890123456";
char *ptr_char  = "new string literal";
...
array5 [0] = 'M';
ptr_char [0] = 'N';
```

**Instrucción que lo origina:** `ptr_char[0] = 'N';`. `ptr_char` apunta directamente al literal, no a una copia.

**Condiciones:** se produce siempre que la ejecución llegue a esa línea.

**Consecuencia:** comportamiento indefinido; normalmente, un fallo de segmentación.

**Contraste:** `array5[0] = 'M'` es válido. `array5` es un array local que se inicializa con una copia del literal, por lo que su contenido es modificable.

**Regla aplicable:** STR30-C. El compilador no avisa porque en C un literal tiene tipo `char[]`, no `const char[]`.

**Corrección aplicada (`330456b`):**

```c
char ptr_char [] = "new string literal";
```

**Justificación:** como el programa modifica la cadena después de declararla, se declara como array, igual que `array5`. Así se inicializa con una copia del literal en memoria modificable, lo que corresponde a la solución conforme de STR30-C.

#### P3 — Eliminación insegura del salto de línea tras `fgets`

```c
if (fgets(buf, sizeof(buf), stdin) == NULL) {
      return;
}
buf[strlen(buf) - 1] = '\0';
```

**Instrucción que lo origina:** `buf[strlen(buf) - 1] = '\0';`. Asume que la cadena leída tiene al menos un carácter y que el último es `'\n'`.

**Condiciones:**

- `fgets` puede leer bytes nulos, no solo caracteres de texto. Si el primer byte leído es un `'\0'`, `strlen(buf)` devuelve 0. Como `strlen` devuelve `size_t`, un tipo sin signo, `0 - 1` no da `-1`, sino el mayor valor representable (`SIZE_MAX`). La escritura se realiza muy fuera de los límites de `buf`.
- Si la línea no termina en `'\n'` (es más larga que el buffer, o la entrada acaba sin salto de línea), se borra un carácter válido de los datos.

La función `gets_example_func` no se invoca desde `main`, por lo que el defecto está latente en el programa actual. Sin embargo, se manifestaría en cuanto la función se utilizara.

**Consecuencia:** escritura fuera de los límites del array (comportamiento indefinido) o pérdida silenciosa de datos.

**Regla aplicable:** FIO37-C (_Do not assume that fgets() or fgetws() returns a nonempty string when successful_). Este fragmento coincide con el ejemplo no conforme de la regla.

**Corrección aplicada (`4710ffb`):**

```c
char *p;

if (fgets(buf, sizeof(buf), stdin)) {
  p = strchr(buf, '\n');
  if (p) {
    *p = '\0';
  }
} else {
  return;
}
```

**Justificación:** siguiendo la solución conforme de FIO37-C, el salto de línea no se presupone, sino que se busca con `strchr`. Solo si existe se sustituye por `'\0'`, a través del puntero que guarda su posición. Esto elimina tanto la escritura fuera de límites como la pérdida de un carácter válido.

#### P4 — Cadenas sin terminador nulo tras `strncpy`

```c
char array3[16];
char array4[16];
char array5 []  = "01234567890123456";
...
strncpy(array3, array5, sizeof(array3));
strncpy(array4, array3, strlen(array3));
...
array3[sizeof(array3)-1]='\0';
```

**Instrucciones que lo originan:**

- `array5` contiene 17 caracteres más el terminador (18 bytes), y `array3` tiene 16 bytes. `strncpy` no añade `'\0'` cuando la cadena origen tiene una longitud igual o mayor al límite indicado. Tras la primera llamada, `array3` contiene 16 caracteres sin terminador.
- En la segunda llamada, `strlen(array3)` recorre la memoria más allá de `array3` buscando un `'\0'` que no está dentro del array.
- Además, la segunda copia transfiere `strlen(array3)` bytes, que nunca incluyen el terminador, por lo que `array4` tampoco queda terminada.

**Condiciones:** con los valores actuales, se produce siempre.

**Consecuencia:** lectura fuera de límites, que es comportamiento indefinido. El resultado puede ser un valor de `strlen` arbitrario, una copia de más bytes de los que caben en `array4` o un fallo. La terminación de la línea 103 llega tarde: el acceso indebido ya ocurrió en la línea 98.

**Regla aplicable:** STR32-C (_Do not pass a non-null-terminated character sequence to a library function that expects a string_), porque `array3` se pasa a `strlen` sin estar terminada.

**Corrección aplicada (`f7202a8`, `0f357cc`, `ff1fc32`):**

```c
// truncamiento forzoso
strncpy(array3, array5, sizeof(array3) -1);
array3[sizeof(array3) -1]  = '\0';

// copia a array 4
strncpy(array4, array3, sizeof(array4) -1);
array4[sizeof(array4) -1] = '\0';
```

Cada copia se limita a `tamaño - 1` bytes y el destino se termina explícitamente justo después, antes de cualquier uso como cadena. La terminación tardía del original (línea 103) se elimina en `0f357cc`, porque queda redundante.

La primera versión de esta corrección (`f7202a8`) limitaba la copia a `array4` con `strlen(array4) - 1`. Como `array4` no está inicializada, esa llamada reintroducía el problema: `strlen` recorría memoria indeterminada y, si devolvía 0, la resta daba `SIZE_MAX`. El error no producía ningún warning y se detectó en ejecución con AddressSanitizer (ver 7.4). Se corrigió en `ff1fc32` sustituyendo `strlen` por `sizeof`, que da el tamaño declarado del array sin leer su contenido.

**Justificación y alternativa pendiente de valorar:** la solución adoptada es la de truncamiento que propone STR32-C. Cumple la regla, pero compromete la integridad del dato: `array3` solo conserva los primeros 15 de los 17 caracteres de `array5`. Una alternativa que también evita el problema sin perder datos es aumentar el tamaño de `array3` y `array4` para que puedan contener la cadena completa. Elegir entre ambas depende de si el truncamiento es aceptable para el programa, lo que debe valorarse con el desarrollador. Se opta por el truncamiento para cumplir con el objetivo del ejercicio, y se deja documentado de forma explícita en el código (comentario "truncamiento forzoso").

#### P5 — Desbordamiento de `key` con los argumentos del programa

```c
char key[24];
...
strcpy(key, argv[1]);
strcat(key, " = ");
strcat(key, argv[2]);
```

**Instrucción que lo origina:** las tres llamadas escriben en `key` sin comprobar si los datos caben. `argv[1]` y `argv[2]` los controla quien ejecuta el programa.

**Condiciones:** `key` admite 23 caracteres más el terminador. Como `" = "` ocupa 3, el desbordamiento se produce cuando `strlen(argv[1]) + strlen(argv[2]) > 20`. Basta con que `argv[1]` supere los 23 caracteres para que el desbordamiento ocurra ya en `strcpy`. La validación de `argc` añadida en la Fase 1 garantiza que los argumentos existen, pero no limita su longitud.

**Consecuencia:** desbordamiento de buffer en la pila. Puede sobrescribir otras variables locales o datos de control de la pila, lo que provoca un fallo del programa o, en el peor caso, permite alterar su flujo de ejecución.

**Regla aplicable:** STR31-C (_Guarantee that storage for strings has sufficient space for character data and the null terminator_). Aplica porque el tamaño del destino es fijo y la longitud de los datos depende de una entrada externa no validada.

**Corrección aplicada (`258b5a5`, `ff1fc32`):**

```c
size_t key_len = strlen(argv[1]) + strlen(" = ") + strlen(argv[2]) + 1;
char *key = (char *)malloc(key_len);

if (key != NULL) {
  snprintf(key, key_len, "%s = %s", argv[1], argv[2]);
  free(key);
} else {
  return EXIT_FAILURE;
}
```

**Justificación:** en lugar de un buffer de tamaño fijo, se calcula primero el tamaño exacto que necesita el resultado (las dos longitudes, el separador y el terminador) y se reserva esa cantidad de memoria dinámica. Es el mismo enfoque que la solución conforme de STR31-C para argumentos de `argv`. Si la reserva falla, el programa termina con error. La cadena se construye con `snprintf`, que nunca escribe más de `key_len` bytes y siempre termina el resultado, y la memoria se libera tras su uso. En el mismo commit, el `return 1` de la validación de `argc` se sustituye por `EXIT_FAILURE`.

La primera versión (`258b5a5`) pasaba `argv[1]` dos veces a `snprintf`. No provocaba un desbordamiento, porque `snprintf` respeta `key_len`, pero producía un resultado incorrecto y truncado cuando `argv[1]` era más largo que `argv[2]`. También incluía `#define __STDC_WANT_LIB_EXT1__1`, que por la falta de un espacio definía una macro distinta de `__STDC_WANT_LIB_EXT1__` y no tenía ningún efecto. Ambos problemas se corrigieron en `ff1fc32`.

#### P6 — Valor de retorno de `fgets` sin comprobar

```c
// get_y_or_n (gets sustituida por fgets en el paso 4)
char response[8];
fgets(response, sizeof(response), stdin);
if (response[0] == 'n')
    exit(0);

// main
fgets(response, sizeof(response), stdin);
```

**Instrucción que lo origina:** ninguna de las dos llamadas comprueba si `fgets` devolvió `NULL`.

**Condiciones:** `fgets` falla si se alcanza el fin de archivo sin leer ningún carácter (por ejemplo, Ctrl+D en la terminal o entrada redirigida desde un archivo vacío) o si se produce un error de lectura. En el primer caso, el buffer no se modifica. Como `response` es un array local sin inicializar, `response[0]` contiene un valor indeterminado. En el segundo caso, el estándar establece que el contenido del buffer es indeterminado.

**Consecuencia:** el programa toma decisiones (como continuar o terminar en `get_y_or_n`) a partir de datos que no proceden de la entrada.

**Reglas aplicables:** ERR33-C (_Detect and handle standard library errors_), porque no se trata el error de una función de la biblioteca estándar. FIO40-C (_Reset strings on fgets() or fgetws() failure_), porque el buffer no se restablece tras el fallo y se usa a continuación.

**Corrección aplicada (`27182eb`):**

```c
// get_y_or_n
if (fgets(response, sizeof(response), stdin) == NULL) {
  exit(EXIT_FAILURE);
}

// main
if (fgets(response, sizeof(response), stdin) == NULL) {
  return EXIT_FAILURE;
}
```

**Justificación:** ERR33-C exige tratar el error según la función que falla. En el caso de `fgets`, el indicador de error es el retorno `NULL`. Ante un fallo, el programa termina con código de error, de modo que el contenido indeterminado del buffer nunca llega a usarse. Esto cumple el objetivo de FIO40-C por una vía distinta a su solución conforme, que restablece el buffer a cadena vacía: aquí no es necesario restablecerlo, porque no se vuelve a leer.

### 7.4 Verificación

La versión corregida se compiló con las opciones documentadas en la sección 2 y se ejecutó con AddressSanitizer y UndefinedBehaviorSanitizer, que detectan en tiempo de ejecución accesos fuera de límites y comportamiento indefinido:

```
gcc -std=c11 -Wall -Wextra -Wpedantic exampleStrings_fixed.c -o exampleStrings_fixed
gcc -std=c11 -g -fsanitize=address,undefined exampleStrings_fixed.c -o exampleStrings_asan
```

**Detección del fallo en la primera corrección de P4.** Con la versión de `f7202a8` (`strncpy(array4, array3, strlen(array4) -1)`), que compilaba sin warnings, AddressSanitizer abortó la ejecución:

```
==119==ERROR: AddressSanitizer: negative-size-param: (size=-1)
    #0 ... in strncpy
    #1 ... in main exampleStrings.c:133
...
    [112, 128) 'array4' (line 86) <== Memory access at offset 112 is inside this variable
SUMMARY: AddressSanitizer: negative-size-param ... in strncpy
```

**Pruebas sobre la versión final** (ejecutable compilado con sanitizers):

|Caso|Comando|Resultado|
|---|---|---|
|Ejecución normal|`printf 'hola\ny\n' \| ./exampleStrings_asan uno dos`|Imprime `Foobar`, `Foobar`, `Hello World` (×2); exit 0; sin informes|
|Respuesta `n`|`printf 'hola\nn\n' \| ./exampleStrings_asan uno dos`|Termina en `get_y_or_n`; exit 0|
|Entrada vacía (EOF)|`printf '' \| ./exampleStrings_asan uno dos`|Termina por el control de `fgets`; exit 1|
|Argumentos largos|Dos argumentos de 200 caracteres|Sin desbordamiento; exit 0. Con el `key[24]` original, este caso desbordaba la pila|
|Sin argumentos|`./exampleStrings_asan`|Mensaje de uso; exit 1|
|`__FILE__` con ruta|Compilando como `sub/exampleStrings_fixed.c`|`get_dirname` imprime `sub`|

En ninguno de los casos los sanitizers informaron de accesos indebidos ni de comportamiento indefinido.

## 8. Corrección y justificación

### Solución A — Corrección mínima

Se aplicó en los problemas cuya corrección no requería cambiar la interfaz ni la estructura del código:

|#|Corrección|
|---|---|
|P2|Cambio de la declaración de `ptr_char` de puntero a array|
|P3|Sustitución de `strlen(buf) - 1` por la búsqueda de `'\n'` con `strchr`|
|P4|Límite de copia a `tamaño - 1` y terminación explícita inmediata|
|P6|Comprobación del retorno de `fgets` y terminación en caso de fallo|

### Solución B — Refactorización

Se aplicó en los problemas donde el origen del error estaba en el diseño, de modo que una corrección mínima habría dejado abierta la posibilidad de reintroducirlo:

|#|Corrección|Por qué se refactoriza|
|---|---|---|
|P1|`get_dirname` cambia de firma: recibe el buffer destino y su tamaño|La función original modificaba su argumento. Con la nueva interfaz, no puede escribir en `pathname` (el compilador lo impide) y el llamador controla el tamaño del resultado|
|P5|`key` pasa de buffer fijo a memoria dinámica del tamaño exacto|Con un buffer fijo, cualquier límite elegido podría superarse con argumentos más largos. Calcular el tamaño a partir de los datos elimina esa dependencia|

### Solución seleccionada y justificación

En cada problema se partió de la solución conforme que propone el propio estándar CERT, adaptada al código cuando fue necesario. Las correcciones mínimas se mantuvieron allí donde bastaban para eliminar el problema sin efectos secundarios. La refactorización se reservó para P1 y P5, donde el error derivaba de la forma en que estaba diseñada la función o el buffer.

Quedan pendientes de valorar con el desarrollador dos decisiones:

- **P4:** si el truncamiento de `array5` es aceptable o si conviene aumentar el tamaño de `array3` y `array4`.
- **Paso 3 de la Fase 1:** si las variables comentadas (`size_array1`, `size_array2`, `analitic3`) tienen alguna finalidad en el programa.

## 9. Uso de Inteligencia Artificial

### 9.1 Herramientas utilizadas

|Herramienta|Fase|Rol|
|---|---|---|
|Gemini 3.6, modo Pensar|Fase 1|Primera capa: consultas generales sobre conceptos de C|
|Gemini 3.1 Pro|Fase 1|Segunda capa: análisis específico del funcionamiento de fragmentos concretos|
|Cuaderno de Gemini con el _SEI CERT C Coding Standard_ como única fuente|Fase 2|Identificación de construcciones no conformes en el código completo|
|Claude Opus 5.5 (Anthropic)|Documentación y revisión|Redacción asistida del README y revisión de las correcciones mediante compilación y pruebas|

### 9.2 Tareas para las que se utilizó

**Fase 1 — Comprensión del código y de los mensajes del compilador.** El objetivo era entender cada parte del código, no obtener correcciones. Las consultas se plantearon de forma aislada y progresiva:

1. Preguntas conceptuales sobre el elemento desconocido (por ejemplo, qué es `gets`).
2. Preguntas sobre sus parámetros, cuando no quedaban claros.
3. Usos hipotéticos, sin copiar el código del ejercicio.
4. Solo después, el fragmento concreto donde se localizaba el error.
5. Con el concepto entendido, el autor proponía la corrección adaptada al código.

En esta fase no se compartió el código completo con ninguna herramienta, y la IA no sugirió correcciones directas.

**Fase 2 — Localización de problemas y búsqueda de reglas CERT.** El cuaderno de Gemini, limitado al documento del estándar, identificó los problemas y las reglas candidatas. A partir de ahí se siguió el método de la Fase 1: cada regla se buscó en el estándar y cada corrección la planteó el autor, tomando como referencia las soluciones conformes de CERT.

**Documentación y revisión — Claude.** Se utilizó para:

- Reconstruir el historial de la Fase 1 a partir de los commits, los diffs, las salidas guardadas en `errores/` y `warnings/` y la bitácora de trabajo, y redactar el README a partir de ello.
- Revisar el informe de Gemini antes de integrarlo, señalando errores y omisiones (ver 9.4).
- Evaluar las correcciones del autor: compilarlas y ejecutarlas con sanitizers y entradas límite, y señalar los fallos encontrados.

Por indicación expresa del autor, Claude no propuso correcciones para los problemas CERT. La única excepción es la corrección de la línea `strncpy(array4, ...)`, que se pidió de forma explícita tras detectarse el fallo (ver 9.4, ejemplo 1).

### 9.3 Verificación de las respuestas

Ninguna respuesta de IA se consideró evidencia por sí misma:

- **Reglas CERT:** cada regla citada se contrastó con el texto del estándar, comprobando que existe, que su descripción coincide y que aplica al fragmento señalado.
- **Correcciones:** cada versión se compiló con las opciones documentadas (`-std=c11 -Wall -Wextra -Wpedantic`). La versión final se ejecutó además con AddressSanitizer y UndefinedBehaviorSanitizer, que detectan en tiempo de ejecución accesos fuera de límites y comportamiento indefinido.
- **Casos de prueba:** ejecución normal; respuesta `n` en `get_y_or_n`; entrada vacía (EOF); argumentos de 200 caracteres cada uno; ejecución sin argumentos; y compilación con ruta, para que `__FILE__` contenga `'/'` y se ejercite `get_dirname` (ver 7.4).
- **Otros compiladores:** además de Apple Clang, el código se compiló con GCC. La revisión de Claude lo compiló con GCC 13.3 en Linux, que emitió un warning `-Wunused-but-set-variable` en `ptr_char`. La comprobación del autor con GCC en línea no mostró errores ni warnings, al igual que Apple Clang, el compilador documentado.

### 9.4 Ejemplos relevantes

**Ejemplo 1 — Una corrección del autor que reintroducía el problema.** En la primera versión de la corrección de P4, la copia a `array4` quedó así:

```c
strncpy(array4, array3, strlen(array4) -1);
```

El código compilaba sin warnings con GCC y Clang, incluso con `-O2`. En la revisión, Claude señaló que `array4` no estaba inicializada: `strlen` leía memoria indeterminada (lo mismo que prohíbe STR32-C) y, si devolvía 0, la resta en `size_t` daba `SIZE_MAX`, con lo que `strncpy` intentaba rellenar con ceros casi toda la memoria a partir de `array4`. La afirmación no se dio por buena sin más: se reprodujo con AddressSanitizer, que abortó con `negative-size-param: (size=-1)` en esa línea. Tras sustituir `strlen` por `sizeof`, el programa se ejecutó sin errores bajo los sanitizers. 

**Ejemplo 2 — Una corrección que no hacía lo que pretendía.** En la corrección de P5 se añadió `#define __STDC_WANT_LIB_EXT1__1`. Por la falta de un espacio, definía una macro distinta de `__STDC_WANT_LIB_EXT1__`, sin ningún efecto. Además, el código no usaba funciones del Anexo K (`snprintf` es C estándar), la biblioteca de macOS no implementa ese anexo y, al empezar por `__`, el identificador está reservado (DCL37-C). La línea se eliminó por sugerencia de Claude, mientras se le solicitó la construcción de este README.

### 9.5 Tiempo dedicado

|Etapa|Tiempo aproximado|
|---|---|
|Fase 1: compilación, comprensión del código y corrección de errores y warnings|4 h|
|Fase 2: auditoría CERT, correcciones y documentación del README|3 h 30 min|
|**Total**|**7 h 30 min**|
