# MAC_LAB4

TP Lab 4 - Microcontroleurs : architecture et communication (Centrale Nantes, ITII).
Cible : Nucleo-32 STM32F303K8.

## Structure

- `lab4/` : code du TP (`main.c`, `CMakeLists.txt`, enonce `lab4.pdf`, config `.vscode`)
- `sys/` : startup, linker script, CMSIS, scripts CMake (depot du cours)
- `lib/` : bibliotheques du cours (`pinAccess`, `adc`, `pwm`, `spi`, `exti`)

## Compiler

Ouvrir le dossier `lab4/` dans VS Code (extensions CMake Tools + Cortex-Debug), puis `Ctrl+Shift+B`.

En ligne de commande (toolchain STM32CubeCLT dans le PATH) :

```
cd lab4
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target bin
```

Flash : tache VS Code « Flash lab4 (ST-Link) » (STM32_Programmer_CLI), debug : F5 (Cortex-Debug, ST-Link).
