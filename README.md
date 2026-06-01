# STM32 Proteus Snake Game

A fun embedded-systems project that runs a simple **Snake game** on an STM32F401RE microcontroller inside **Proteus**. The game uses four GPIO input buttons for movement and a 128x64 graphical LCD to display the snake, apple, and game map.

This project was made for practice and experimentation after working with STM32, GPIO, interrupts, and LCD control. It is not a fully polished game engine, but the main gameplay works: the snake moves around the screen, changes direction using buttons, wraps around the display edges, eats apples, and grows in length.

## Project Overview

The project uses an STM32F401RE microcontroller and a graphical LCD based on the ST7565-style display protocol. Instead of using a text LCD, this project uses a larger pixel-based LCD so the game has enough space to draw the snake and apple.

The LCD is controlled manually through GPIO pins. The firmware keeps a full display buffer in memory, updates the snake and apple positions, and sends the buffer to the LCD repeatedly.

## Main Features

- Snake game implemented in embedded C
- Runs on STM32F401RE
- Simulated in Proteus
- 128x64 graphical LCD display
- 1024-byte screen buffer
- Four GPIO buttons for direction control
- External interrupt input handling using EXTI
- Button debounce logic
- Apple generation with random position
- Snake growth after eating an apple
- Screen wrap-around when the snake reaches the display border
- Manual LCD driver implementation without a ready-made graphics library

## Hardware / Simulation Setup

| Component | Usage |
|---|---|
| STM32F401RE | Main microcontroller |
| Proteus | Simulation environment |
| ST7565-style 128x64 graphical LCD | Game display |
| GPIOA PA0 | Move right button |
| GPIOA PA3 | Move left button |
| GPIOA PA6 | Move up button |
| GPIOA PA9 | Move down button |
| GPIOC PC0-PC7 | LCD 8-bit data bus |
| GPIOB PB0 | LCD A0 / command-data select |
| GPIOB PB1 | LCD R/W control |
| GPIOB PB2 | LCD enable pin |

## Display Buffer Design

The graphical LCD resolution is:

```text
Width  = 128 pixels
Height = 64 pixels
```

The ST7565-style LCD memory is divided into 8 pages. Each page is 8 pixels high and 128 pixels wide.

```text
Number of pages = 64 / 8 = 8 pages
Bytes per page  = 128 bytes
Total buffer    = 128 * 8 = 1024 bytes
```

In the code, the screen buffer is defined as:

```c
uint8_t SnakeMapBuffer[1024];
```

Each pixel is mapped into this buffer using:

```text
page      = y / 8
byteIndex = page * 128 + x
bitIndex  = y % 8
```

Then the selected bit is turned on or off to draw a pixel.

## Game Logic

The snake position is stored in two arrays:

```c
#define MAX_SNAKE_LENGTH 100
uint8_t snake_X[MAX_SNAKE_LENGTH];
uint8_t snake_Y[MAX_SNAKE_LENGTH];
uint8_t snake_length = 5;
```

The first element of the arrays is the snake head:

```text
snake_X[0], snake_Y[0]
```

During each game update:

1. Every body segment takes the position of the segment before it.
2. The snake head moves based on the current direction.
3. If the head crosses the screen border, it wraps to the opposite side.
4. If the head reaches the apple position, the snake length increases.
5. A new apple is generated at a random valid position.
6. The frame buffer is cleared and redrawn.
7. The updated buffer is sent to the LCD.

## Direction Control

The project uses four input pins configured as external interrupts:

| Pin | Direction |
|---|---|
| PA0 | Right |
| PA3 | Left |
| PA6 | Up |
| PA9 | Down |

The direction is stored using two variables:

```c
int8_t idx = 1;
int8_t idy = 0;
```

For example:

```text
Right: idx = 1,  idy = 0
Left:  idx = -1, idy = 0
Up:    idx = 0,  idy = -1
Down:  idx = 0,  idy = 1
```

The code also prevents direct reverse movement. For example, if the snake is moving right, it cannot immediately move left.

## LCD Driver

The LCD is controlled directly using GPIO pins:

```c
#define LCD_A0_PORT GPIOB
#define LCD_A0_PIN  GPIO_PIN_0

#define LCD_RW_PORT GPIOB
#define LCD_RW_PIN  GPIO_PIN_1

#define LCD_E_PORT  GPIOB
#define LCD_E_PIN   GPIO_PIN_2

#define LCD_DATA_PORT GPIOC
```

The driver sends commands and data using an 8-bit parallel bus. The screen is refreshed page by page:

```text
8 pages * 128 columns = 1024 bytes per full refresh
```

## Firmware Flow

```text
Start
  |
  v
Initialize HAL, system clock, and GPIO
  |
  v
Initialize graphical LCD
  |
  v
Initialize snake position and generate first apple
  |
  v
Main loop checks game timing
  |
  v
Move snake
  |
  v
Check apple collision
  |
  v
Update frame buffer
  |
  v
Draw frame buffer on LCD
  |
  v
Repeat
```

## Known Issue

Sometimes there is a small lag or visual glitch where the snake tail appears to grow even when the apple was not clearly eaten. The game still works overall, but this behavior can happen during simulation.

Possible reasons:

- The LCD refresh is slow because the full 1024-byte buffer is sent through GPIO every frame.
- The game loop currently moves the snake very quickly.
- The apple is only one pixel, so the snake may eat it even if it is not visually obvious.
- When the snake grows, the new tail segment could be initialized more carefully.
- Proteus simulation timing may not perfectly match real hardware behavior.

A possible improvement is to reduce the game speed, update only changed pixels, or initialize the new tail position based on the previous tail before increasing `snake_length`.

## Important Source Files

| File | Description |
|---|---|
| `snake/Core/Src/main.c` | Main game logic, LCD driver, GPIO setup, and interrupt callback |
| `snake/Core/Inc/main.h` | Main STM32 header file |
| `snake/snake.ioc` | STM32CubeMX project configuration |
| `snake/MDK-ARM/snake.uvprojx` | Keil uVision project file |
| `snake.pdsprj` | Proteus simulation project |

## How to Build

1. Open the Keil project:

```text
snake/MDK-ARM/snake.uvprojx
```

2. Build the project in Keil uVision.
3. Use the generated output file in the Proteus simulation if needed.
4. Open the Proteus project:

```text
snake.pdsprj
```

5. Run the simulation and control the snake using the four input buttons.

