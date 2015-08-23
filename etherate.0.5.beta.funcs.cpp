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
 * void build_headers(struct FRAME_HEADERS *FRAME_HEADERS)
 * int cli_args(int argc, char *argv[], struct APP_PARAMS *APP_PARAMS,
             struct FRAME_HEADERS *FRAME_HEADERS, struct TEST_INTERFACE *TEST_INTERFACE,
             struct TEST_PARAMS *TEST_PARAMS, struct MTU_TEST *MTU_TEST,
             struct QM_TEST *QM_TEST)
 * int get_interface_mtu_by_name(struct TEST_INTERFACE *TEST_INTERFACE)
 * int get_sock_interface(struct TEST_INTERFACE *TEST_INTERFACE)
 * void list_interfaces()
 * void print_examples ()
 * void print_usage()
 * int set_sock_interface_index(struct TEST_INTERFACE *TEST_INTERFACE);
 * int set_sock_interface_name(struct TEST_INTERFACE *TEST_INTERFACE);
 * void signal_handler(int signal);
 * void string_explode(string str, string separator, vector<string>* results);
 * void sync_settings(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS * TEST_PARAMS,
                  struct MTU_TEST *MTU_TEST, struct QM_TEST *QM_TEST);
 *
 */



// Build the Ethernet headers for sending frames
void build_headers(struct FRAME_HEADERS *FRAME_HEADERS);

// Process the CLI arguments
int cli_args(int argc, char *argv[], struct APP_PARAMS *APP_PARAMS,
             struct FRAME_HEADERS *FRAME_HEADERS, struct TEST_INTERFACE *TEST_INTERFACE,
             struct TEST_PARAMS *TEST_PARAMS, struct MTU_TEST *MTU_TEST,
             struct QM_TEST *QM_TEST);

// Get the MTU of the interface used for the test
int get_interface_mtu_by_name(struct TEST_INTERFACE *TEST_INTERFACE);

// Try to automatically chose an interface to run the test on
int get_sock_interface(struct TEST_INTERFACE *TEST_INTERFACE);

// List interfaces and hardware (MAC) address
void list_interfaces();

// Print common CLI examples
void print_examples ();

// Print CLI args and usage
void print_usage();

// Try to open the passed socket on a user specified interface by index
int set_sock_interface_index(struct TEST_INTERFACE *TEST_INTERFACE);

// Try to open the passed socket on a user specified interface by name
int set_sock_interface_name(struct TEST_INTERFACE *TEST_INTERFACE);

// Signal handler to notify remote host of local application termiantion
void signal_handler(int signal);

// Explode a string into an array using a seperator value
void string_explode(string str, string separator, vector<string>* results);

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

    // Check to see if a PCP value or VLAN ID has been supplier
    // for the main etherate frame header
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

      // Push on the ETHERTYPE for the Etherate payload
      TPI = htons(FRAME_HEADERS->ETHERTYPE);
      memcpy((void*)(FRAME_HEADERS->TX_BUFFER+BUFFER_OFFSET), pTPI, sizeof(TPI));
      BUFFER_OFFSET+=sizeof(TPI);

      FRAME_HEADERS->LENGTH = BUFFER_OFFSET;

}


int cli_args(int argc, char *argv[], struct APP_PARAMS *APP_PARAMS,
             struct FRAME_HEADERS *FRAME_HEADERS, struct TEST_INTERFACE *TEST_INTERFACE,
             struct TEST_PARAMS *TEST_PARAMS, struct MTU_TEST *MTU_TEST,
             struct QM_TEST *QM_TEST)
{

    int RET_EX_USAGE = -1;
    int RET_EXIT_SUCCESS = 0;

    ///// THIS FILTH NEEDS TO BE REMOVED!!!!
    vector<string> exploded;
    string explodestring;

    if (argc > 1) 
    {

        for (int i=1; i<argc; i++) 
        {

            // Change to receive mode
            if (strncmp(argv[i],"-r",2)==0) 
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
            } else if (strncmp(argv[i],"-d",2)==0) {
                explodestring = argv[i+1];
                exploded.clear();
                string_explode(explodestring, ":", &exploded);

                if ((int) exploded.size() != 6) 
                {
                    cout << "Error: Invalid destination MAC address!"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }

                cout << "Destination MAC ";

                // Here strtoul() is used to convert the interger value of c_str()
                // to a hex number by giving it the number base 16 (hexadecimal)
                // otherwise it would be atoi()
                for (int j = 0; (j < 6) && (j < exploded.size()); ++j)
                {
                    FRAME_HEADERS->DESTINATION_MAC[j] = (unsigned char)strtoul(exploded[j].c_str(),
                                          NULL, 16);

                    cout << setw(2)<<setfill('0')<<hex<<int(FRAME_HEADERS->DESTINATION_MAC[j])<<":";
                }

                cout << dec << endl;
                i++;


            // Disable settings sync between TX and RX
            } else if (strncmp(argv[i],"-g",2)==0) {
                APP_PARAMS->TX_SYNC = false;


            // Specifying a custom source MAC address
            } else if (strncmp(argv[i],"-s",2)==0) {
                explodestring = argv[i+1];
                exploded.clear();
                string_explode(explodestring, ":", &exploded);

                if ((int) exploded.size() != 6) 
                {
                    cout << "Error: Invalid source MAC address!"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }
                cout << "Custom source MAC ";

                for (int j = 0; (j < 6) && (j < exploded.size()); ++j)
                {
                    FRAME_HEADERS->SOURCE_MAC[j] = (unsigned char)strtoul(exploded[j].c_str(),
                                     NULL, 16);

                    cout << setw(2)<<setfill('0')<<hex<<int(FRAME_HEADERS->SOURCE_MAC[j])<<":";
                }
                cout << dec << endl;
                i++;


            // Specifying a custom interface name
            } else if (strncmp(argv[i],"-i",2)==0) {
                if (argc > (i+1))
                {
                    strncpy(TEST_INTERFACE->IF_NAME,argv[i+1],sizeof(argv[i+1]));
                    i++;
                } else {
                    cout << "Oops! Missing interface name"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Specifying a custom interface index
            } else if (strncmp(argv[i],"-I",2)==0) {
                if (argc > (i+1))
                {
                    TEST_INTERFACE->IF_INDEX = atoi(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing interface index"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Requesting to list interfaces
            } else if (strncmp(argv[i],"-l",2)==0) {
                list_interfaces();
                return EXIT_SUCCESS;


            // Specifying a custom frame payload size in bytes
            } else if (strncmp(argv[i],"-f",2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_SIZE = atoi(argv[i+1]);
                    if (TEST_PARAMS->F_SIZE > 1500)
                    {
                        cout << "WARNING: Make sure your device supports baby"
                             << " giants or jumbo frames as required"<<endl;
                    }
                    i++;

                } else {
                    cout << "Oops! Missing frame size"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Specifying a custom transmission duration in seconds
            } else if (strncmp(argv[i],"-t",2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_DURATION = atoll(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing transmission duration"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Specifying the total number of frames to send instead of duration
            } else if (strncmp(argv[i],"-c",2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_COUNT = atoll(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing max frame count"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Specifying the total number of bytes to send instead of duration
            } else if (strncmp(argv[i],"-b",2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->F_BYTES = atoll(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing max byte transfer limit"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Enable ACK mode testing
            } else if (strncmp(argv[i],"-a",2)==0) {
                TEST_PARAMS->F_ACK = true;


            // Limit TX rate to max bytes per second
            } else if (strncmp(argv[i],"-m",2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->B_TX_SPEED_MAX = atol(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing max TX rate"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Limit TX rate to max bits per second
            } else if (strncmp(argv[i],"-M",2)==0) {
                if (argc > (i+1))
                {
                    TEST_PARAMS->B_TX_SPEED_MAX = atol(argv[i+1]) / 8;
                    i++;
                } else {
                    cout << "Oops! Missing max TX rate"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Set 802.1p PCP value
            } else if (strncmp(argv[i],"-p",2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->PCP = atoi(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing 802.1p PCP value"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Set 802.1q VLAN ID
            } else if (strncmp(argv[i],"-v",2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->VLAN_ID = atoi(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing 802.1p VLAN ID"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Set 802.1ad QinQ outer VLAN ID
            } else if (strncmp(argv[i],"-q",2)==0) {
                if (argc > (i+1))
                {
                    FRAME_HEADERS->QINQ_ID = atoi(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing 802.1ad QinQ outer VLAN ID"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Set 802.1ad QinQ outer PCP value
            } else if (strncmp(argv[i],"-o",2)==0){
                if (argc > (i+1))
                {
                    FRAME_HEADERS->QINQ_PCP = atoi(argv[i+1]);
                    i++;
                } else {
                    cout << "Oops! Missing 802.1ad QinQ outer PCP value"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Set a custom ETHERTYPE
            } else if (strncmp(argv[i],"-e",2)==0){
                if (argc > (i+1))
                {
                    FRAME_HEADERS->ETHERTYPE = (int)strtoul(argv[i+1], NULL, 16);
                    i++;
                } else {
                    cout << "Oops! Missing ETHERTYPE value"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Run an MTU sweep test
            } else if (strncmp(argv[i],"-U",2)==0){
                if (argc > (i+1))
                {
                    MTU_TEST->MTU_TX_MIN = atoi(argv[i+1]);
                    MTU_TEST->MTU_TX_MAX = atoi(argv[i+2]);
                    if (MTU_TEST->MTU_TX_MAX > F_SIZE_MAX) { 
                        cout << "MTU size can not exceed the maximum hard"
                             << " coded frame size: "<<F_SIZE_MAX<<endl;
                             return RET_EX_USAGE;
                    }
                    MTU_TEST->ENABLED = true;
                    i+=2;
                } else {
                    cout << "Oops! Missing min/max MTU sizes"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Perform the link quality measurement
            } else if (strncmp(argv[i],"-Q",2)==0){
                if (argc > (i+1))
                {
                    QM_TEST->INTERVAL = atoi(argv[i+1]);
                    QM_TEST->TIMEOUT = atoi(argv[i+2]);
                    if (QM_TEST->TIMEOUT > QM_TEST->INTERVAL) { 
                        cout << "Oops! The echo timeout can not exceed the "
                             << "interval "<<endl;
                             return RET_EX_USAGE;
                    }
                    // Convert to ns for use with timespec
                    QM_TEST->INTERVAL_NSEC = (QM_TEST->INTERVAL*1000000)%1000000000;
                    QM_TEST->INTERVAL_SEC = ((QM_TEST->INTERVAL*1000000)-QM_TEST->INTERVAL_NSEC)/1000000000;
                    QM_TEST->TIMEOUT_NSEC = (QM_TEST->TIMEOUT*1000000)%1000000000;
                    QM_TEST->TIMEOUT_SEC = ((QM_TEST->TIMEOUT*1000000)-QM_TEST->TIMEOUT_NSEC)/1000000000;
                    QM_TEST->ENABLED = true;
                    i+=2;
                } else {
                    cout << "Oops! Missing interval and timeout values"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return RET_EX_USAGE;
                }


            // Display version
            } else if (strncmp(argv[i],"-V",2)==0 ||
                      strncmp(argv[i],"--version",9)==0) {
                cout << "Etherate version "<<version<<endl;
                return RET_EXIT_SUCCESS;


            // Display usage instructions
            } else if (strncmp(argv[i],"-h",2)==0 ||
                      strncmp(argv[i],"--help",6)==0) {
                print_usage();
                return RET_EXIT_SUCCESS;
            }


        }

        return RET_EXIT_SUCCESS;

    } else {

        return RET_EXIT_SUCCESS;

    }

}


int get_interface_mtu_by_name(struct TEST_INTERFACE *TEST_INTERFACE)
{

    struct TEST_INTERFACE *pTEST_INTERFACE = TEST_INTERFACE;

    const int NO_INT = 0;

    struct ifreq ifr;

    const char * IF_NAME_P = pTEST_INTERFACE->IF_NAME;

    strcpy(ifr.ifr_name,IF_NAME_P);

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

    const int NO_INT = 0;

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


void list_interfaces()
{

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    int SOCKET_FD, counter;
    SOCKET_FD = socket(AF_INET, SOCK_RAW, htons(ETH_P_ALL));

    if (getifaddrs(&ifaddr)==-1)
    {
        perror("getifaddrs");
        exit(EX_PROTOCOL);
    }

    for (ifa = ifaddr, counter = 0; ifa != NULL; ifa = ifa->ifa_next, counter++)
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

    return;

}


void print_examples ()
{

    return;
}


void print_usage ()
{

    struct FRAME_HEADERS *FRAME_HEADERS = pFRAME_HEADERS;

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
            "[Source]\n"
            "\t-s\tWithout this we default to 00:00:5E:00:00:01\n"
            "\t\tas the TX host and :02 as the RX host.\n"
            "\t\tSpecify a custom source MAC address, -s 11:22:33:44:55:66\n"
            "\t-i\tSet interface by name. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-I\tSet interface by index. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-l\tList interface indexes (then quit) for use with -i option.\n"
            "[Options]\n"
            "\t-a\tAck mode, have the receiver ack each frame during the test\n"
            "\t\t(This will significantly reduce the speed of the test).\n"
            "\t-b\tNumber of bytes to send, default is %ld, default behaviour\n"
            "\t\tis to wait for duration.\n"
            "\t\tOnly one of -t, -c or -b can be used, both override -t,\n"
            "\t\t-b overrides -c.\n"
            "\t-c\tNumber of frames to send, default is %ld, default behaviour\n"
            "\t\tis to wait for duration.\n"
            "\t-e\tSet a custom ETHERTYPE value the default is 0x0800 (IPv4).\n"
            "\t-f\tFrame payload size in bytes, default is %d\n"
            "\t\t(%d bytes is the expected size on the wire with headers).\n"
            "\t-m\tMax bytes per/second to send, -m 125000 (1Mps).\n"
            "\t-M\tMax bits per/second to send. -M 1000000 (1Mbps).\n"
            "\t-t\tTransmition duration, integer in seconds, default is %ld.\n"
            "[Transport]\n"
            "\t-v\tAdd an 802.1q VLAN tag. None in the header by default.\n"
            "\t\tIf using a PCP value with -p a default VLAN of 0 is added.\n"
            "\t-p\tAdd an 802.1p PCP value from 1 to 7 using options -p 1 to\n"
            "\t\t-p 7. If more than one value is given, the highest is used.\n"
            "\t\tDefault is 0 if none specified.\n"
            "\t\t(If no 802.1q tag is set the VLAN 0 will be used).\n"
            "\t-q\tAdd an outter Q-in-Q tag. If used without -v, 1 is used\n"
            "\t\tfor the inner VLAN ID.\n"
            "\t\t#NOT IMPLEMENTED YET#\n"
            "\t-o\tAdd an 802.1p PCP value to the outer Q-in-Q VLAN tag.\n"
            "\t\tIf no PCP value is specified and a Q-in-Q VLAN ID is,\n"
            "\t\t0 will be used. If no outer Q-in-Q VLAN ID is supplied this\n"
            "\t\toption is ignored. -o 1 to -o 7 like the -p option above.\n"
            "\t\t#NOT IMPLEMENTED YET#\n"
            "[Addtional Tests]\n"
            "\t-U\tSpecify a minimum and maximum MTU size in bytes then\n"
            "\t\tperform an MTU sweep on the link towards the RX host to\n"
            "\t\tfind the maximum size supported, such as -U 1400 1600\n"
            "\t-Q\tSpecify an echo interval and timeout value in millis\n"
            "\t\tthen measure link quality (RTT, jitter, loss), using\n"
            "\t\t-Q 1000 1000 (default duration is %ld, see -t).\n"
            "[Other]\n"
            "\t-x\tDisplay examples.\n"
            "\t\t#NOT IMPLEMENTED YET#\n"
            "\t-V|--version Display version\n"
            "\t-h|--help Display this help text\n",
            F_BYTES_DEF, F_COUNT_DEF, F_SIZE_DEF, (F_SIZE_DEF+FRAME_HEADERS->LENGTH),
            F_DURATION_DEF, F_DURATION_DEF);

}


int set_sock_interface_index(struct TEST_INTERFACE *TEST_INTERFACE)
{

    const int NO_INT = 0;

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    int counter;

    if (getifaddrs(&ifaddr)==-1)
    {
        perror("getifaddrs");
        exit(EX_PROTOCOL);
    }

    for (ifa = ifaddr, counter = 0; ifa != NULL; ifa = ifa->ifa_next, counter++)
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

                    return ifreq.ifr_ifindex;
                }

            }

        }

    }

    return NO_INT;

}


int set_sock_interface_name(struct TEST_INTERFACE *TEST_INTERFACE)
{

    const int NO_INT = 0;

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    int counter;

    if (getifaddrs(&ifaddr)==-1)
    {
        perror("getifaddrs");
        exit(EX_PROTOCOL);
    }

    for (ifa = ifaddr, counter = 0; ifa != NULL; ifa = ifa->ifa_next, counter++)
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

                    return ifreq.ifr_ifindex;
                }

            }

        }

    }

    return NO_INT;

}


void signal_handler(int signal)
{
// The purpose of this signal handling function is to catch
// a user killing the test host before the test has ended, telling the
// other host we are doing down, to give it a chance to reset.

    struct FRAME_HEADERS *FRAME_HEADERS = pFRAME_HEADERS;
    struct TEST_INTERFACE *TEST_INTERFACE = pTEST_INTERFACE;

    /////REMOVE THIS FILTH
    std::string param;
    std::stringstream ss;
    ss << "etheratedeath";
    param = ss.str();


    strncpy(FRAME_HEADERS->TX_DATA,param.c_str(), param.length());

    int TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                            FRAME_HEADERS->LENGTH+param.length(), 0,
                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));


    cout << "Leaving promiscuous mode" << endl;

    strncpy(ethreq.ifr_name,TEST_INTERFACE->IF_NAME,IFNAMSIZ);

    if (ioctl(TEST_INTERFACE->SOCKET_FD,SIOCGIFFLAGS,&ethreq)==-1) {
        cout << "Error getting socket flags, entering promiscuous mode failed."
             << endl;
        perror("ioctl() ");
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(TEST_INTERFACE->SOCKET_FD,SIOCSIFFLAGS,&ethreq)==-1) {
        cout << "Error setting socket flags, promiscuous mode failed." << endl;
        perror("ioctl() ");
    }

    close(TEST_INTERFACE->SOCKET_FD);

    exit(130);

}


void string_explode(string str, string separator, vector<string>* results)
{
// This function was gleaned from;
// http://www.infernodevelopment.com/perfect-c-string-explode-split
// Thanks to the author Baran Ornarli

    int found;
    results->clear();
    found = str.find_first_of(separator);

    while (found != string::npos)
    {
        if (found > 0) results->push_back(str.substr(0,found)); 
        str = str.substr(found+1);
        found = str.find_first_of(separator);
    }

    if (str.length() > 0) results->push_back(str);

}

void sync_settings(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS * TEST_PARAMS,
                  struct MTU_TEST *MTU_TEST, struct QM_TEST *QM_TEST)
{

    ///// REMOVE THIS FILTH
    std::string param;
    std::stringstream ss;
    ss.setf(ios::fixed);
    ss.setf(ios::showpoint);
    ss.precision(9);
    cout<<fixed<<setprecision(9);
    ///// AND THIS FILTH
    vector<string> exploded;
    string explodestring;


    int TX_RET_VAL;
    int RX_LEN;

    struct TEST_INTERFACE *pTEST_INTERFACE = TEST_INTERFACE;
    struct QM_TEST *pQM_TEST = QM_TEST;


    // Set up the test by communicating settings with the RX host receiver
    if (APP_PARAMS->TX_MODE == true && APP_PARAMS->TX_SYNC == true)
    {

        cout << "Running in TX mode, synchronising settings"<<endl;

        // Testing with a custom frame size
        if (FRAME_HEADERS->ETHERTYPE != ETHERTYPE_DEF)
        {
            ss << "etherateethertype"<<FRAME_HEADERS->ETHERTYPE;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "ETHERTYPE set to "<<FRAME_HEADERS->ETHERTYPE<<endl;
        }

        // Testing with a custom frame size
        if (TEST_PARAMS->F_SIZE != F_SIZE_DEF)
        {
            ss << "etheratesize"<<TEST_PARAMS->F_SIZE;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "Frame size set to "<<TEST_PARAMS->F_SIZE<<endl;
        }


        // Testing with a custom duration
        if (TEST_PARAMS->F_DURATION != F_DURATION_DEF)
        {
            ss << "etherateduration"<<TEST_PARAMS->F_DURATION;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout <<"Test duration set to "<<TEST_PARAMS->F_DURATION<<endl;
        }


        // Testing with a custom frame count
        if (TEST_PARAMS->F_COUNT != F_COUNT_DEF)
        {
            ss << "etheratecount"<<TEST_PARAMS->F_COUNT;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout <<"Frame count set to "<<TEST_PARAMS->F_COUNT<<endl;
        }


        // Testing with a custom byte limit
        if (TEST_PARAMS->F_BYTES != F_BYTES_DEF)
        {
            ss << "etheratebytes"<<TEST_PARAMS->F_BYTES;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "Byte limit set to "<<TEST_PARAMS->F_BYTES<<endl;
        }


        // Testing with a custom max speed limit
        if (TEST_PARAMS->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
        {
            ss << "etheratespeed"<<TEST_PARAMS->B_TX_SPEED_MAX;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "Max TX speed set to "<<((TEST_PARAMS->B_TX_SPEED_MAX*8)/1000/1000)<<"Mbps"<<endl;
        }

        // Testing with a custom inner VLAN PCP value
        if (FRAME_HEADERS->PCP != PCP_DEF)
        {
            ss << "etheratepcp"<<FRAME_HEADERS->PCP;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH, 
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "Inner VLAN PCP value set to "<<FRAME_HEADERS->PCP<<endl;
        }

        // Tesing with a custom QinQ PCP value
        if (FRAME_HEADERS->QINQ_PCP != QINQ_PCP_DEF)
        {
            ss << "etherateQINQ_PCP"<<FRAME_HEADERS->QINQ_PCP;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "QinQ VLAN PCP value set to "<<FRAME_HEADERS->QINQ_PCP<<endl;
        }

        // Tell the receiver to run in ACK mode
        if (TEST_PARAMS->F_ACK == true)
        {
            param = "etherateack";
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "ACK mode enabled"<<endl;
        }

        // Tell RX we will run an MTU sweep test
        if (MTU_TEST->ENABLED == true)
        {
            ss.str("");
            ss.clear();
            ss<<"etheratemtusweep:"<<MTU_TEST->MTU_TX_MIN<<":"<<MTU_TEST->MTU_TX_MAX<<":";
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            cout << "MTU sweep test enabled from "<<MTU_TEST->MTU_TX_MIN<<" to "<<MTU_TEST->MTU_TX_MAX<<endl;
        }


        // Send over the time for delay calculation between hosts, repeat the
        // test QM_TEST->DELAY_TEST_COUNT times for interrupt coalescing and to make an average
        cout << "Calculating delay between hosts..." << endl;

        // Make sure the RX host is ready for us to begin the delay tests
        sleep (2);

        for (int i = 1; i <= QM_TEST->DELAY_TEST_COUNT; i++)
        {
            // The monotonic value is used here in case we are unlucky enough to
            // run this during and NTP update, so we stay consistent
            clock_gettime(CLOCK_MONOTONIC_RAW, &pQM_TEST->TS_RTT);
            ss.str("");
            ss.clear();
            ss<<"etheratetime"<<i<<"1:"<<QM_TEST->TS_RTT.tv_sec<<":"<<QM_TEST->TS_RTT.tv_nsec;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);
            ss.str("");
            ss.clear();
            ss<<"etheratetime"<<i<<"2:"<<QM_TEST->TS_RTT.tv_sec<<":"<<QM_TEST->TS_RTT.tv_nsec;
            param = ss.str();
            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            // After sending the 2nd time value we wait for RX to return the
            // calculated delay value;
            bool TX_DELAY_WAITING = true;
            ss.str("");
            ss.clear();
            ss<<"etheratedelay.";
            param = ss.str();

            while (TX_DELAY_WAITING)
            {
                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, TEST_PARAMS->F_SIZE_TOTAL, 0, NULL, NULL);

                if (param.compare(0,param.size(),FRAME_HEADERS->RX_DATA,0,14) == 0)
                {
                    exploded.clear();
                    explodestring = FRAME_HEADERS->RX_DATA;
                    string_explode(explodestring, ".", &exploded);
                    ss.str("");
                    ss.clear();
                    ss << exploded[1].c_str()<<"."<<exploded[2].c_str();
                    param = ss.str();
                    QM_TEST->pDELAY_RESULTS[i] = strtod(param.c_str(), NULL);
                    TX_DELAY_WAITING = false;
                }

            }

        } // End delay TX loop


        // Let the receiver know all delay values have been sent
        param = "etheratedelayfinished";
        strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                            0, (struct sockaddr*)&pTEST_INTERFACE->SOCKET_ADDRESS,
                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

        double DELAY_AVERAGE = 0;

        for (int i=1; i<=QM_TEST->DELAY_TEST_COUNT; i++)
        {
            DELAY_AVERAGE += QM_TEST->pDELAY_RESULTS[i];        }

        DELAY_AVERAGE = (DELAY_AVERAGE/QM_TEST->DELAY_TEST_COUNT);

        cout << "Tx to Rx delay calculated as "<<DELAY_AVERAGE<<" seconds"<<endl;


        // Let the receiver know all settings have been sent
        param = "etherateallset";
        strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                            0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

        cout << "Settings have been synchronised"<<endl<<endl;


    } else if (APP_PARAMS->TX_MODE == false && APP_PARAMS->TX_SYNC == true) {

        cout << "Running in RX mode, awaiting TX host"<<endl;
        // In listening mode we start by waiting for each parameter to come through
        // So we start a loop until they have all been received
        bool SYNC_WAITING = true;

        // These values are used to calculate the delay between TX and RX hosts
        //double timeTX1,timeRX1,timeTX2,timeRX2,timeTXdiff,timeRXdiff,timeTXdelay;
        double TIME_TX_1[QM_TEST->DELAY_TEST_COUNT];
        double TIME_TX_2[QM_TEST->DELAY_TEST_COUNT];
        double TIME_RX_1[QM_TEST->DELAY_TEST_COUNT];
        double TIME_RX_2[QM_TEST->DELAY_TEST_COUNT];
        double TIME_TX_DIFF[QM_TEST->DELAY_TEST_COUNT];
        double TIME_RX_DIFF[QM_TEST->DELAY_TEST_COUNT];
        int DELAY_INDEX = 1;

        // In this loop we are grabbing each incoming frame and looking at the 
        // string that prefixes it, to state which variable the following value
        // will be for
        while (SYNC_WAITING)
        {

            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, TEST_PARAMS->F_SIZE_TOTAL, 0, NULL, NULL);

            // TX has sent a non-default ethertype
            if (strncmp(FRAME_HEADERS->RX_DATA,"etherateethertype",17) == 0)
            {
                /////diff = (RX_LEN-17);
                ss << FRAME_HEADERS->RX_DATA;
                param = ss.str().substr(RX_LEN-17);;
                FRAME_HEADERS->ETHERTYPE = strtol(param.c_str(),NULL,10);
                printf("ETHERTYPE set to 0x%X\n", FRAME_HEADERS->ETHERTYPE);

            }


            // TX has sent a non-default frame payload size
            if (strncmp(FRAME_HEADERS->RX_DATA,"etheratesize",12) == 0)
            {
                /////diff = (RX_LEN-12);
                ss << FRAME_HEADERS->RX_DATA;
                param = ss.str().substr(12,(RX_LEN-12));
                TEST_PARAMS->F_SIZE = strtol(param.c_str(),NULL,10);
                cout << "Frame size set to " << TEST_PARAMS->F_SIZE << endl;
            }


            // TX has sent a non-default transmition duration
            if (strncmp(FRAME_HEADERS->RX_DATA,"etherateduration",16) == 0)
            {
                /////diff = (RX_LEN-16);
                ss << FRAME_HEADERS->RX_DATA;
                param = ss.str().substr(16,(RX_LEN-16));
                TEST_PARAMS->F_DURATION = strtoull(param.c_str(),0,10);
                cout << "Test duration set to " << TEST_PARAMS->F_DURATION << endl;
            }


            // TX has sent a frame count to use instead of duration
            if (strncmp(FRAME_HEADERS->RX_DATA,"etheratecount",13) == 0)
            {
                /////diff = (RX_LEN-13);
                ss << FRAME_HEADERS->RX_DATA;
                param = ss.str().substr(13,(RX_LEN-13));
                TEST_PARAMS->F_COUNT = strtoull(param.c_str(),0,10);
                cout << "Frame count set to " << TEST_PARAMS->F_COUNT << endl;
            }


            // TX has sent a total bytes value to use instead of frame count
            if (strncmp(FRAME_HEADERS->RX_DATA,"etheratebytes",13) == 0)
            {
                //////diff = (RX_LEN-13);
                ss << FRAME_HEADERS->RX_DATA;
                param = ss.str().substr(13,(RX_LEN-13));
                TEST_PARAMS->F_BYTES = strtoull(param.c_str(),0,10);
                cout << "Byte limit set to " << TEST_PARAMS->F_BYTES << endl;
            }

            // TX has set a custom PCP value
            if (strncmp(FRAME_HEADERS->RX_DATA,"etheratepcp",11) == 0)
            {
                /////diff = (RX_LEN-11);
                ss << FRAME_HEADERS->RX_DATA;
                param = ss.str().substr(11,(RX_LEN-11));
                FRAME_HEADERS->PCP = strtoull(param.c_str(),0,10);
                cout << "PCP value set to " << FRAME_HEADERS->PCP << endl;
            }

            // TX has set a custom PCP value
            if (strncmp(FRAME_HEADERS->RX_DATA,"etherateqinqpcp",15) == 0)
            {
                /////diff = (RX_LEN-15);
                ss << FRAME_HEADERS->RX_DATA;
                param = ss.str().substr(15,(RX_LEN-15));
                FRAME_HEADERS->QINQ_PCP = strtoull(param.c_str(),0,10);
                cout << "QinQ PCP value set to "<<FRAME_HEADERS->QINQ_PCP<<endl;
            }

            // TX has requested RX run in ACK mode
            if (strncmp(FRAME_HEADERS->RX_DATA,"etherateack",11) == 0)
            {
                TEST_PARAMS->F_ACK = true;
                cout << "ACK mode enabled" << endl;
            }

            // Tx has requested MTU sweep test
            if (strncmp(FRAME_HEADERS->RX_DATA,"etheratemtusweep",16) == 0)
            {
                exploded.clear();
                explodestring = FRAME_HEADERS->RX_DATA;
                string_explode(explodestring, ":", &exploded);
                MTU_TEST->MTU_TX_MIN = atoi(exploded[1].c_str());
                MTU_TEST->MTU_TX_MAX = atoi(exploded[2].c_str());
                MTU_TEST->ENABLED = true;
                cout << "MTU sweep test enabled from "<<MTU_TEST->MTU_TX_MIN<<" to "
                     <<  MTU_TEST->MTU_TX_MAX<<endl;
            }

            // Now begin the part to calculate the delay between TX and RX hosts.
            // Several time values will be exchanged to estimate the delay between
            // the two. Then the process is repeated two more times so and average
            // can be taken, which is shared back with TX;
            if (strncmp(FRAME_HEADERS->RX_DATA,"etheratetime",12) == 0) {

                // Get the time we are receiving TX's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &pQM_TEST->TS_RTT);

                // Extract the sent time
                exploded.clear();
                explodestring = FRAME_HEADERS->RX_DATA;
                string_explode(explodestring, ":", &exploded);

                if (exploded[0].compare(exploded[0].length()-1,1,"1") == 0)
                {

                    // Record the time we received this TX value
                    ss.str("");
                    ss.clear();
                    ss << QM_TEST->TS_RTT.tv_sec<<"."<<QM_TEST->TS_RTT.tv_nsec;
                    ss >> TIME_RX_1[DELAY_INDEX];

                    // Record the TX value
                    ss.str("");
                    ss.clear();
                    ss << atol(exploded[1].c_str())<<"."<<atol(exploded[2].c_str());
                    ss >> TIME_TX_1[DELAY_INDEX];

                }
                  else if (exploded[0].compare(exploded[0].length()-1,1,"2") == 0)
                {

                    // Record the time we received this TX value
                    ss.str("");
                    ss.clear();
                    ss << QM_TEST->TS_RTT.tv_sec<<"."<<QM_TEST->TS_RTT.tv_nsec;
                    ss >> TIME_RX_2[DELAY_INDEX];

                    // Record the TX value
                    ss.clear();
                    ss.str("");
                    ss << atol(exploded[1].c_str())<<"."<<atol(exploded[2].c_str());
                    ss >> TIME_TX_2[DELAY_INDEX];

                    // Calculate the delay
                    TIME_TX_DIFF[DELAY_INDEX] = TIME_TX_2[DELAY_INDEX]-TIME_TX_1[DELAY_INDEX];
                    TIME_RX_DIFF[DELAY_INDEX] = TIME_RX_2[DELAY_INDEX]-TIME_RX_1[DELAY_INDEX];

                    if (TIME_RX_DIFF[DELAY_INDEX]-TIME_TX_DIFF[DELAY_INDEX] > 0) {

                        QM_TEST->pDELAY_RESULTS[DELAY_INDEX] = TIME_RX_DIFF[DELAY_INDEX]-TIME_TX_DIFF[DELAY_INDEX];

                    // This value has returned as minus and is invalid
                    } else {
                        QM_TEST->pDELAY_RESULTS[DELAY_INDEX] = 0;
                    }

                    // Send the delay back to the TX host
                    ss.clear();
                    ss.str("");
                    ss << "etheratedelay."<<QM_TEST->pDELAY_RESULTS[DELAY_INDEX];
                    param = ss.str();
                    strncpy(FRAME_HEADERS->TX_DATA,param.c_str(), param.length());
                    TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                        param.length()+FRAME_HEADERS->LENGTH, 0,
                                        (struct sockaddr*)&pTEST_INTERFACE->SOCKET_ADDRESS,
                                        sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                    DELAY_INDEX++;

                }


            } // End if delay value


            if (strncmp(FRAME_HEADERS->RX_DATA,"etheratedelayfinished",21) == 0)
            {

                double DELAY_AVERAGE = 0;

                for (int  i=1; i<=QM_TEST->DELAY_TEST_COUNT; i++)
                {
                    DELAY_AVERAGE += QM_TEST->pDELAY_RESULTS[i];
                }

                DELAY_AVERAGE = (DELAY_AVERAGE/QM_TEST->DELAY_TEST_COUNT);

                cout << "Tx to Rx delay calculated as "<<DELAY_AVERAGE<<" seconds"<<endl;

            }

            // All settings have been received
            if (strncmp(FRAME_HEADERS->RX_DATA,"etherateallset",14) == 0)
            {
                SYNC_WAITING = false;
                cout<<"Settings have been synchronised"<<endl<<endl;
            }

        } // SYNC_WAITING bool

      
    } // TX or RX mode


    return;

}