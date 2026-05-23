/**
  ******************************************************************************
  * @file    main.c
  * @brief   Bootloader OTA STM32F411 — entry point
  ******************************************************************************
  */

#include "main.h"
#include "uart_log.h"
#include "boot.h"
#include "crc.h"
#include "flash_io.h"
#include "metadata.h"
#include "ota_proto.h"

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    LOG_BOOT("=== Bootloader v1 ===");

#ifdef BOOTSTRAP_METADATA
    {
        LOG_BOOT(">>> BOOTSTRAP_METADATA mode <<<");

        const uint32_t app_size = 44932;
        uint32_t app_crc = crc32_of_flash(SLOT_A_ADDR, app_size);
        LOG_BOOT("computed slot A CRC = 0x%08lX (size=%lu)",
                 (unsigned long)app_crc, (unsigned long)app_size);

        bootloader_meta_t m = {
            .magic            = META_MAGIC,
            .version_meta     = META_VERSION,
            .active_slot      = SLOT_A,
            .boot_count       = 0,
            .slot_a_version   = 0x00010000U,
            .slot_a_size      = app_size,
            .slot_a_crc       = app_crc,
            .slot_a_validated = 1,
            .slot_b_version   = 0,
            .slot_b_size      = 0,
            .slot_b_crc       = 0,
            .slot_b_validated = 0,
        };
        HAL_StatusTypeDef st = meta_write(&m);
        LOG_BOOT("meta_write returned %d, halting", st);
        while (1) { HAL_Delay(1000); }
    }
#endif

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /* 1. Lire et valider la metadata */
    bootloader_meta_t meta;
    meta_read(&meta);

    if (!meta_validate(&meta)) {
        LOG_BOOT("metadata invalide, halt");
        while (1) {
            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            HAL_Delay(200);
        }
    }
    LOG_BOOT("metadata OK, active_slot=%c", meta.active_slot == SLOT_A ? 'A' : 'B');

    /* 2. Choisir le slot actif */
    uint32_t slot_addr = (meta.active_slot == SLOT_A) ? SLOT_A_ADDR : SLOT_B_ADDR;
    uint32_t slot_size = (meta.active_slot == SLOT_A) ? meta.slot_a_size : meta.slot_b_size;
    uint32_t slot_crc  = (meta.active_slot == SLOT_A) ? meta.slot_a_crc  : meta.slot_b_crc;

    if (slot_size == 0) {
        LOG_BOOT("slot %c vide, halt", meta.active_slot == SLOT_A ? 'A' : 'B');
        while (1) { HAL_Delay(1000); }
    }

    /* 3. Fenêtre OTA (2 secondes pour envoyer 'U') */
    LOG_BOOT("waiting 2s for OTA trigger ('U' on UART)...");
    if (ota_check_enter_mode(2000)) {
        ota_run_session();
        LOG_BOOT("OTA session ended unexpectedly, resetting");
        HAL_Delay(100);
        NVIC_SystemReset();
    }

    /* 4. Vérification CRC du firmware */
    uint32_t computed = crc32_of_flash(slot_addr, slot_size);
    if (computed != slot_crc) {
        LOG_BOOT("CRC MISMATCH slot %c : calc=%08lX expected=%08lX",
                 meta.active_slot == SLOT_A ? 'A' : 'B',
                 (unsigned long)computed, (unsigned long)slot_crc);
        while (1) {
            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            HAL_Delay(100);
        }
    }
    LOG_BOOT("CRC OK slot %c (%08lX)",
             meta.active_slot == SLOT_A ? 'A' : 'B', (unsigned long)computed);

    /* 5. Petit clignotement avant le saut */
    for (int i = 0; i < 4; i++) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(500);
    }
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /* 6. Saut vers l'application */
    jump_to_app(slot_addr);

    /* Jamais atteint */
    while (1) { }
}


void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}


static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}


static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
}


void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}


#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif