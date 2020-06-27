#include "gmac.h"
#include "hardware.h"
#include "cpu.h"
#include "print.h"

#define GMAC_TX_BUFFER_SIZE 1500
#define GMAC_RX_BUFFER_SIZE 128
#define GMAC_TX_DESCRIPTORS 2
#define GMAC_RX_DESCRIPTORS 16

/// Receive buffer descriptor located in system memory
struct gmac_rx_desc {
    // Bits [31..2]  marks the address of the buffer
    // Bit  [1]      marks the last descriptor in the buffer list
    // Bit  [0]      marks ownership. This bit MUST be zero if for the GMAC to
    //               transfer data to the buffer. The GMAC sets this bit to one
    //               after a transfer
    u32 addr;

    // Bit  [31]     broadcase address detected
    // Bit  [30]     multicast hash match
    // Bit  [29]     unicast hash match
    // Bit  [27]     specific address found
    // Bits [26..25] specifc address match - specifies the register in binary
    // Bit  [24]     type ID register match if RX checksum offloading is 
    //               enabled, otherwise - indicates snap encoding
    // Bits [32..22] type ID register encoded in binary if RX checksum offloading
    //               is enabled, otherwise - checksum status
    // Bit  [21]     VLAN tag detected
    // Bit  [20]     priority tag detected
    // Bits [19..17] VLAN priority 
    // Bit  [16]     CFI bit
    // Bit  [15]     end of frame
    // Bit  [14]     start of frame
    // Bit  [13]     with jumbo frame bit 13 is concatenated with the size field
    //               in order to receive jumbo frames. If ignore FCS enabled
    //               this bit indicated FCS status - 1 for bad FCS
    // Bits [12..0]  length of frame
    u32 info;
};

/// Transmitt buffer descriptor located in system memory
struct gmac_tx_desc {
    // Bits [31..0]  address of buffer
    u32 addr;

    // Bit  [31]     used bit - must be zero for the GMAC to read data from the
    //               buffer. After a succesful transfer this bit is set to one
    //               by the GMAC
    // Bit  [30]     wrap bit - marks the last descriptor in the buffer list
    // Bit  [29]     retry limit exceeded
    // Bit  [27]     transmitt frame corruption due to AHB error
    // Bit  [26]     late collision
    // Bits [22..20] 0b000 - no error
    //               0b001 - VLAN packet with uncomplete or wrong header
    //               0b010 - SNAP packet with uncomplete or wrong header
    //               0b011 - no IP packet or short IP packet or not IPv1 or IPv6
    //               0b100 - packet is not IP, SNAP or VLAN
    //               0b101 - non supported packet fragmentation
    //               0b110 - packet type is not UDP or TCP
    //               0b111 premature end of packet
    // Bit  [16]     no CRC is to be appended
    // Bit  [15]     last buffer in current frame
    // Bits [13..0]  length of buffer
    u32 info;
};

/// Allocate the TX and RX descriptor structure in memory
ALIGN(8) static struct gmac_tx_desc tx_descriptors[GMAC_TX_DESCRIPTORS];
ALIGN(8) static struct gmac_rx_desc rx_descriptors[GMAC_RX_DESCRIPTORS];

ALIGN(8) static struct gmac_tx_desc tx_descriptor_dummy;
ALIGN(8) static struct gmac_rx_desc rx_descriptor_dummy;

/// Allocate the TX and RX buffers. NOTE that the RX buffer must be in a 
/// multiple of 64 bytes. 
ALIGN(32) static u8 tx_buffers[GMAC_TX_DESCRIPTORS][GMAC_TX_BUFFER_SIZE];
ALIGN(32) static u8 rx_buffers[GMAC_RX_DESCRIPTORS][GMAC_RX_BUFFER_SIZE];

ALIGN(32) static u8 tx_buffer_dummy[4];
ALIGN(32) static u8 rx_buffer_dummy[4];

static u32 tx_buffer_index;
static u32 rx_buffer_index;

/// Initializes and links the tx buffer descriptors to the tx buffers
static void gmac_init_tx_buffer_pool(void) {

    // Set the tx index to zero by default
    tx_buffer_index = 0;
    
    for (u32 i = 0; i < GMAC_TX_DESCRIPTORS; i++) {
        tx_descriptors[i].addr = (u32)tx_buffers[i];
        tx_descriptors[i].info = 0;
    }

    // The last descriptor must have the wrap bit set
    tx_descriptors[GMAC_TX_DESCRIPTORS - 1].info |= (1 << 30);

    // Write the TX buffer queue base address
    GMAC->TBQB = (u32)&tx_descriptors[0];
}

/// Initializes and links the rx buffer descriptors to the rx buffers
static void gmac_init_rx_buffer_pool(void) {
    // Set the tx index to zero by default
    rx_buffer_index = 0;
    
    for (u32 i = 0; i < GMAC_RX_DESCRIPTORS; i++) {
        rx_descriptors[i].addr = (u32)rx_buffers[i];
        rx_descriptors[i].info = 0;
    }

    // The last descriptor must have the wrap bit set
    rx_descriptors[GMAC_RX_DESCRIPTORS - 1].addr |= (1 << 1);

    // Write the TX buffer queue base address
    GMAC->RBQB = (u32)&rx_descriptors[0];
}

/// Initializes the dummy buffer pool
static void gmac_init_dummy_buffer_pool(void) {
    rx_descriptor_dummy.addr = (u32)rx_buffer_dummy;
    rx_descriptor_dummy.info = 0;
    tx_descriptor_dummy.addr = (u32)tx_buffer_dummy;
    tx_descriptor_dummy.info = 0;
    
    tx_descriptor_dummy.info |= (1 << 30);
    tx_descriptor_dummy.addr |= (1 << 1);

    for (u8 i = 0; i < 5; i++) {
        GMAC->TBQBAPQ[i] = (u32)&tx_descriptor_dummy;
        GMAC->RBQBAPQ[i] = (u32)&rx_descriptor_dummy;
    }
}

void gmac_init(void) {
    // Severy configuration options must be done while the TX and RX circuits
    // are disabled
    // NCR
    // NCFGR
    // UR
    // DCFGR
    // WOL
    // IPGS
    gmac_enable_loop_back();

    // Clear all statistics registers
    GMAC->NCR |= (1 << 5);

    // Basic configuration, enable 100 MHz operation, full-duplex mode, at MCK
    // divided by 64
    GMAC->NCFGR = (1 << 0) | (1 << 1) | (4 << 18);

    // The ethernet PHY only supports RMII interface
    GMAC->UR = 0;

    // Configure the DMA register and set the DMA receive buffer size to 128 B
    GMAC->DCFGR = (4 << 0) | (3 << 8) | (1 << 10) | (2 << 16);

    // Clear the wake-up on LAN
    GMAC->WOL = 0;
    GMAC->IPGS = (1 << 8) | 1;

    gmac_init_rx_buffer_pool();
    gmac_init_tx_buffer_pool();
    gmac_init_dummy_buffer_pool();

    GMAC->IER = (1 << 7) | (1 << 1);
}

void gmac_deinit(void) {
    GMAC->IDR = 0xFFFFFFFF;
    GMAC->NCR = 0x00000000;
}

void gmac_enable(void) {
    GMAC->NCR |= (1 << 2) | (1 << 3);
}

void gmac_disable(void) {
    GMAC->NCR &= ~(1 << 2) | (1 << 3);
}

void gmac_enable_loop_back(void) {
    // These write operations must NOT be combined
    // Disable RX and TX
    u8 gmac_state = (GMAC->NCR >> 2) & 0b11;
    GMAC->NCR &= ~((1 << 2) | (1 << 3));

    // Enable local loop back
    GMAC->NCR |= (1 << 1);

    // Reenable TX and RX if needed
    GMAC->NCR |= ((gmac_state & 0b11) << 2);
}

void gmac_disable_loop_back(void) {
    // These write operations must NOT be combined
    // Disable RX and TX
    u8 gmac_state = (GMAC->NCR >> 2) & 0b11;
    GMAC->NCR &= ~((1 << 2) | (1 << 3));

    // Enable local loop back
    GMAC->NCR &= ~(1 << 1);

    // Reenable TX and RX if needed
    GMAC->NCR |= ((gmac_state & 0b11) << 2);
}

u16 gmac_in_phy(u8 phy_addr, u8 reg) {
    // Enable the PHY maintainance bit
    GMAC->NCR |= (1 << 4);

    GMAC->MAN = (0b10 << 16) | ((phy_addr & 0b11111) << 23) | 
                ((reg & 0b11111) << 18) | (0b10 << 28) | (1 << 30);
    
    while (!(GMAC->NSR & (1 << 2)));

    // Read the response
    u16 data = (u16)(GMAC->MAN & 0xFFFF);

    // Disable the PHY maintainance bit
    GMAC->NCR &= ~(1 << 4);

    return data;
}

void gmac_out_phy(u8 phy_addr, u8 reg, u16 data) {
    // Enable the PHY maintainance bit
    GMAC->NCR |= (1 << 4);

    GMAC->MAN = (0b10 << 16) | ((phy_addr & 0b11111) << 23) | 
                ((reg & 0b11111) << 18) | (0b01 << 28) | (1 << 30) | data;
    
    while (!(GMAC->NSR & (1 << 2)));

    // Disable the PHY maintainance bit
    GMAC->NCR &= ~(1 << 4);
}

u32 gmac_get_raw_length(void) {
    // This will hold the raw received data length
    u32 length = 0;
    u8 sof_valid = 0;

    for (u32 i = 0; i < GMAC_RX_DESCRIPTORS; i++) {
        // Index will point to the current buffer descriptor to read
        u32 index = rx_buffer_index + i;
        if (index >= GMAC_RX_DESCRIPTORS) {
            index -= GMAC_RX_DESCRIPTORS;
        }

        // Bit 0 of the address marks the ownership. If this is set to zero the
        // buffer is not owned by the GMAC
        if ((rx_descriptors[index].addr & 0b1) == 0) {
            break;
        }

        // The computed length will start from the SOF buffer and and on the
        // EOF buffer. All buffer between these should be counted
        if (rx_descriptors[index].info & (1 << 14)) {
            sof_valid = 1;
        }

        // Check for the EOF tag
        if (rx_descriptors[index].info & (1 << 15)) {
            break;
        }

        // Compute the size which is the 13 lower bits in the info field
        if (sof_valid) {
            length += rx_descriptors[index].info & 0b1111111111111;
        }
    }

    // This will ONLY compute the size and should therefore NOT modify the 
    // `rx_buffer_index`

    return length;
    
}

u32 gmac_read_raw(u8* buffer, u32 size) {

}

u32 gmac_write_raw(const u8* buffer, u32 size) {

}

void gmac_handler(void) {
    (void)GMAC->ISR;
    (void)GMAC->TSR;
    (void)GMAC->RSR;

    printl("GMAC");
}
