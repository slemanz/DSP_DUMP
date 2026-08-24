#include "driver_flash.h"

void flash_set_program_size(uint32_t psize)
{
    FLASH->CR &= ~(FLASH_CR_PSIZE_MSK);
    FLASH->CR |=  (psize << FLASH_CR_PSIZE_POS);
}

void flash_wait_for_last_operation(void)
{
    while((FLASH->SR & FLASH_SR_BSY_MSK));
}

void flash_unlock_cr(void)
{
    FLASH->KEYR = FLASH_KEY1_CR;
    FLASH->KEYR = FLASH_KEY2_CR;
}

void flash_unlock_write(void)
{
    FLASH->OPTKEYR = FLASH_OPTKEY1;
    FLASH->OPTKEYR = FLASH_OPTKEY2;
}

void flash_lock_cr(void)
{
    FLASH->CR |= (1U << 31U);
}

void flash_lock_write(void)
{
    FLASH->OPTCR |= (1U << 0);
}

void flash_program_double_word(uint32_t address, uint64_t data)
{
    flash_wait_for_last_operation();
    flash_set_program_size(FLASH_PSIZE_X64);

    FLASH->CR |= (1U << 0);
    MMIO64(address) = data;

    flash_wait_for_last_operation();
    FLASH->CR &= ~(1U << 0);
}

void flash_program_word(uint32_t address, uint32_t data)
{
    flash_wait_for_last_operation();
    flash_set_program_size(FLASH_PSIZE_X32);

    FLASH->CR |= (1U << 0);
    MMIO32(address) = data;

    flash_wait_for_last_operation();
    FLASH->CR &= ~(1U << 0);
}

void flash_program_half_word(uint32_t address, uint16_t data)
{
    flash_wait_for_last_operation();
    flash_set_program_size(FLASH_PSIZE_X16);

    FLASH->CR |= (1U << 0);
    MMIO16(address) = data;

    flash_wait_for_last_operation();
    FLASH->CR &= ~(1U << 0);
}

void flash_program_byte(uint32_t address, uint8_t data)
{
    flash_wait_for_last_operation();
    flash_set_program_size(FLASH_PSIZE_X8);

    FLASH->CR |= (1U << 0);
    MMIO8(address) = data;

    flash_wait_for_last_operation();
    FLASH->CR &= ~(1U << 0);
}

void flash_program(uint32_t address, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
    {
    	flash_program_byte(address+i, data[i]);
    }
}

void flash_erase_sector(uint32_t sector)
{

    flash_wait_for_last_operation();
    flash_set_program_size(FLASH_PSIZE_X32);

    if(sector > 7)
    {
        return;
    }

    FLASH->CR &= ~(0xF << 3);
    FLASH->CR |= (sector << 3);
    FLASH->CR |= (1U << 1);
    FLASH->CR |= (1U << 16);

    flash_wait_for_last_operation();
    FLASH->CR &= ~(1U << 1);
	FLASH->CR &= ~(0xF << 3);
}

void flash_erase_sectors(uint32_t sector, uint32_t Len)
{
    uint32_t length = Len;
    uint32_t sector_erase = sector;

    do
    {
        length--;
        flash_erase_sector(sector_erase);
        sector_erase++;
    } while(length);
}

void flash_set_latency(uint32_t wait_states)
{
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_MSK)
               | (wait_states & FLASH_ACR_LATENCY_MSK);

    /* the write is posted, so read it back before going any faster */
    while ((FLASH->ACR & FLASH_ACR_LATENCY_MSK) != (wait_states & FLASH_ACR_LATENCY_MSK))
    {
    }
}

uint32_t flash_get_latency(void)
{
    return FLASH->ACR & FLASH_ACR_LATENCY_MSK;
}

void flash_art_config(uint8_t prefetch, uint8_t icache, uint8_t dcache)
{
    uint32_t acr = FLASH->ACR;

    acr &= ~(FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    FLASH->ACR = acr;

    FLASH->ACR |= (FLASH_ACR_ICRST | FLASH_ACR_DCRST);
    FLASH->ACR &= ~(FLASH_ACR_ICRST | FLASH_ACR_DCRST);

    if (prefetch == ENABLE) { acr |= FLASH_ACR_PRFTEN; }
    if (icache   == ENABLE) { acr |= FLASH_ACR_ICEN; }
    if (dcache   == ENABLE) { acr |= FLASH_ACR_DCEN; }

    FLASH->ACR = acr;
}