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
 * File: Etherate Functions
 *
 * Updates: https://github.com/jwbensley/Etherate
 * http://null.53bits.co.uk/index.php?page=etherate
 * Please send corrections, ideas and help to: jwbensley@gmail.com 
 * (I'm a beginner if that isn't obvious!)
 *
 * Compile: g++ -o etherate etherate.cpp -lrt
 * 
 * File Contents:
 *
 * void build_headers(char* &TX_BUFFER, unsigned char (&DESTINATION_MAC)[6], 
        unsigned char (&SOURCE_MAC)[6], int &ETHERTYPE, short &PCP,
        short &VLAN_ID, short &QINQ_ID, short &QINQ_PCP, int &ETH_HEADERS_LEN);
 * int get_interface_mtu_by_name(int &SOCKET_FD, char &IF_NAME);
 * int get_sock_interface(int &SOCKET_FD);
 * void list_interfaces();
 * void print_examples ();
 * void print_usage();
 * int set_sock_interface_index(int &SOCKET_FD, int &IF_INDEX);
 * int set_sock_interface_name(int &SOCKET_FD, char &IF_NAME);
 * void signal_handler(int signal);
 * void string_explode(string str, string separator, vector<string>* results);
 *
 */


void build_headers(char* &TX_BUFFER, unsigned char (&DESTINATION_MAC)[6], 
     unsigned char (&SOURCE_MAC)[6], int &ETHERTYPE, short &PCP,
     short &VLAN_ID, short &QINQ_ID, short &QINQ_PCP, int &ETH_HEADERS_LEN) {

    int BUFFER_OFFSET = 0;
    short TPI = 0;
    short TCI = 0;
    short *p = &TPI;
    short *c = &TCI;
    short VLAN_ID_TMP;

    // Copy the destination and source MAC addresses
    memcpy((void*)TX_BUFFER, (void*)DESTINATION_MAC, ETH_ALEN);
    BUFFER_OFFSET+=ETH_ALEN;
    memcpy((void*)(TX_BUFFER+BUFFER_OFFSET), (void*)SOURCE_MAC, ETH_ALEN);
    BUFFER_OFFSET+=ETH_ALEN;

    // Check to see if QinQ VLAN ID has been supplied
    if(QINQ_PCP!=QINQ_PCP_DEF || QINQ_ID!=QINQ_ID_DEF)
    {

        // Add on the QinQ Tag Protocol Identifier
        // 0x88a8 == IEEE802.1ad, 0x9100 == older IEEE802.1QinQ
        TPI = htons(0x88a8);
        memcpy((void*)(TX_BUFFER+BUFFER_OFFSET), p, sizeof(TPI));
        BUFFER_OFFSET+=sizeof(TPI);

        // Now build the QinQ Tag Control Identifier:
        VLAN_ID_TMP = QINQ_ID;
        TCI = (QINQ_PCP & 0x07) << 5;
        QINQ_ID = QINQ_ID >> 8;
        TCI = TCI | (QINQ_ID & 0x0f);
        QINQ_ID = VLAN_ID_TMP;
        QINQ_ID = QINQ_ID << 8;
        TCI = TCI | (QINQ_ID & 0xffff);

        memcpy((void*)(TX_BUFFER+BUFFER_OFFSET), c, sizeof(TCI));
        BUFFER_OFFSET+=sizeof(TCI);

        // If an outer VLAN ID has been set, but not an inner one
        // (which would be a mistake) set it to 1 so the frame is still valid
        if(VLAN_ID==0) VLAN_ID=1;

    }

    // Check to see if a PCP value or VLAN ID has been supplier
    // for the main etherate frame header
    if(PCP!=PCP_DEF || VLAN_ID!=VLAN_ID_DEF)
    {

        TPI = htons(0x8100);
        memcpy((void*)(TX_BUFFER+BUFFER_OFFSET), p, sizeof(TPI));
        BUFFER_OFFSET+=sizeof(TPI);

        VLAN_ID_TMP = VLAN_ID;
        TCI = (PCP & 0x07) << 5;
        VLAN_ID = VLAN_ID >> 8;
        TCI = TCI | (VLAN_ID & 0x0f);
        VLAN_ID = VLAN_ID_TMP;
        VLAN_ID = VLAN_ID << 8;
        TCI = TCI | (VLAN_ID & 0xffff);

        memcpy((void*)(TX_BUFFER+BUFFER_OFFSET), c, sizeof(TCI));
        BUFFER_OFFSET+=sizeof(TCI);

    }

      // Push on the ETHERTYPE for the Etherate payload
      TPI = htons(ETHERTYPE);
      memcpy((void*)(TX_BUFFER+BUFFER_OFFSET), p, sizeof(TPI));
      BUFFER_OFFSET+=sizeof(TPI);

      ETH_HEADERS_LEN = BUFFER_OFFSET;

}


int get_interface_mtu_by_name(int &SOCKET_FD, char &IF_NAME) {

    const int NO_INT = 0;

    struct ifreq ifr;

    const char * IF_NAME_P = &IF_NAME;

    strcpy(ifr.ifr_name,IF_NAME_P);

    if(ioctl(SOCKET_FD, SIOCGIFMTU, &ifr)==0)
    {
        return ifr.ifr_mtu;
    }

    return NO_INT;

}


int get_sock_interface(int &SOCKET_FD) {
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

    if (ioctl(SOCKET_FD, SIOCGIFCONF, &ifc) < 0)
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
            if(strncmp(ifr->ifr_name, "eth", 3)==0 ||
               strncmp(ifr->ifr_name, "en", 2)==0 ||
               strncmp(ifr->ifr_name, "em2", 3)==0 ||
               strncmp(ifr->ifr_name, "wlan", 4)==0)
               //strncmp(ifr->ifr_name, "lo", 2)==0)
            {

                strncpy(ifreq.ifr_name,ifr->ifr_name,sizeof(ifreq.ifr_name));

                // Does this device even have hardware address?
                if (ioctl (SOCKET_FD, SIOCGIFHWADDR, &ifreq) != 0)
                {
                    printf("Error: Device has no hardware address: \n"
                           "SIOCGIFHWADDR(%s): %m\n", ifreq.ifr_name);
                    return NO_INT;
                }

                // Get the interface index
                ioctl(SOCKET_FD, SIOCGIFINDEX, &ifreq);

                // Get the interface name
                strncpy(IF_NAME,ifreq.ifr_name,IFNAMSIZ);

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


void list_interfaces() {

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


void print_examples () {

}


void print_usage () {

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
            "[Other]\n"
            "\t-x\tDisplay examples.\n"
            "\t\t#NOT IMPLEMENTED YET#\n"
            "\t-V|--version Display version\n"
            "\t-h|--help Display this help text\n",
            F_BYTES_DEF, F_COUNT_DEF, F_SIZE_DEF, (F_SIZE_DEF+ETH_HEADERS_LEN),
            F_DURATION_DEF);

}


int set_sock_interface_index(int &SOCKET_FD, int &IF_INDEX) {

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
            if (ioctl (SOCKET_FD, SIOCGIFHWADDR, &ifreq)==0)
            {

                // Get the interface index
                ioctl(SOCKET_FD, SIOCGIFINDEX, &ifreq);

                // Check if this is the interface index the user specified
                if (ifreq.ifr_ifindex==IF_INDEX)
                {

                    // Get the interface name
                    strncpy(IF_NAME,ifreq.ifr_name,IFNAMSIZ);

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


int set_sock_interface_name(int &SOCKET_FD, char &IF_NAME) {

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
            if (ioctl (SOCKET_FD, SIOCGIFHWADDR, &ifreq)==0)
            {

                // Check if this is the interface name the user specified
                if (strcmp(ifreq.ifr_name,(char *)&IF_NAME)==0)
                {

                    // Get the interface index
                    ioctl(SOCKET_FD, SIOCGIFINDEX, &ifreq);

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


void signal_handler(int signal) {
// The purpose of this signal handling function is to catch
// a user killing the test host before the test has ended, telling the
// other host we are doing down, to give it a chance to reset.

    std::string param;
    std::stringstream ss;
    ss << "etheratedeath";
    param = ss.str();
    strncpy(TX_DATA,param.c_str(), param.length());
    TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, ETH_HEADERS_LEN+param.length(), 0, 
                 (struct sockaddr*)&socket_address, sizeof(socket_address));


    cout << "Leaving promiscuous mode" << endl;

    strncpy(ethreq.ifr_name,IF_NAME,IFNAMSIZ);

    if (ioctl(SOCKET_FD,SIOCGIFFLAGS,&ethreq)==-1) {
        cout << "Error getting socket flags, entering promiscuous mode failed."
             << endl;
        perror("ioctl() ");
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(SOCKET_FD,SIOCSIFFLAGS,&ethreq)==-1) {
        cout << "Error setting socket flags, promiscuous mode failed." << endl;
        perror("ioctl() ");
    }

    close(SOCKET_FD);

    exit(130);

}


void string_explode(string str, string separator, vector<string>* results) {
// This function was gleaned from;
// http://www.infernodevelopment.com/perfect-c-string-explode-split
// Thanks to the author Baran Ornarli

    int found;
    results->clear();
    found = str.find_first_of(separator);

    while(found != string::npos)
    {
        if(found > 0) results->push_back(str.substr(0,found)); 
        str = str.substr(found+1);
        found = str.find_first_of(separator);
    }

    if(str.length() > 0) results->push_back(str);

}
