#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstring>

extern "C" {
#include <ecrt.h>
}

// ---------- CONFIG ---------- 
// generated from: sudo ethercat xml > slaves.xml. or read from: sudo ethercat slaves -v
#define VENDOR_ID  1516597871
#define PRODUCT_ID 0x00029252

// ---------- GLOBAL ----------
static ec_master_t *master = nullptr;
static ec_domain_t *domain = nullptr;
static ec_slave_config_t *sc = nullptr;

static uint8_t *domain_pd = nullptr;

// PDO offsets
static unsigned int off_control_word;
static unsigned int off_target_position;
static unsigned int off_status_word;
static unsigned int off_actual_position;

// ---------- PDO CONFIG ----------

// RxPDO (master → slave)
// from pdos.txt, which is generated from: sudo ethercat pdos > pdos.txt
static ec_pdo_entry_info_t slave_pdo_entries[] = {
    {0x607A, 0x00, 32}, // target position
    {0x60FE, 0x00, 32}, // Digital outputs
    {0x6040, 0x00, 16}, // control word
};

// TxPDO (slave → master)
static ec_pdo_entry_info_t slave_pdo_entries_tx[] = {
    {0x6064, 0x00, 32}, // actual position
    {0x60FD, 0x00, 32}, // Digital inputs
    {0x6041, 0x00, 16}, // status word
};

// PDOs ( RxPDO and TxPDO)
static ec_pdo_info_t slave_pdos[] = {
    {0x1600, 3, slave_pdo_entries},
    {0x1A00, 3, slave_pdo_entries_tx},
};

// Sync managers
// from pdos.txt: // SM2 → RxPDO (outputs), SM3 → TxPDO (inputs)
static ec_sync_info_t slave_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT,  0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, &slave_pdos[0], EC_WD_ENABLE},
    {3, EC_DIR_INPUT,  1, &slave_pdos[1], EC_WD_DISABLE},
    {0xff}
};

// Domain registration
// from pdos.txt:
static ec_pdo_entry_reg_t domain_regs[] = {
    {0, 4, VENDOR_ID, PRODUCT_ID, 0x6040, 0, &off_control_word},
    {0, 4, VENDOR_ID, PRODUCT_ID, 0x607A, 0, &off_target_position},
    {0, 4, VENDOR_ID, PRODUCT_ID, 0x6041, 0, &off_status_word},
    {0, 4, VENDOR_ID, PRODUCT_ID, 0x6064, 0, &off_actual_position},
    {}
};

// ---------- SIGNAL ----------
// later for while loop, interrupt when: ctrl+c (in main: SIGINT)
static bool running = true;
void signal_handler(int)
{
    running = false;
}

// ---------- MAIN ----------
int main()
{
    signal(SIGINT, signal_handler);

    // 1. Request master
    master = ecrt_request_master(0); 
    if (!master) {
        std::cerr << "Failed to get master\n";
        return -1;
    }

    // 2. Create domain
    domain = ecrt_master_create_domain(master);
    if (!domain) {
        std::cerr << "Failed to create domain\n";
        return -1;
    }

    // 3. Configure slave (slave: 4)
    sc = ecrt_master_slave_config(master, 0, 4, VENDOR_ID, PRODUCT_ID);
    if (!sc) {
        std::cerr << "Failed to config slave\n";
        return -1;
    }

    // 4. Configure PDOs
    if (ecrt_slave_config_pdos(sc, EC_END, slave_syncs)) {
        std::cerr << "Failed PDO config\n";
        return -1;
    }

    // 5. Register PDOs
    if (ecrt_domain_reg_pdo_entry_list(domain, domain_regs)) {
        std::cerr << "PDO registration failed\n";
        return -1;
    }

    // 6. Activate master
    if (ecrt_master_activate(master)) {
        std::cerr << "Master activate failed\n";
        return -1;
    }

    // 7. Get process data pointer
    domain_pd = ecrt_domain_data(domain);

    std::cout << "Master activated. Starting loop...\n";

    static bool target_set = false;
    static int32_t target = 0;

    // ---------- CYCLIC LOOP ----------
    while (running) {

        // Receive
        ecrt_master_receive(master);
        ecrt_domain_process(domain);

        // Read
        uint16_t status = EC_READ_U16(domain_pd + off_status_word);
        int32_t position = EC_READ_S32(domain_pd + off_actual_position);

        static uint16_t control = 0;

        std::cout << "Status: 0x" << std::hex << status
                  << "  control: 0x" << control << std::dec
                  << " Pos: " << std::dec << position << std::endl;


        // Write (safe: zero control word)
        // EC_WRITE_U16(domain_pd + off_control_word, 0x0000);

        // basic CiA402 enable sequence
        if (status & 0x0008)                    control = 0x0080; // fault reset
        else if ((status & 0x004F) == 0x0040)   control = 0x0006; // Shutdown, (0x0040: Switch ON Disabled)
        else if ((status & 0x006F) == 0x0021)   control = 0x0007; // Switch on, (0x0021: ready to switch on)
        else if ((status & 0x006F) == 0x0023)   control = 0x000F; // Enable operation (0x0023: switched on)
        EC_WRITE_U16(domain_pd + off_control_word, control);

        

        if (!target_set && status == 0x1237) {
            target = position + 1000;   // very small step [motor specific, exp: for Elmo Gold, 1000 = 1 degree] 
            target_set = true;
        }

        // to keep position unchanged, use the following:
        // EC_WRITE_S32(domain_pd + off_target_position, position);

        EC_WRITE_S32(domain_pd + off_target_position, target_set ? target : position);


        // Send
        ecrt_domain_queue(domain);
        ecrt_master_send(master);

        usleep(1000); // 1 ms
    }

    // Cleanup: disable motor before exit
    EC_WRITE_U16(domain_pd + off_control_word, 0x0000);
    ecrt_domain_queue(domain);
    ecrt_master_send(master);

    std::cout << "Stopping...\n";
    return 0;
}