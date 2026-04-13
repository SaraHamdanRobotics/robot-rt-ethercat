# EtherCAT Introduction and Setup

## Communication Layer

The communication layer with the motors uses EtherCAT. This is determined from:
- Motor drive datasheet/brand/ESI file existence
- Ports on the drive (2 Ethernet ports: in/out, often EtherCAT)

### Key Concepts
- **Ethernet**: Physical cable (RJ45)
- **EtherCAT**: Communication protocol over Ethernet
- **CAN Bus**: Slower bus; use CAN-USB eRob Debugger for manual configuration/debugging, not RT control
- **CANopen**: Protocol on CAN
- **CoE**: CANopen over EtherCAT (CANopen messages tunneled inside EtherCAT)
- **ZeroErr**: Motor brand (HW)
- **eRob**: Company name and provided SW/Tools

## Checking Ethernet Interface

1. Check if PC has Ethernet interface: `$ ip a`
2. After connecting cable, check detection: `sudo ethtool enx207bd2598ad7` (Note: driver `cdc_ncm` → USB Ethernet device)
3. Check traffic: `sudo tcpdump -i enx207bd2598ad7`
   - Without cable: link-type EN10MB (Ethernet)
   - With cable: Check for 0x88a4 or EtherCAT frames
   - No traffic: Need EtherCAT master SW

## Installing IgH EtherCAT Master

IgH EtherCAT master is SW that makes your PC talk EtherCAT, acting as the EtherCAT master. PC becomes the real-time controller; robot motors' drivers (and end IO) are slaves.

### Installation Steps
- Clone repository: `git clone https://github.com/sittner/ec-master.git ~/repos/ethercat` (or similar, from official website)

### Useful Commands
- `sudo systemctl status ethercat.service` (start/stop/restart/...)
- `sudo depmod` (generate kernel module dependency list)
- `sudo systemctl daemon-reload` (reload configurations)

## Configuring the NIC for EtherCAT

1. Identify Ethernet name: `ip a` → e.g., `enx207bd2598ad7` or MAC address `"20:7b:d2:59:8a:d7"`
2. Edit config: `/etc/ethercat.conf` or `/usr/local/etc/ethercat.conf`
   - Set: `MASTER0_DEVICE="enx207bd2598ad7"` (or MAC)
   - Set: `DEVICE_MODULES="generic"`

### Verification
- `sudo dmesg | grep -i ethercat` (should show master and slaves)
- `ls -l /dev/EtherCAT*` (e.g., `/dev/EtherCAT0`)
- `sudo ethercat slaves` (should show slaves, e.g., 7 slaves: 6 joints + end IO)

Note: Slaves are in PREOP state initially. OP is requested after application configures slaves/domains and activates master.

## EtherCAT Commands

- `sudo ethercat master`: Master/bus summary
- `sudo ethercat pdos`: Process data layout for cyclic RT exchange
- `sudo ethercat cstruct`: Generated C-style slave config skeleton
- `sudo ethercat xml > slaves.xml`: Readable snapshot of discovered slave info
- `sudo ethercat slave -p 0`: Info for one slave
- `sudo ethercat sdos -p 0`: SDOs for slave

### Reading Data
- `sudo ethercat upload -p 0 0x6041 0`: Status word of motor 0 (e.g., output: 0x1250 4688)

## Changing Slave States

To change slaves from PREOP → OP:

1. Run: `sudo ethercat state OP`
   - States: 'INIT', 'PREOP', 'BOOT', 'SAFEOP', 'OP'
2. Check: `sudo ethercat slaves`

If still PREOP with error:
- Check `sudo dmesg | grep -i ethercat`
- Common error: "Failed to set SAFEOP state, slave refused state change (PREOP + ERROR). AL status message 0x001E: 'Invalid input configuration'."
- Reason: Missing configuration (PDO mapping/ENI). EtherCAT won't go to OP without valid config.

IgH EtherCAT master does NOT use ENI directly via CLI. You need a user-space application with code like:
- `ecrt_master_slave_config(...)`
- `ecrt_slave_config_pdos(...)`
- `ecrt_domain_reg_pdo_entry_list(...)`
- `ecrt_master_activate(...)`

## PDO Configuration

To write configuration:

1. Get slave info: `sudo ethercat xml > slaves.xml` or `sudo ethercat slaves -v`
2. Get PDOs: `sudo ethercat pdos > pdos.txt`
   - RxPDO (commands): 0x1600...
   - TxPDO (feedback): 0x1A00...

Note: From XML, PDOs are Fixed + Mandatory → No need for complex remapping.

