#include "main.h"
#include <math.h>
#include <stdlib.h> 
#include <string.h> 

/* --- Function Prototypes --- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

void LCD_DrawMap(uint8_t *buffer);
void LCD_InitGraphics(void);
void ClearMapBuffer(void);
void DrawPixelInMap(uint8_t x, uint8_t y, uint8_t color);

void applegenerator(void);
void update_map(void);
void init_game(void);
void move_snake(void);

/* --- LCD ST7565 Hardware Defines --- */
#define LCD_A0_PORT GPIOB  // A0 replaces RS
#define LCD_A0_PIN  GPIO_PIN_0

#define LCD_RW_PORT GPIOB
#define LCD_RW_PIN  GPIO_PIN_1

#define LCD_E_PORT  GPIOB
#define LCD_E_PIN   GPIO_PIN_2

#define LCD_DATA_PORT GPIOC

/* --- Game Variables --- */
int8_t  idx = 1;
int8_t  idy = 0;

// The buffer is still 1024 bytes (128 columns * 8 pages = 1024)
uint8_t SnakeMapBuffer[1024];

uint8_t apple_idx = 0;
uint8_t apple_idy = 0;
int8_t is_apple_generated = 0;

#define MAX_SNAKE_LENGTH 100
uint8_t snake_X[MAX_SNAKE_LENGTH];
uint8_t snake_Y[MAX_SNAKE_LENGTH];
uint8_t snake_length = 5;

/* --- Main Function --- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // Initialize Screen with ST7565 Protocol
    LCD_InitGraphics();
    
    // Setup initial snake coordinates and draw the first frame
    init_game();

    // Variable to control game speed
    uint32_t last_move_time = HAL_GetTick();

    while (1)
    {
        // Execute the game logic loop every 150ms
        if (HAL_GetTick() - last_move_time >= 25) 
        {
            move_snake();
            last_move_time = HAL_GetTick();
        }
    }
}

/* --- Core Game Logic --- */

void init_game(void)
{
    snake_length = 5;
    
    for(int i = 0; i < snake_length; i++) 
    {
        snake_X[i] = 64 - i; 
        snake_Y[i] = 32;     
    }
    
    idx = 1; 
    idy = 0;
    
    HAL_Delay(10); 
    srand(HAL_GetTick()); 
    
    applegenerator();     
    
    update_map();
    LCD_DrawMap(SnakeMapBuffer);
}

void move_snake(void) 
{
    for (int i = snake_length - 1; i > 0; i--) 
    {
        snake_X[i] = snake_X[i - 1];
        snake_Y[i] = snake_Y[i - 1];
    }
    
    snake_X[0] += idx;
    snake_Y[0] += idy;
    
    if (snake_X[0] == 255) snake_X[0] = 127; 
    else if (snake_X[0] >= 128) snake_X[0] = 0;   
    
    if (snake_Y[0] == 255) snake_Y[0] = 63;  
    else if (snake_Y[0] >= 64) snake_Y[0] = 0;    
    
    if (snake_X[0] == apple_idx && snake_Y[0] == apple_idy) 
    {
        is_apple_generated = 0; 
        
        if (snake_length < MAX_SNAKE_LENGTH) {
            snake_length++;
        }
        
        applegenerator(); 
    }
    
    update_map();
    LCD_DrawMap(SnakeMapBuffer); 
}

void applegenerator(void) 
{
    if (is_apple_generated == 0) 
    {
        uint8_t valid_position = 0;
        while (valid_position == 0) 
        {
            apple_idx = rand() % 128; 
            apple_idy = rand() % 64;  
            valid_position = 1; 
            
            for (int i = 0; i < snake_length; i++) 
            {
                if (apple_idx == snake_X[i] && apple_idy == snake_Y[i]) 
                {
                    valid_position = 0; 
                    break;
                }
            }
        }
        is_apple_generated = 1;
    }
}

void update_map(void) 
{
    ClearMapBuffer();
    if (is_apple_generated == 1) {
        DrawPixelInMap(apple_idx, apple_idy, 1);
    }
    for (int i = 0; i < snake_length; i++) {
        DrawPixelInMap(snake_X[i], snake_Y[i], 1);
    }
}

/* --- Interrupt Callbacks --- */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = HAL_GetTick();
    
    if (interrupt_time - last_interrupt_time > 50) 
    {
        if (GPIO_Pin == GPIO_PIN_0)        { if (idx != -1) { idx = 1; idy = 0; } }
        else if (GPIO_Pin == GPIO_PIN_3)   { if (idx != 1) { idx = -1; idy = 0; } }
        else if (GPIO_Pin == GPIO_PIN_6)   { if (idy != 1) { idx = 0; idy = -1; } }
        else if (GPIO_Pin == GPIO_PIN_9)   { if (idy != -1) { idx = 0; idy = 1; } }
    }
    last_interrupt_time = interrupt_time;
}

/* --- LCD Driver Functions (NEW ST7565 Logic) --- */

void LCD_PulseEnable(void) 
{
    HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_SET);
    for(volatile int i = 0; i < 200; i++); 
    HAL_GPIO_WritePin(LCD_E_PORT, LCD_E_PIN, GPIO_PIN_RESET);
    for(volatile int i = 0; i < 200; i++); 
}

void LCD_WriteBus8(uint8_t v) 
{
    GPIOC->ODR = (GPIOC->ODR & 0xFF00) | v; 
}

void LCD_SendCommand(uint8_t cmd) 
{
    HAL_GPIO_WritePin(LCD_A0_PORT, LCD_A0_PIN, GPIO_PIN_RESET); // A0 = 0 for Command
    HAL_GPIO_WritePin(LCD_RW_PORT, LCD_RW_PIN, GPIO_PIN_RESET);
    LCD_WriteBus8(cmd);
    LCD_PulseEnable();
}

void LCD_SendData(uint8_t data) 
{
    HAL_GPIO_WritePin(LCD_A0_PORT, LCD_A0_PIN, GPIO_PIN_SET);   // A0 = 1 for Data
    HAL_GPIO_WritePin(LCD_RW_PORT, LCD_RW_PIN, GPIO_PIN_RESET);
    LCD_WriteBus8(data);
    LCD_PulseEnable();
}

void LCD_InitGraphics(void) 
{
    HAL_Delay(50); // Power on delay

    // ST7565 Initialization Sequence
    LCD_SendCommand(0xE2); // Software Reset
    HAL_Delay(5);
    
    LCD_SendCommand(0xA2); // 1/9 Bias ratio
    LCD_SendCommand(0xA0); // ADC Normal (Left to Right mapping)
    LCD_SendCommand(0xC8); // COM Output Reverse (Top to Bottom mapping)
    
    LCD_SendCommand(0x2F); // Power Control: Booster, Regulator, Follower ON
    HAL_Delay(5);
    
    LCD_SendCommand(0x26); // Resistor ratio
    
    LCD_SendCommand(0x81); // Electronic Volume Mode Set (Contrast)
    LCD_SendCommand(0x1F); // Contrast value (0x00 to 0x3F)
    
    LCD_SendCommand(0xAF); // Display ON
    LCD_SendCommand(0x40); // Set Start Line to 0
}

void LCD_DrawMap(uint8_t *buffer) 
{
    // The ST7565 uses 8 "Pages" (Each page is 8 pixels tall)
    for (uint8_t page = 0; page < 8; page++) 
    {
        LCD_SendCommand(0xB0 | page); // Set Page Address (0 to 7)
        LCD_SendCommand(0x10);        // Set Column Address MSB (0)
        LCD_SendCommand(0x00);        // Set Column Address LSB (0)

        // Send 128 bytes for this specific horizontal page
        for (uint8_t col = 0; col < 128; col++) 
        {
            // Calculate index in our 1024-byte buffer
            uint16_t buffer_index = (page * 128) + col;
            LCD_SendData(buffer[buffer_index]);
        }
    }
}

void ClearMapBuffer(void) 
{
    memset(SnakeMapBuffer, 0x00, sizeof(SnakeMapBuffer));
}

void DrawPixelInMap(uint8_t x, uint8_t y, uint8_t color) 
{
    if (x >= 128 || y >= 64) return; 

    // ST7565 Vertical Math Mapping:
    uint8_t page = y / 8;               // Find which of the 8 pages (rows of bytes) it is in
    uint16_t byteIndex = (page * 128) + x; // Find the exact byte in that page
    uint8_t bitIndex = y % 8;           // Find the specific vertical bit in that byte

    if (color) {
        SnakeMapBuffer[byteIndex] |= (1 << bitIndex);  // Turn pixel ON
    } else {
        SnakeMapBuffer[byteIndex] &= ~(1 << bitIndex); // Turn pixel OFF
    }
}

/* --- Hardware Setup Configuration --- */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_3|GPIO_PIN_6|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP; 
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void Error_Handler(void) { __disable_irq(); while (1) { } }
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { }
#endif
