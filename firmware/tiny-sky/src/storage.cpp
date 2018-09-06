#include "storage.h"
#include "settings.h"

#include <ptlib/ptlib.h>

#include <libopencm3/stm32/flash.h>

int eeprom_write(uint32_t address, const uint32_t *data, int length_in_words)
{
    // Wait until FLASH is not busy
    uint32_t time_start;

    time_start = millis();
    while (FLASH_SR & FLASH_SR_BSY) {
        if (millis() - time_start > 100) return -1;
    }

	if ((FLASH_PECR & FLASH_PECR_PELOCK) != 0) {
        flash_unlock_pecr();	
        if (FLASH_PECR & FLASH_PECR_PELOCK) return -2;
    }

	/* erase only if needed */
	FLASH_PECR &= ~FLASH_PECR_FTDW;
	for (int i = 0; i < length_in_words; i++) {
		MMIO32(address + (i * sizeof(uint32_t))) = data[i];
        
        time_start = millis();
		while (FLASH_SR & FLASH_SR_BSY) {
            if (millis() - time_start > 100) return -1;
        }
	}

    delay(100);
	flash_lock_pecr();
    return 0;
}

int extflash_write(uint32_t address, const uint8_t *buffer, int size) {
    const uint8_t *wr_buf = (const uint8_t *)buffer;

    uint32_t page_remaining = 256 - (address & 0xFF);
    while (size > 0) {
        int wr_len = (size < page_remaining) ? size : page_remaining;
        while (gState.flash.busy()) {
            // idle wait
        }
        gState.flash.programPage(address, wr_buf, wr_len);
        address += wr_len;
        wr_buf += wr_len;
        size -= wr_len;
        page_remaining = 256;
    }
    return 0;
}

int extflash_read(uint32_t address, uint8_t *buffer, int size) {
    while (gState.flash.busy()) {
        // idle wait
    }
    gState.flash.read(address, buffer, size);
    return 0;
}

static    uint32_t            log_size;

int xlog_init() {
    uint32_t high = 0x200000 - 1;
    uint32_t low  = 0x2000;
    uint8_t  buffer[16];

    while (low <= high) {
        uint32_t middle = low + (high - low) / 2;

        extflash_read(middle, buffer, sizeof(buffer));
        bool empty = true;
        for (int i = 0; i < sizeof(buffer); i++) {
            if (buffer[i] != 0xFF) {
                empty = false;
                break;
            }
        }

        if (empty) {
            high = middle - 1;
        }
        else {
            low = middle + 1;
        }
    }
    log_size = low - 0x2000;

    // // mount the filesystem
    // int err = lfs_mount(&lfs, &lfs_cfg);

    // // reformat if we can't mount the filesystem
    // // this should only happen on the first boot
    // if (err) {
    //     lfs_format(&lfs, &lfs_cfg);
    //     err = lfs_mount(&lfs, &lfs_cfg);
    //     if (err) return -2;
    // }

    return 0;    
}

int xlog_erase_log() {
    while (gState.flash.busy()) {
        // idle wait
    }
    for (uint32_t address = 0x2000; address < 0x2000 + log_size; address += 0x1000) {
        gState.flash.eraseSector(address);
        while (gState.flash.busy()) {
            // idle wait
        }
    }
    log_size = 0;    
}

int xlog_free_space() {
    return 0x200000 - (0x1000 + log_size);
}

int xlog_used_space() {
    return log_size;
}

void xlog_arm(uint32_t timestamp, uint8_t hour, uint8_t minute, uint8_t second) {
    uint8_t buffer[] = {
        0x00,
        (uint8_t)(timestamp >>  0),
        (uint8_t)(timestamp >>  8),
        (uint8_t)(timestamp >> 16),
        (uint8_t)(timestamp >> 24),
        hour,
        minute,
        second
    };
    extflash_write(0x2000 + log_size, buffer, sizeof(buffer));
    log_size += sizeof(buffer);    
}

void xlog_mag(uint32_t timestamp, int16_t mag_x, int16_t temp_q4) {
    uint8_t buffer[] = {
        0x02,
        (uint8_t)(timestamp >>  0),
        (uint8_t)(timestamp >>  8),
        (uint8_t)(timestamp >> 16),
        (uint8_t)(timestamp >> 24),
        (uint8_t)(mag_x >>  0),
        (uint8_t)(mag_x >>  8),
        (uint8_t)((temp_q4 + 8) >> 4)
    };
    //lfs_file_write(&lfs, &log_file, buffer, sizeof(buffer));
    extflash_write(0x2000 + log_size, buffer, sizeof(buffer));
    log_size += sizeof(buffer);
}

void xlog_baro(uint32_t timestamp, uint32_t pressure_q4, int16_t temp_q4) {
    // Convert to units of 2 Pa
    uint16_t pressure2 = ((pressure_q4 + 8) >> 4) / 2;
    uint8_t buffer[] = {
        0x01,
        (uint8_t)(timestamp >>  0),
        (uint8_t)(timestamp >>  8),
        (uint8_t)(timestamp >> 16),
        (uint8_t)(timestamp >> 24),
        (uint8_t)(pressure2 >>  0),
        (uint8_t)(pressure2 >>  8),
        (uint8_t)((temp_q4 + 8) >> 4)
    };
    //lfs_file_write(&lfs, &log_file, buffer, sizeof(buffer));
    extflash_write(0x2000 + log_size, buffer, sizeof(buffer));
    log_size += sizeof(buffer);    
}

void xlog_pos(uint32_t timestamp, float latitude, float longitude, float altitude) {
    uint16_t alt = altitude;
    uint32_t lat = latitude * 10 * 1000 * 1000;
    uint32_t lon = longitude * 10 * 1000 * 1000;
    uint8_t buffer[] = {
        0x04,
        (uint8_t)(timestamp >>  0),
        (uint8_t)(timestamp >>  8),
        (uint8_t)(timestamp >> 16),
        (uint8_t)(timestamp >> 24),
        (uint8_t)(alt >> 0),
        (uint8_t)(alt >> 8),
        (uint8_t)(lat >>  0),
        (uint8_t)(lat >>  8),
        (uint8_t)(lat >> 16),
        (uint8_t)(lat >> 24),
        (uint8_t)(lon >>  0),
        (uint8_t)(lon >>  8),
        (uint8_t)(lon >> 16),
        (uint8_t)(lon >> 24)
    };
    extflash_write(0x2000 + log_size, buffer, sizeof(buffer));
    log_size += sizeof(buffer);   
}

void xlog_eject(uint32_t timestamp) {
    uint8_t buffer[] = {
        0x10,
        (uint8_t)(timestamp >>  0),
        (uint8_t)(timestamp >>  8),
        (uint8_t)(timestamp >> 16),
        (uint8_t)(timestamp >> 24)
    };
    extflash_write(0x2000 + log_size, buffer, sizeof(buffer));
    log_size += sizeof(buffer);   
}

// extern "C" {
//     static int spi_flash_lfs_read(const struct lfs_config *c, lfs_block_t block, 
//         lfs_off_t off, void *buffer, lfs_size_t size);

//     static int spi_flash_lfs_prog(const struct lfs_config *c, lfs_block_t block, 
//         lfs_off_t off, const void *buffer, lfs_size_t size);

//     static int spi_flash_lfs_erase(const struct lfs_config *c, lfs_block_t block);

//     static int spi_flash_lfs_sync(const struct lfs_config *c);
// }


// #define LFS_BLOCK_SIZE      4096
// #define LFS_BLOCK_COUNT     512         // 16 Mbit

// #define LFS_READ_SIZE       16
// #define LFS_PROG_SIZE       (2 * LFS_READ_SIZE)

// uint8_t lfs_read_buffer[LFS_READ_SIZE];
// uint8_t lfs_prog_buffer[LFS_PROG_SIZE];
// uint8_t lfs_file_buffer[LFS_PROG_SIZE];
// uint8_t lfs_lookahead_buffer[16];

// // configuration of the filesystem is provided by this struct
// const lfs_config lfs_cfg = {
//     .context = 0,

//     // block device operations
//     .read  = spi_flash_lfs_read,
//     .prog  = spi_flash_lfs_prog,
//     .erase = spi_flash_lfs_erase,
//     .sync  = spi_flash_lfs_sync,

//     // block device configuration
//     .read_size = LFS_READ_SIZE,    // Minimum size of a block read
//     .prog_size = LFS_PROG_SIZE,    // Minimum size of a block program
//     .block_size = LFS_BLOCK_SIZE,  // Size of an erasable block, N*prog_size
//     .block_count = LFS_BLOCK_COUNT,// Number of erasable blocks on the device
//     .lookahead = 128, // Number of blocks to lookahead during block allocation, N*32

//     .read_buffer = lfs_read_buffer,
//     .prog_buffer = lfs_prog_buffer,
//     .lookahead_buffer = lfs_lookahead_buffer,
//     .file_buffer = lfs_file_buffer
// };



// static int spi_flash_lfs_read(const struct lfs_config *c, lfs_block_t block, 
//     lfs_off_t off, void *buffer, lfs_size_t size) 
// {
//     uint32_t address = (block * LFS_BLOCK_SIZE) | off;

//     while (gState.flash.busy()) {
//         // idle wait
//     }
//     gState.flash.read(address, (uint8_t *)buffer, size);
//     return 0;
// }

// static int spi_flash_lfs_prog(const struct lfs_config *c, lfs_block_t block, 
//     lfs_off_t off, const void *buffer, lfs_size_t size) 
// {
//     uint32_t address = (block * LFS_BLOCK_SIZE) | off;
//     const uint8_t *wr_buf = (const uint8_t *)buffer;

//     uint32_t page_remaining = 256 - (address & 0xFF);
//     while (size > 0) {
//         int wr_len = (size < page_remaining) ? size : page_remaining;
//         while (gState.flash.busy()) {
//             // idle wait
//         }
//         gState.flash.programPage(address, wr_buf, wr_len);
//         address += wr_len;
//         wr_buf += wr_len;
//         size -= wr_len;
//         page_remaining = 256;
//     }
//     return 0;
// }

// static int spi_flash_lfs_erase(const struct lfs_config *c, lfs_block_t block) 
// {
//     uint32_t address = (block * LFS_BLOCK_SIZE);

//     while (gState.flash.busy()) {
//         // idle wait
//     }
//     gState.flash.eraseSector(address);
//     return 0;
// }

// static int spi_flash_lfs_sync(const struct lfs_config *c) {
//     while (gState.flash.busy()) {
//         // idle wait
//     }
//     // Nothing to sync here, indicate success
//     return 0;
// }

// static int _traverse_df_cb(void *p, lfs_block_t block){
// 	uint32_t *nb = (uint32_t *)p;
// 	*nb += 1;
// 	return 0;
// }

// int AppState::free_space(){
	// uint32_t n_allocated = 0;
	// int err = lfs_traverse(&lfs, _traverse_df_cb, &n_allocated);
	// if (err < 0) {
	// 	return err;
	// }
// }