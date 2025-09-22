/*
 * Copyright (c) 2025
 * Regis Rousseau
 * Univ Lyon, INSA Lyon, Inria, CITI, EA3720
 * SPDX-License-Identifier: Apache-2.0
 */

//  ========== includes ====================================================================
#include "app_eeprom.h"

//  ========== app_eeprom_init =============================================================
int8_t app_eeprom_init(const struct device *dev)
{
	// check if the EEPROM device is ready
	if (!device_is_ready(dev)) {
		printk("%s: device is not ready\n", dev->name);
		return -1;
	}

	// erase the data storage partition
	int8_t ret = flash_erase(dev, SPI_FLASH_OFFSET, SPI_FLASH_SECTOR_SIZE*SPI_FLASH_SECTOR_NB);
	if (ret != 0){
		printf("MX25R64 flash erase failed. error: %d\n", ret);
		return -1;
	} else {
		printk("MX25R64 flash erase succeeded\n");
	}	
	return 1;
}

//  ========== app_eeprom_write ============================================================
// write data to EEPROM
int8_t app_eeprom_write(const struct device *dev, const uint8_t *data, size_t length)
{
    int8_t ret = flash_write(dev, SPI_FLASH_OFFSET, data, length);
    if (ret != 0) {
        printk("error writing data. Error: %d\n", ret);
        return -1;
    }
    printk("successfully wrote %zu bytes to address 0x%X\n", length, SPI_FLASH_OFFSET);
    return 0;
}

//  ========== app_rom_read ================================================================
// read data from EEPROM
int8_t app_eeprom_read(const struct device *dev, uint8_t *data, size_t length)
{
    int ret = flash_read(dev, SPI_FLASH_OFFSET, data, length);
    if (ret != 0) {
        printk("error reading data. Error: %d\n", ret);
        return -1;
    }
    printk("successfully read %zu bytes from address 0x%X\n", length, SPI_FLASH_OFFSET);
    return 0;
}

//  ======== app_rom_handler ===============================================================
int8_t app_eeprom_handler(const struct device *dev)
{
    uint8_t buffer[2*MAX_RECORDS] = {0};        // enough space for tADC data
    int16_t adc_data[MAX_RECORDS] = {0};

    if (!device_is_ready(dev)) {
        printk("%s: device is not ready\n", dev->name);
        return -1;
    }

    // get ADC data and serialize it into the buffer
    for (int i = 0; i < MAX_RECORDS; i++) {
        adc_data[i] = app_nrf52_get_adc();
        buffer[i * 2] = (adc_data[i] >> 8) & 0xFF;     // high byte
        buffer[i * 2 + 1] = adc_data[i] & 0xFF;        // low byte
    }

    // write buffer to EEPROM
    if (app_eeprom_write(dev, buffer, sizeof(buffer)) != 0) {
        return -1;
    }

    k_sleep(K_MSEC(2000));

    // read back and verify
    uint8_t read_buffer[256] = {0};
    if (app_eeprom_read(dev, read_buffer, sizeof(read_buffer)) != 0) {
        return -1;
    }

    // deserialize and print ADC data
    for (int i = 0; i < MAX_RECORDS; i++) {
        int16_t read_adc = (read_buffer[i * 2] << 8) | read_buffer[i * 2 + 1];
        printk("read ADC value [%d]: %d\n", i, read_adc);
    }
    return 0;
}


