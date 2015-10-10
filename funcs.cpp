/*
 * License:
 *
 * Copyright (c) 2012-2015 James Bensley.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * 
 * File: Etherate Gerneral Functions
 *
 * File Contents:
 * build_headers()
 * build_tlv()
 * build_sub_tlv()
 * cli_args()
 * explode_char()
 * get_interface_mtu_by_name()
 * get_sock_interface()
 * htonll()
 * list_interfaces()
 * ntohll()
 * print_usage()
 * set_sock_interface_index()
 * set_sock_interface_name()
 * signal_handler()
 * sync_settings()
 *
 */



// Build the Ethernet headers for sending frames
void build_headers(struct FRAME_HEADERS *FRAME_HEADERS);

// Build the Etherate TLV header
void build_tlv(struct FRAME_HEADERS *FRAME_HEADERS, unsigned short TLV_TYPE,
               unsigned long TLV_VALUE);

// Build a sub-TLV value after the TLV header
void build_sub_tlv(struct FRAME_HEADERS *FRAME_HEADERS, unsigned short SUB_TLV_TYPE,
                   unsigned long long SUB_TLV_VALUE);

// Process the CLI arguments
int cli_args(int argc, char *argv[], struct APP_PARAMS *APP_PARAMS,
             struct FRAME_HEADERS *FRAME_HEADERS, struct TEST_INTERFACE *TEST_INTERFACE,
             struct TEST_PARAMS *TEST_PARAMS, struct MTU_TEST *MTU_TEST,
             struct QM_TEST *QM_TEST);

// Explode a char string based on the passed field delimiter
int explode_char(char *string, char *delim, char *tokens[]);

// Get the MTU of the interface used for the test
int get_interface_mtu_by_name(struct TEST_INTERFACE *TEST_INTERFACE);

// Try to automatically chose an interface to run the test on
int get_sock_interface(struct TEST_INTERFACE *TEST_INTERFACE);

unsigned long long htonll(unsigned long long val);

// List interfaces and hardware (MAC) address
void list_interfaces();

unsigned long long ntohll(unsigned long long val);

// Print CLI args and usage
void print_usage(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS);

// Try to open the passed socket on a user specified interface by index
int set_sock_interface_index(struct TEST_INTERFACE *TEST_INTERFACE);

// Try to open the passed socket on a user specified interface by name
int set_sock_interface_name(struct TEST_INTERFACE *TEST_INTERFACE);

// Signal handler to notify remote host of local application termiantion
void signal_handler(int signal);

// Send the settings from TX to RX
void sync_settings(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS * TEST_PARAMS,
                  struct MTU_TEST *MTU_TEST, struct QM_TEST *QM_TEST);



void build_headers(struct FRAME_HEADERS *FRAME_HEADERS)
{

    int BUFFER_OFFSET = 0;
    short TPI = 0;
    short TCI = 0;
    short *pTPI = &TPI;
    short *pTCI = &TCI;
    short VLAN_ID_TMP;

    // Copy the destination and source MAC addresses
    memcpy((void*)FRAME_HEADERS->TX_BUFFER, (void*)FRAME_HEADERS->DESTINATION_MAC, ETH_ALEN);
    BUFFER_OFFSET+=ETH_ALEN;
    memcpy((void*)(FRAME_HEADERS->TX_BUFFER+BUFFER_OFFSET), (void*)FRAME_HEADERS->SOURCE_MAC, ETH_ALEN);
    BUFFER_OFFSET+=ETH_ALEN;

    // Check to see if QinQ VLAN ID has been supplied
    if (FRAME_HEADERS->QINQ_PCP != QINQ_PCP_DEF || FRAME_HEADERS->QINQ_ID != QINQ_ID_DEF)
    {

        // Add on the QinQ Tag Protocol Identifier
        // 0x88a8 == IEEE802.1ad, 0x9100 == older IEEE802.1QinQ
        TPI = htons(0x88a8);
        memcpy((void*)(FRAME_HEADERS->TX_BUFFER+BUFFER_OFFSET), pTPI, sizeof(TPI));
        BUFFER_OFFSET+=sizeof(TPI);

        // Now build the QinQ Tag Control Identifier:
        VLAN_ID_TMP = FRAME_HEADERS->QINQ_ID;
        TCI = (FRAME_HEADERS->QINQ_PCP & 0x07) << 5;
        FRAME_HEADERS->QINQ_ID = FRAME_HEADERS->QINQ_ID >> 8;
        TCI = TCI | (FRAME_HEADERS->QINQ_ID & 0x0f);
        FRAME_HEADERS->QINQ_ID = VLAN_ID_TMP;
        FRAME_HEADERS->QINQ_ID = FRAME_HEADERS->QINQ_ID << 8;
        TCI = TCI | (FRAME_HEADERS->QINQ_ID & 0xffff);

        memcpy((void*)(FRAME_HEADERS->TX_BUFFER+BUFFER_OFFSET), pTCI, sizeof(TCI));
        BUFFER_OFFSET+=sizeof(TCI);

        // If an outer VLAN ID has been set, but not an inner one
        // (which would be a mistake) set it to 1 so the frame is still valid
        if (FRAME_HEADERS->VLAN_ID == 0) FRAME_HEADERS->VLAN_ID = 1;

    }

    // Check to see if an inner VLAN PCP value or VLAN ID has been supplied
    if (FRAME_HEADERS->PCP != PCP_DEF || FRAME_HEADERS->VLAN_ID != VLAN_ID_DEF)
    {

        TPI = htons(0x8100);
        memcpy((void*)(FRAME_HEADERS->TX_BUFFER+BUFFER_OFFSET), pTPI, sizeof(TPI));
        BUFFER_OFFSET+=sizeof(TPI);

        VLAN_ID_TMP = FRAME_HEADERS->VLAN_ID;
        TCI = (FRAME_HEADERS->PCP & 0x07) << 5;
        FRAME_HEADERS->VLAN_ID = FRAME_HEADERS->VLAN_ID >> 8;
        TCI = TCI | (FRAME_HEADERS->VLAN_ID & 0x0f);
        FRAME_HEADERS->VLAN_ID = VLAN_ID_TMP;
        FRAME_HEADERS->VLAN_ID = FRAME_HEADERS->VLAN_ID << 8;
        TCI = TCI | (FRAME_HEADERS->VLAN_ID & 0xffff);

        memcpy((void*)(FRAME_HEADERS->TX_BUFFER+BUFFER_OFFSET), pTCI, sizeof(TCI));
        BUFFER_OFFSET+=sizeof(TCI);

    }

    // Push on the ethertype for the Etherate payload
    TPI = htons(FRAME_HEADERS->ETHERTYPE);
    memcpy((void*)(FRAME_HEADERS->TX_BUFFER+BUFFER_OFFSET), pTPI, sizeof(TPI));
    BUFFER_OFFSET+=sizeof(TPI);

    FRAME_HEADERS->LENGTH = BUFFER_OFFSET;

    // Pointers to the payload section of the frame
    FRAME_HEADERS->RX_DATA = FRAME_HEADERS->RX_BUFFER + FRAME_HEADERS->LENGTH;
    FRAME_HEADERS->TX_DATA = FRAME_HEADERS->TX_BUFFER + FRAME_HEADERS->LENGTH;

    // When receiving a single tagged frame the Kernel strips off the VLAN tag,
    // so RX_DATA buffer starts 4 bytes earlier in the receive buffer than in
    // it does in the transit buffer
    if (FRAME_HEADERS->LENGTH == 18)
    {
        FRAME_HEADERS->RX_DATA -= 4;
    // When receiving a double tagged frame, the Kernel as above strips off the
    // outter VLAN tag, but not any inner tags, so again the RX_DATA buffer
    // starts 4 bytes earlier in the receive buffer than does in the transmit buffer
    } else if (FRAME_HEADERS->LENGTH == 22) {
        // http://stackoverflow.com/questions/24355597/
        // linux-when-sending-ethernet-frames-the-ethertype-is-being-re-written
        FRAME_HEADERS->RX_DATA -=4;
    }

}



void build_tlv(struct FRAME_HEADERS *FRAME_HEADERS, unsigned short TLV_TYPE, unsigned long TLV_VALUE)
{

    char TLV_LENGTH = sizeof(TLV_VALUE);

    char *BUFFER_OFFSET = FRAME_HEADERS->TX_DATA;
    memcpy(BUFFER_OFFSET, &TLV_TYPE, sizeof(TLV_TYPE));

    BUFFER_OFFSET += sizeof(TLV_TYPE);
    memcpy(BUFFER_OFFSET, &TLV_LENGTH, sizeof(TLV_LENGTH));
    
    BUFFER_OFFSET += sizeof(TLV_LENGTH);
    memcpy(BUFFER_OFFSET, &TLV_VALUE, (int)TLV_LENGTH);

    FRAME_HEADERS->RX_TLV_TYPE  = (unsigned short*)FRAME_HEADERS->RX_DATA;

    FRAME_HEADERS->RX_TLV_VALUE = (unsigned long*)(FRAME_HEADERS->RX_DATA +
                                  sizeof(TLV_TYPE) + sizeof(TLV_LENGTH));

    return;

}



void build_sub_tlv(struct FRAME_HEADERS *FRAME_HEADERS, unsigned short SUB_TLV_TYPE, unsigned long long SUB_TLV_VALUE)
{

    char SUB_TLV_LENGTH = sizeof(SUB_TLV_VALUE);

    char *BUFFER_OFFSET = FRAME_HEADERS->TX_DATA + FRAME_HEADERS->TLV_SIZE;
    memcpy(BUFFER_OFFSET, &SUB_TLV_TYPE, sizeof(SUB_TLV_TYPE));
    
    BUFFER_OFFSET += sizeof(SUB_TLV_TYPE);
    memcpy(BUFFER_OFFSET, &SUB_TLV_LENGTH, sizeof(SUB_TLV_LENGTH));
    
    BUFFER_OFFSET += sizeof(SUB_TLV_LENGTH);
    memcpy(BUFFER_OFFSET, &SUB_TLV_VALUE, (int)SUB_TLV_LENGTH);

    FRAME_HEADERS->RX_SUB_TLV_TYPE  = (unsigned short*) (FRAME_HEADERS->RX_DATA +
                                                FRAME_HEADERS->TLV_SIZE);

    FRAME_HEADERS->RX_SUB_TLV_VALUE = (unsigned long long*) (FRAME_HEADERS->RX_DATA +
                                                             FRAME_HEADERS->TLV_SIZE +
                                                             sizeof(SUB_TLV_TYPE) +
                                                             sizeof(SUB_TLV_LENGTH));

    return;

}



int cli_args(int argc, char *argv[], struct APP_PARAMS *APP_PARAMS,
             struct FRAME_HEADERS *FRAME_HEADERS,struct TEST_INTERFACE *TEST_INTERFACE,
             struct TEST_PARAMS *TEST_PARAMS, struct MTU_TEST *MTU_TEST,
             struct QM_TEST *QM_TEST)
{

    int RET_EXIT_APP =    -2;
    int RET_EX_USAGE =    -1;
    int RET_EXIT_SUCCESS = 0;

    if (argc > 1) 
    {

        for (int i = 1; i < argc; i++) 
        {

            // Change to receive mode
            if (strncmp(argv[i], "-r" ,2)==0) 
            {
                APP_PARAMS->TX_MODE = false;

                FRAME_HEADERS->SOURCE_MAC[0] = 0x00;
                FRAME_HEADERS->SOURCE_MAC[1] = 0x00;
                FRAME_HEADERS->SOURCE_MAC[2] = 0x5E;
                FRAME_HEADERS->SOURCE_MAC[3] = 0x00;
                FRAME_HEADERS->SOURCE_MAC[4] = 0x00;
                FRAME_HEADERS->SOURCE_MAC[5] = 0x02;

                FRAME_HEADERS->DESTINATION_MAC[0] = 0x00;
                FRAME_HEADERS->DESTINATION_MAC[1] = 0x00;
                FRAME_HEADERS->DESTINATION_MAC[2] = 0x5E;
                FRAME_HEADERS->DESTINATION_MAC[3] = 0x00;
                FRAME_HEADERS->DESTINATION_MAC[4] = 0x00;
                FRAME_HEADERS->DESTINATION_MAC[5] = 0x01;


            // Specifying a custom destination MAC address
            } else if (strncmp(argv[i], "-d", 2)==0) {
                int count;
                char *tokens[6];
                char delim[] = ":";

                count = explode_char(argv[i+1], delim, tokens);
                if (count == 6) {
                    for (int j = 0; j < 6; j++)
                    {
                        FRAME_HEADERS->DESTINATION_MAC[j] = (unsigned char)strtoul(tokens[j],NULL, 16);
                    }

                } else {
                    printf("Error: Invalid destination MAC address!\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }
                i++;


            // Disable settings sync between TX and RX
            } else if (strncmp(argv[i], "-g", 2)==0) {
                APP_PARAMS->TX_SYNC = false;


            // Disable the TX to RX delay check
            } else if (strncmp(argv[i], "-G", 2)==0) {
                APP_PARAMS->TX_DELAY = false;


            // Specifying a custom source MAC address
            } else if (strncmp(argv[i], "-s" ,2)==0) {
                int count;
                char *tokens[6];
                char delim[] = ":";

                count = explode_char(argv[i+1], delim, tokens);
                if (count == 6) {
                    for (int j = 0; j < 6; j++)
                    {
                        FRAME_HEADERS->SOURCE_MAC[j] = (unsigned char)strtoul(tokens[j],NULL, 16);
                    }

                } else {
                    printf("Error: Invalid source MAC address!\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }
                i++;


            // Specifying a custom interface name
            } else if (strncmp(argv[i], "-i", 2)==0) {
                if (argc > (i+1))
                {
                    strncpy(TEST_INTERFACE->IF_NAME,argv[i+1],sizeof(argv[i+1]));
                    i++;
                } else {
                    printf("Oops! Missing interface name\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Specifying a custom interface index
            } else if (strncmp(argv[i], "-I", 2)==0) {
                if (argc > (i+1))
                {
                    TEST_INTERFACE->IF_INDEX = (int)strtoul(argv[i+1], NULL, 0);
                    i++;
                } else {
                    printf("Oops! Missing interface index\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Requesting to list interfaces
            } else if (strncmp(argv[i], "-l", 2)==0) {
                list_interfaces();
                return RET_EXIT_APP;


            // Specifying a custom frame payload size in bytes
            } else if (strncmp(argv[i], "-f", 2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_SIZE = (unsigned int)strtoul(argv[i+1], NULL, 0);
                    if (TEST_PARAMS->F_SIZE > 1500)
                    {
                        printf("WARNING: Make sure your device supports baby "
                               "giants or jumbo frames as required\n");
                    }
                    i++;

                } else {
                    printf("Oops! Missing frame size\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Specifying a custom transmission duration in seconds
            } else if (strncmp(argv[i], "-t", 2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_DURATION = strtoull(argv[i+1], NULL, 0);
                    i++;
                } else {
                    printf("Oops! Missing transmission duration\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Specifying the total number of frames to send instead of duration
            } else if (strncmp(argv[i], "-c", 2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_COUNT = strtoull(argv[i+1], NULL, 0);
                    i++;
                } else {
                    printf("Oops! Missing max frame count\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Specifying the total number of bytes to send instead of duration
            } else if (strncmp(argv[i], "-b", 2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_BYTES = strtoull(argv[i+1], NULL, 0);
                    i++;
                } else {
                    printf("Oops! Missing max byte transfer limit\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Enable ACK mode testing
            } else if (strncmp(argv[i], "-a", 2)==0) {
                TEST_PARAMS->F_ACK = true;


            // Limit TX rate to max bytes per second
            } else if (strncmp(argv[i], "-m", 2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->B_TX_SPEED_MAX = strtoul(argv[i+1], NULL, 0);
                    i++;
                } else {
                    printf("Oops! Missing max TX rate\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Limit TX rate to max bits per second
            } else if (strncmp(argv[i], "-M", 2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->B_TX_SPEED_MAX = (unsigned long)floor(strtoul(argv[i+1], NULL, 0) / 8);
                    i++;
                } else {
                    printf("Oops! Missing max TX rate\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Set 802.1p PCP value
            } else if (strncmp(argv[i], "-p", 2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->PCP = (unsigned short)atoi(argv[i+1]);
                    i++;
                } else {
                    printf("Oops! Missing 802.1p PCP value\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Set 802.1q VLAN ID
            } else if (strncmp(argv[i], "-v", 2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->VLAN_ID = (unsigned short)atoi(argv[i+1]);
                    i++;
                } else {
                    printf("Oops! Missing 802.1p VLAN ID\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Set 802.1ad QinQ outer VLAN ID
            } else if (strncmp(argv[i], "-q", 2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->QINQ_ID = (unsigned short)atoi(argv[i+1]);
                    i++;
                } else {
                    printf("Oops! Missing 802.1ad QinQ outer VLAN ID\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Set 802.1ad QinQ outer PCP value
            } else if (strncmp(argv[i], "-o", 2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->QINQ_PCP = (unsigned short)atoi(argv[i+1]);
                    i++;
                } else {
                    printf("Oops! Missing 802.1ad QinQ outer PCP value\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Set a custom ethertype
            } else if (strncmp(argv[i], "-e", 2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->ETHERTYPE = (unsigned short)strtol(argv[i+1], NULL, 16);
                    i++;
                } else {
                    printf("Oops! Missing ethertype value\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Enable the MTU sweep test
            } else if (strncmp(argv[i], "-U", 2)==0) {
                if (argc > (i+1))
                {
                    MTU_TEST->MTU_TX_MIN = (unsigned short)atoi(argv[i+1]);
                    MTU_TEST->MTU_TX_MAX = (unsigned short)atoi(argv[i+2]);
                    if (MTU_TEST->MTU_TX_MAX > F_SIZE_MAX) { 
                        printf("MTU size can not exceed the maximum hard "
                               "coded frame size: %u\n", F_SIZE_MAX);
                             return RET_EX_USAGE;
                    }
                    MTU_TEST->ENABLED = true;
                    i+=2;
                } else {
                    printf("Oops! Missing min/max MTU sizes\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Enable the link quality measurement tests
            } else if (strncmp(argv[i], "-Q", 2)==0) {
                if (argc > (i+1))
                {
                    QM_TEST->INTERVAL = (unsigned int)atoi(argv[i+1]);
                    QM_TEST->TIMEOUT =  (unsigned int)atoi(argv[i+2]);
                    if (QM_TEST->TIMEOUT > QM_TEST->INTERVAL) { 
                        printf("Oops! Echo timeout exceeded the interval\n");
                        return RET_EX_USAGE;
                    }
                    // Convert to ns for use with timespec
                    QM_TEST->INTERVAL_NSEC = (QM_TEST->INTERVAL*1000000)%1000000000;
                    QM_TEST->INTERVAL_SEC =  ((QM_TEST->INTERVAL*1000000)-QM_TEST->INTERVAL_NSEC)/1000000000;
                    QM_TEST->TIMEOUT_NSEC =  (QM_TEST->TIMEOUT*1000000)%1000000000;
                    QM_TEST->TIMEOUT_SEC =   ((QM_TEST->TIMEOUT*1000000)-QM_TEST->TIMEOUT_NSEC)/1000000000;
                    QM_TEST->ENABLED =       true;
                    i+=2;
                } else {
                    printf("Oops! Missing interval and timeout values\n"
                           "Usage info: %s -h\n", argv[0]);
                    return RET_EX_USAGE;
                }


            // Display version
            } else if (strncmp(argv[i], "-V", 2)==0 ||
                       strncmp(argv[i], "--version", 9)==0) {

                printf("Etherate version %s\n", VERSION);
                return RET_EXIT_APP;


            // Display usage instructions
            } else if (strncmp(argv[i], "-h", 2)==0 ||
                       strncmp(argv[i], "--help", 6)==0) {

                print_usage(APP_PARAMS, FRAME_HEADERS);
                return RET_EXIT_APP;


            // Else the user entered an invalid argument
            } else {
                    printf("Oops! Invalid argument %s\n"
                           "Usage info: %s -h\n", argv[i], argv[0]);
                    return RET_EX_USAGE;
            }


        }

        return RET_EXIT_SUCCESS;

    } else {

        return RET_EXIT_SUCCESS;

    }

}



int explode_char(char *string, char *delim, char *tokens[])
{

    int count = 0;
    char *token;
    char *stringp;
 
    stringp = string;
    while (stringp != NULL) {
        token = strsep(&stringp, delim);
        tokens[count] = token;
        count++;
    }

    return count;

}



int get_interface_mtu_by_name(struct TEST_INTERFACE *TEST_INTERFACE)
{

    const int NO_INT = -1;

    struct ifreq ifr;

    const char *pIF_NAME = TEST_INTERFACE->IF_NAME;

    strcpy(ifr.ifr_name,pIF_NAME);

    if (ioctl(TEST_INTERFACE->SOCKET_FD, SIOCGIFMTU, &ifr)==0)
    {
        return ifr.ifr_mtu;
    }

    return NO_INT;

}



int get_sock_interface(struct TEST_INTERFACE *TEST_INTERFACE)
{
// This function was gleaned from;
// http://cboard.cprogramming.com/linux-programming/43261-ioctl-request-get
// -hw-address.html
// As far as I can tell its a variation on some open source code originally
// written for Diald, here: http://diald.sourceforge.net/FAQ/diald-faq.html
// So thanks go to the Diald authors Eric Schenk and Gordon Soukoreff!

    const int NO_INT = -1;

    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl(TEST_INTERFACE->SOCKET_FD, SIOCGIFCONF, &ifc) < 0)
    {
        printf("Error: No compatible interfaces found: ioctl(SIOCGIFCONF): %m\n");
        return NO_INT;
    }

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
    {

        // Is this a packet capable device? (Doesn't work with AF_PACKET?)
        if (ifr->ifr_addr.sa_family == AF_INET)
        {

            // Try to get a typical ethernet adapter, not a bridge virt interface
            if (strncmp(ifr->ifr_name, "eth", 3)==0 ||
               strncmp(ifr->ifr_name, "en", 2)==0 ||
               strncmp(ifr->ifr_name, "em", 3)==0 ||
               strncmp(ifr->ifr_name, "wlan", 4)==0)
            {

                strncpy(ifreq.ifr_name,ifr->ifr_name,sizeof(ifreq.ifr_name));

                // Does this device even have hardware address?
                if (ioctl (TEST_INTERFACE->SOCKET_FD, SIOCGIFHWADDR, &ifreq) != 0)
                {
                    printf("Error: Device has no hardware address: \n"
                           "SIOCGIFHWADDR(%s): %m\n", ifreq.ifr_name);
                    return NO_INT;
                }

                // Get the interface index
                ioctl(TEST_INTERFACE->SOCKET_FD, SIOCGIFINDEX, &ifreq);

                // Get the interface name
                strncpy(TEST_INTERFACE->IF_NAME,ifreq.ifr_name,IFNAMSIZ);

                printf("Using device %s with address "
                       "%02x:%02x:%02x:%02x:%02x:%02x, interface index %u\n",
                       ifreq.ifr_name,
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[0],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[1],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[2],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[3],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[4],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[5],
                       ifreq.ifr_ifindex);

                return ifreq.ifr_ifindex;

            } // If eth* || en* || em* || wlan*

        } // If AF_INET

    } // Looping through interface count

    return NO_INT;

}



unsigned long long htonll(unsigned long long val)
{
    if (__BYTE_ORDER == __BIG_ENDIAN) return (val);
    else return __bswap_64(val);
}



void list_interfaces()
{

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    int SOCKET_FD;
    SOCKET_FD = socket(AF_INET, SOCK_RAW, htons(ETH_P_ALL));

    if (getifaddrs(&ifaddr)==-1)
    {
        perror("getifaddrs");
        exit(EX_PROTOCOL);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        // Does this interface sub address family support AF_PACKET
        if (ifa->ifa_addr->sa_family==AF_PACKET)
        {

            // Set the ifreq by interface name
            strncpy(ifreq.ifr_name,ifa->ifa_name,sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (SOCKET_FD, SIOCGIFHWADDR, &ifreq)==0)
            {

                // Get the interface index
                ioctl(SOCKET_FD, SIOCGIFINDEX, &ifreq);

                // Print the current interface details
                printf("Device %s with address %02x:%02x:%02x:%02x:%02x:%02x, "
                       "interface index %u\n",
                       ifreq.ifr_name,
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[0],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[1],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[2],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[3],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[4],
                       (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[5],
                       ifreq.ifr_ifindex);
            }

        }

    }

    freeifaddrs(ifaddr);

    return;

}



unsigned long long ntohll(unsigned long long val)
{
    if (__BYTE_ORDER == __BIG_ENDIAN) return (val);
    else return __bswap_64(val);
}



void print_usage (struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS)
{

    printf ("Usage info; [Mode] [Destination] [Source] [Transport] [Shortcuts] [Other]\n"
            "[Mode]\n"
            "\t-r\tChange to receive (listening) mode.\n"
            "\t\tDefault is transmit mode.\n"
            "[Destination]\n"
            "\t-d\tSpecify a custom desctination MAC address, \n"
            "\t\t-d 11:22:33:44:55:66\n"
            "\t\tWithout this we default to 00:00:5E:00:00:02\n"
            "\t\tas the TX host and :01 as the RX host.\n"
            "\t-g\tTX host skips settings synchronisation with RX host\n"
            "\t\tand begins transmitting regardless of RX state.\n"
            "\t-G\tTX host skips delay calcuation towards RX host\n"
            "\t\tand begins transmitting regardless of RX state.\n"
            "[Source]\n"
            "\t-s\tWithout this we default to 00:00:5E:00:00:01\n"
            "\t\tas the TX host and :02 as the RX host.\n"
            "\t\tSpecify a custom source MAC address, -s 11:22:33:44:55:66\n"
            "\t-i\tSet interface by name. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-I\tSet interface by index. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-l\tList interface indexes (then quit) for use with -i option.\n"
            "[Test Options]\n"
            "\t-a\tAck mode, have the receiver ack each frame during the test\n"
            "\t\t(This will significantly reduce the speed of the test).\n"
            "\t-b\tNumber of bytes to send, default is %lu, default behaviour\n"
            "\t\tis to wait for duration.\n"
            "\t\tOnly one of -t, -c or -b can be used, both override -t,\n"
            "\t\t-b overrides -c.\n"
            "\t-c\tNumber of frames to send, default is %lu, default behaviour\n"
            "\t\tis to wait for duration.\n"
            "\t-e\tSet a custom ethertype, the default is 0x%04x.\n"
            "\t-f\tFrame payload size in bytes, default is %u\n"
            "\t\t(excluding headers, %u bytes on the wire).\n"
            "\t-m\tMax bytes per/second to send, -m 125000 (1Mps).\n"
            "\t-M\tMax bits per/second to send. -M 1000000 (1Mbps).\n"
            "\t-u\tHow frequently the screen shall be updated during a test,\n"
            "\t\tseconds as integer, default is %d\n"
            "\t-t\tTransmition duration, seconds as integer, default is %llu.\n"
            "[Transport]\n"
            "\t-v\tAdd an 802.1q VLAN tag. None in the header by default.\n"
            "\t\tIf using a PCP value with -p a default VLAN of 0 is added.\n"
            "\t-p\tAdd an 802.1p PCP value from 1 to 7 using options -p 1 to\n"
            "\t\t-p 7. If more than one value is given, the highest is used.\n"
            "\t\tDefault is 0 if none specified.\n"
            "\t\t(If no 802.1q tag is set the VLAN 0 will be used).\n"
            "\t-q\tAdd an outter Q-in-Q tag. If used without -v, 1 is used\n"
            "\t\tfor the inner VLAN ID.\n"
            "\t-o\tAdd an 802.1p PCP value to the outer Q-in-Q VLAN tag.\n"
            "\t\tIf no PCP value is specified and a Q-in-Q VLAN ID is,\n"
            "\t\t0 will be used. If no outer Q-in-Q VLAN ID is supplied this\n"
            "\t\toption is ignored. -o 1 to -o 7 like the -p option above.\n"
            "[Additonal Tests]\n"
            "\t-U\tSpecify a minimum and maximum MTU size in bytes then\n"
            "\t\tperform an MTU sweep on the link towards the RX host to\n"
            "\t\tfind the maximum size supported, -U 1400 1600\n"
            "\t-Q\tSpecify an echo interval and timeout value in millis\n"
            "\t\tthen measure link quality (RTT and jitter), using\n"
            "\t\t-Q 1000 1000 (default duration is %llu, see -t).\n"
            "[Other]\n"
            "\t-V|--version Display version\n"
            "\t-h|--help Display this help text\n",
            F_BYTES_DEF,
            F_COUNT_DEF,
            ETHERTYPE_DEF,
            F_SIZE_DEF,
            (F_SIZE_DEF+FRAME_HEADERS->LENGTH),
            APP_PARAMS->STATS_DELAY,
            F_DURATION_DEF,
            F_DURATION_DEF);

}



int set_sock_interface_index(struct TEST_INTERFACE *TEST_INTERFACE)
{

    const int NO_INT = -1;

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr)==-1)
    {
        perror("getifaddrs");
        exit(EX_PROTOCOL);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        // Does this interface sub address family support AF_PACKET
        if (ifa->ifa_addr->sa_family==AF_PACKET)
        {

            // Set the ifreq by interface name
            strncpy(ifreq.ifr_name,ifa->ifa_name,sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (TEST_INTERFACE->SOCKET_FD, SIOCGIFHWADDR, &ifreq)==0)
            {

                // Get the interface index
                ioctl(TEST_INTERFACE->SOCKET_FD, SIOCGIFINDEX, &ifreq);

                // Check if this is the interface index the user specified
                if (ifreq.ifr_ifindex==TEST_INTERFACE->IF_INDEX)
                {

                    // Get the interface name
                    strncpy(TEST_INTERFACE->IF_NAME,ifreq.ifr_name,IFNAMSIZ);

                    printf("Using device %s with address "
                           "%02x:%02x:%02x:%02x:%02x:%02x, interface index %u\n",
                           ifreq.ifr_name,
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[0],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[1],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[2],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[3],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[4],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[5],
                           ifreq.ifr_ifindex);

                    freeifaddrs(ifaddr);

                    return ifreq.ifr_ifindex;

                }

            }

        }

    }

    freeifaddrs(ifaddr);

    return NO_INT;

}



int set_sock_interface_name(struct TEST_INTERFACE *TEST_INTERFACE)
{

    const int NO_INT = -1;

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr)==-1)
    {
        perror("getifaddrs");
        exit(EX_PROTOCOL);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        // Does this interface sub address family support AF_PACKET
        if (ifa->ifa_addr->sa_family==AF_PACKET)
        {

            // Set the ifreq by interface name
            strncpy(ifreq.ifr_name,ifa->ifa_name,sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (TEST_INTERFACE->SOCKET_FD, SIOCGIFHWADDR, &ifreq)==0)
            {

                // Check if this is the interface name the user specified
                if (strcmp(ifreq.ifr_name,(char *)&TEST_INTERFACE->IF_NAME)==0)
                {

                    // Get the interface index
                    ioctl(TEST_INTERFACE->SOCKET_FD, SIOCGIFINDEX, &ifreq);

                    printf("Using device %s with address "
                           "%02x:%02x:%02x:%02x:%02x:%02x, interface index %u\n",
                           ifreq.ifr_name,
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[0],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[1],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[2],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[3],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[4],
                           (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[5],
                           ifreq.ifr_ifindex);

                    freeifaddrs(ifaddr);

                    return ifreq.ifr_ifindex;

                }

            }

        }

    }

    freeifaddrs(ifaddr);

    return NO_INT;

}



void signal_handler(int signal)
{

    // Send a dying gasp to the other host in case the application is ending
    // before the running test has finished

    struct FRAME_HEADERS *FRAME_HEADERS = pFRAME_HEADERS;
    struct TEST_INTERFACE *TEST_INTERFACE = pTEST_INTERFACE;
    struct QM_TEST *QM_TEST = pQM_TEST;

    build_tlv(FRAME_HEADERS, htons(TYPE_APPLICATION), htonl(VALUE_DYINGGASP));

    int TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                        FRAME_HEADERS->LENGTH+FRAME_HEADERS->TLV_SIZE,
                        0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                        sizeof(TEST_INTERFACE->SOCKET_ADDRESS));


    printf("Leaving promiscuous mode\n");

    strncpy(ethreq.ifr_name,TEST_INTERFACE->IF_NAME,IFNAMSIZ);

    if (ioctl(TEST_INTERFACE->SOCKET_FD,SIOCGIFFLAGS,&ethreq)==-1) {
        printf("Error getting socket flags, leaving promiscuous mode failed!\n");
        perror("ioctl() ");
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(TEST_INTERFACE->SOCKET_FD,SIOCSIFFLAGS,&ethreq)==-1) {
        printf("Error setting socket flags, leaving promiscuous mode failed!\n");
        perror("ioctl() ");
    }

    close(TEST_INTERFACE->SOCKET_FD);
    free (QM_TEST->pDELAY_RESULTS);
    free (FRAME_HEADERS->RX_BUFFER);
    free (FRAME_HEADERS->TX_BUFFER);

    exit(signal);

}



void sync_settings(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS * TEST_PARAMS,
                  struct MTU_TEST *MTU_TEST, struct QM_TEST *QM_TEST)
{

    int TX_RET_VAL;
    int RX_LEN;

    build_tlv(FRAME_HEADERS, htons(TYPE_SETTING), htonl(VALUE_SETTING_SUB_TLV));

    // Send test settings to the RX host
    if (APP_PARAMS->TX_MODE == true && APP_PARAMS->TX_SYNC == true)
    {

        printf("\nSynchronising settings with RX host\n");

        // Disable the delay calculation
        if (APP_PARAMS->TX_DELAY != TX_DELAY_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_TXDELAY), htonll(APP_PARAMS->TX_DELAY));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("TX to RX delay calculation disabled\n");

        }

        // Testing with a custom ethertype
        if (FRAME_HEADERS->ETHERTYPE != ETHERTYPE_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_ETHERTYPE), htonll(FRAME_HEADERS->ETHERTYPE));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Ethertype set to 0x%04hx\n", FRAME_HEADERS->ETHERTYPE);

        }

        // Testing with a custom frame size
        if (TEST_PARAMS->F_SIZE != F_SIZE_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMESIZE), htonll(TEST_PARAMS->F_SIZE));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Frame size set to %u\n", TEST_PARAMS->F_SIZE);

        }

        // Testing with a custom duration
        if (TEST_PARAMS->F_DURATION != F_DURATION_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_DURATION), htonll(TEST_PARAMS->F_DURATION));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Test duration set to %llu\n", TEST_PARAMS->F_DURATION);

        }

        // Testing with a custom frame count
        if (TEST_PARAMS->F_COUNT != F_COUNT_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMECOUNT), htonll(TEST_PARAMS->F_COUNT));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Frame count set to %llu\n", TEST_PARAMS->F_COUNT);

        }

        // Testing with a custom byte limit
        if (TEST_PARAMS->F_BYTES != F_BYTES_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_BYTECOUNT), htonll(TEST_PARAMS->F_BYTES));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Byte limit set to %llu\n", TEST_PARAMS->F_BYTES);

        }

        // Testing with a custom max speed limit
        if (TEST_PARAMS->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_MAXSPEED), htonll(TEST_PARAMS->B_TX_SPEED_MAX));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Max TX speed set to %.2fMbps\n", (((float)TEST_PARAMS->B_TX_SPEED_MAX*8)/1000/1000));

        }

        // Testing with a custom inner VLAN PCP value
        if (FRAME_HEADERS->PCP != PCP_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_VLANPCP), htonll(FRAME_HEADERS->PCP));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Inner VLAN PCP value set to %hu\n", FRAME_HEADERS->PCP);

        }

        // Tesing with a custom QinQ PCP value
        if (FRAME_HEADERS->QINQ_PCP != QINQ_PCP_DEF)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_QINQPCP), htonll(FRAME_HEADERS->QINQ_PCP));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("QinQ VLAN PCP value set to %hu\n", FRAME_HEADERS->QINQ_PCP);

        }

        // Tell RX to run in ACK mode
        if (TEST_PARAMS->F_ACK == true)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_ACKMODE), htonll(TEST_PARAMS->F_ACK));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("ACK mode enabled\n");
        }

        // Tell RX an MTU sweep test will be performed
        if (MTU_TEST->ENABLED)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_MTUTEST), htonll(MTU_TEST->ENABLED));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_MTUMIN), htonll(MTU_TEST->MTU_TX_MIN));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_MTUMAX), htonll(MTU_TEST->MTU_TX_MAX));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("MTU sweep test enabled from %u to %u\n", MTU_TEST->MTU_TX_MIN, MTU_TEST->MTU_TX_MAX);
        
        }

        // Tell RX the link quality tests will be performed
        if(QM_TEST->ENABLED)
        {

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_QMTEST), htonll(QM_TEST->ENABLED));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_QMINTERVAL), htonll(QM_TEST->INTERVAL));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_QMTIMEOUT), htonll(QM_TEST->TIMEOUT));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            printf("Link quality tests enabled\n");

        }

        // Let the receiver know all settings have been sent
        build_tlv(FRAME_HEADERS, htons(TYPE_SETTING), htonl(VALUE_SETTING_END));

        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->TLV_SIZE,
                            0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

        printf("Settings have been synchronised\n\n");


    } else if (APP_PARAMS->TX_MODE == false && APP_PARAMS->TX_SYNC == true) {


        printf("\nWaiting for settings from TX host\n");

        // In listening mode RX waits for each parameter to come through,
        // Until TX signals all settings have been sent
        char WAITING = true;

        while (WAITING)
        {

            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER,
                              TEST_PARAMS->F_SIZE_TOTAL, 0, NULL, NULL);

            // All settings have been received
            if (ntohs(*FRAME_HEADERS->RX_TLV_TYPE) == TYPE_SETTING &&
                ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_SETTING_END) {

                WAITING = false;
                printf("Settings have been synchronised\n\n");

            // TX has disabled the TX to RX one way delay calculation
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_TXDELAY) {

                APP_PARAMS->TX_DELAY = (unsigned char)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("TX to RX delay calculation disabled\n");

            // TX has sent a non-default ethertype
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_ETHERTYPE) {

                FRAME_HEADERS->ETHERTYPE = (unsigned short)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("Ethertype set to 0x%04hx\n", FRAME_HEADERS->ETHERTYPE);

            // TX has sent a non-default frame payload size
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_FRAMESIZE) {

                TEST_PARAMS->F_SIZE = (unsigned short)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("Frame size set to %hu\n", TEST_PARAMS->F_SIZE);

            // TX has sent a non-default transmition duration
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_DURATION) {

                TEST_PARAMS->F_DURATION = (unsigned long long)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("Test duration set to %llu\n", TEST_PARAMS->F_DURATION);

            // TX has sent a frame count to use instead of duration
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_FRAMECOUNT) {

                TEST_PARAMS->F_COUNT = (unsigned long long)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("Frame count set to %llu\n", TEST_PARAMS->F_COUNT);

            // TX has sent a total bytes value to use instead of frame count
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_BYTECOUNT) {

                TEST_PARAMS->F_BYTES = (unsigned long long)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("Byte limit set to %llu\n", TEST_PARAMS->F_BYTES);

            // TX speed is limited
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_MAXSPEED) {

                printf("Max TX speed set to %.2fMbps\n", ((float)*FRAME_HEADERS->RX_SUB_TLV_TYPE*8)/1000/1000);

            // TX has set a custom PCP value
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_VLANPCP) {

                FRAME_HEADERS->PCP = (unsigned short)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("PCP value set to %hu\n", FRAME_HEADERS->PCP);

            // TX has set a custom QinQ PCP value
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_QINQPCP) {

                FRAME_HEADERS->QINQ_PCP = (unsigned short)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("QinQ PCP value set to %hu\n", FRAME_HEADERS->QINQ_PCP);

            // TX has requested RX run in ACK mode
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_ACKMODE) {

                TEST_PARAMS->F_ACK = true;
                printf("ACK mode enabled\n");

            // TX has requested MTU sweep test
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_MTUTEST) {

                MTU_TEST->ENABLED = true;
                printf("MTU sweep test enabled\n");

            // TX has set MTU sweep test minimum MTU size
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_MTUMIN) {

                MTU_TEST->MTU_TX_MIN = (unsigned short)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("Minimum MTU set to %u\n", MTU_TEST->MTU_TX_MIN);

            // TX has set MTU sweep test maximum MTU size
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_MTUMAX) {

                MTU_TEST->MTU_TX_MAX = (unsigned short)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                printf("Maximum MTU set to %u\n", MTU_TEST->MTU_TX_MAX);

            // TX has enabled link quality tests
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_QMTEST) {

                QM_TEST->ENABLED = true;
                printf("Link quality tests enabled\n");

            // TX has set echo interval
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_QMINTERVAL) {

                // Convert to ns for use with timespec
                QM_TEST->INTERVAL_NSEC = (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE)*1000000)%1000000000;
                QM_TEST->INTERVAL_SEC =  ((ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE)*1000000)-QM_TEST->INTERVAL_NSEC)/1000000000;
                
            // TX has set echo timeout
            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_QMTIMEOUT) {

                QM_TEST->TIMEOUT_NSEC =  (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE)*1000000)%1000000000;
                QM_TEST->TIMEOUT_SEC =   ((ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE)*1000000)-QM_TEST->TIMEOUT_NSEC)/1000000000;
                
            }

        } // WAITING

      
    } // TX or RX mode


    return;

}
