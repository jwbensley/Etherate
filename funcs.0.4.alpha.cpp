/*
 * Etherate Functions,
 *
 * License: First rule of license club...
 *
 * Updates: https://github.com/jwbensley/Etherate and http://null.53bits.co.uk
 * Please send corrections, ideas and help to: jwbensley@gmail.com 
 * (I'm a beginner if that isn't obvious!)
 *
 * compile with: g++ -o etherate etherate.cpp -lrt
 * 
 * File Contents:
 *
 * void signal_handler(int signal)
 * void print_usage ()
 * void string_explode(string str, string separator, vector<string>* results)
 * int get_sock_interface(int &sockFD)
 * int set_sock_interface_index(int &sockFD, int &ifIndex)
 * void list_interfaces()
 * void build_headers(char* &txBuffer, unsigned char (&destMAC)[6], 
     unsigned char (&sourceMAC)[6], int &ethertype, short &PCP,
     short &vlanID, short &qinqID, short &qinqPCP, int &headersLength)
 *
 */


void signal_handler(int signal) {
// The purpose of this signal handling function is to catch
// a user killing the test host before the test has ended, telling the
// other host we are doing down, to give it a chance to reset.

    std::string param;
    std::stringstream ss;
    ss << "etheratedeath";
    param = ss.str();
    strncpy(txData,param.c_str(), param.length());
    sendResult = sendto(sockFD, txBuffer, headersLength+param.length(), 0, 
                 (struct sockaddr*)&socket_address, sizeof(socket_address));


    cout << "Leaving promiscuous mode" << endl;

    strncpy(ethreq.ifr_name,ifName,IFNAMSIZ);

    if (ioctl(sockFD,SIOCGIFFLAGS,&ethreq)==-1) {
        cout << "Error getting socket flags, entering promiscuous mode failed."
             << endl;
        perror("ioctl() ");
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(sockFD,SIOCSIFFLAGS,&ethreq)==-1) {
        cout << "Error setting socket flags, promiscuous mode failed." << endl;
        perror("ioctl() ");
    }

    close(sockFD);

    exit(130);

}


void print_usage () {

    printf ("Usage info; [Mode] [Destination] [Source] [Options] [Other]\n"
            "[Mode] By default run in transmit mode, not receive\n"
            "\t-r\tChange to receive (listening) mode.\n"
            "[Destination]\n"
            "\t-d\tWithout this we default to 00:00:5E:00:00:02\n"
            "\t\tas the TX host and :01 as the RX host.\n"
            "\t\tSpecify a custom desctination MAC address, \n"
            "\t\t-d 11:22:33:44:55:66\n"
            "[Source]\n"
            "\t\tSpecify a custom source MAC address, -s 11:22:33:44:55:66\n"
            "\t-i\tSet interface by name. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-I\tSet interface by index. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-l\tList interface indexes (then quit) for use with -i option.\n"
            "\t-s\tWithout this we default to 00:00:5E:00:00:01\n"
            "\t\tas the TX host and :02 as the RX host.\n"
            "[Options]\n"
            "\t-a\tAck mode, have the receiver ack each frame during the test\n"
            "\t\t(This will significantly reduce the speed of the test).\n"
            "\t-b\tNumber of bytes to send, default is %ld, default behaviour\n"
            "\t\tis to wait for duration.\n"
            "\t\tOnly one of -t, -c or -b can be used, both override -t,\n"
            "\t\t-b overrides -c.\n"
            "\t-c\tNumber of frames to send, default is %ld, default behaviour\n"
            "\t\tis to wait for duration.\n"
            "\t-e\tSet a custom ethertype value the default is 0x0800 (IPv4).\n"
            "\t-f\tFrame payload size in bytes, default is %d\n"
            "\t\t(%d bytes is the expected size on the wire with headers).\n"
            "\t-m\tMax bytes per/second to send, -m 125000 (1Mbps).\n"
            "\t-t\tTransmition duration, integer in seconds, default is %ld.\n"
            "[Other]\n"
            "\t-v\tAdd an 802.1q VLAN tag. By default none is in the header.\n"
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
            "\t-x\tDisplay examples.\n"
            "\t\t#NOT IMPLEMENTED YET#\n"
            "\t-V|--version Display version\n"
            "\t-h|--help Display this help text\n",
            fBytesDef, fCountDef, fSizeDef, (fSizeDef+headersLength),
            fDurationDef);

}

void print_examples () {

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


int get_sock_interface(int &sockFD) {
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

    if (ioctl(sockFD, SIOCGIFCONF, &ifc) < 0)
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
                if (ioctl (sockFD, SIOCGIFHWADDR, &ifreq) != 0)
                {
                    printf("Error: Device has no hardware address: \n"
                           "SIOCGIFHWADDR(%s): %m\n", ifreq.ifr_name);
                    return NO_INT;
                }

                // Get the interface index
                ioctl(sockFD, SIOCGIFINDEX, &ifreq);

                // Get the interface name
                strncpy(ifName,ifreq.ifr_name,IFNAMSIZ);

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


int set_sock_interface_index(int &sockFD, int &ifIndex) {

    const int NO_INT = 0;

    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    ioctl(sockFD, SIOCGIFCONF, &ifc);

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
    {

        if (ifr->ifr_addr.sa_family == AF_INET)
        {

            strncpy(ifreq.ifr_name,ifr->ifr_name,sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (sockFD, SIOCGIFHWADDR, &ifreq) == 0)
            {

                // Get the interface index
                ioctl(sockFD, SIOCGIFINDEX, &ifreq);

                if (ifreq.ifr_ifindex==ifIndex)
                {
                    // Get the interface name
                    strncpy(ifName,ifreq.ifr_name,IFNAMSIZ);

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

                    return ifIndex;
                }

            }

        }

    }

    return NO_INT;

}


int set_sock_interface_name(int &sockFD, char &ifName) {

    const int NO_INT = 0;

    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    ioctl(sockFD, SIOCGIFCONF, &ifc);

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
    {

        if (ifr->ifr_addr.sa_family == AF_INET)
        {

            strncpy(ifreq.ifr_name,ifr->ifr_name,sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (sockFD, SIOCGIFHWADDR, &ifreq) == 0)
            {


                if (strcmp(ifreq.ifr_name,(char *)&ifName)==0)
                {
                    // Get the interface name
                    //strncpy(ifName,ifreq.ifr_name,IFNAMSIZ);

                    // Get the interface index
                    ioctl(sockFD, SIOCGIFINDEX, &ifreq);

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


void list_interfaces() {

    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];
    int sockFD;
    sockFD = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl(sockFD, SIOCGIFCONF, &ifc) < 0)
    {
        printf("Error: No AF_INET (IPv4) compatible interfaces found: \n"
               "ioctl(SIOCGIFCONF): %m\n");
        return;
    }

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
    {

        if (ifr->ifr_addr.sa_family == AF_INET)
        {
            strncpy(ifreq.ifr_name,ifr->ifr_name,sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (sockFD, SIOCGIFHWADDR, &ifreq) == 0)
            {
                // Get the interface index
                ioctl(sockFD, SIOCGIFINDEX, &ifreq);

                // Print the current interface address
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

            }else{ // Interface has no hardware address
                return; 
            }

        }

    }

}


void build_headers(char* &txBuffer, unsigned char (&destMAC)[6], 
     unsigned char (&sourceMAC)[6], int &ethertype, short &PCP,
     short &vlanID, short &qinqID, short &qinqPCP, int &headersLength) {

    int offset = 0;
    short TPI = 0;
    short TCI = 0;
    short *p = &TPI;
    short *c = &TCI;
    short vlanIDtemp;

    // Copy the destination and source MAC addresses
    memcpy((void*)txBuffer, (void*)destMAC, ETH_ALEN);
    offset+=ETH_ALEN;
    memcpy((void*)(txBuffer+offset), (void*)sourceMAC, ETH_ALEN);
    offset+=ETH_ALEN;

    // Check to see if QinQ VLAN ID has been supplied
    if(qinqPCP!=qinqPCPDef || qinqID!=qinqIDDef)
    {

        // Add on the QinQ Tag Protocol Identifier
        // 0x88a8 == IEEE802.1ad, 0x9100 == older IEEE802.1QinQ
        TPI = htons(0x88a8);
        memcpy((void*)(txBuffer+offset), p, sizeof(TPI));
        offset+=sizeof(TPI);

        // Now build the QinQ Tag Control Identifier:
        vlanIDtemp = qinqID;
        TCI = (qinqPCP & 0x07) << 5;
        qinqID = qinqID >> 8;
        TCI = TCI | (qinqID & 0x0f);
        qinqID = vlanIDtemp;
        qinqID = qinqID << 8;
        TCI = TCI | (qinqID & 0xffff);

        memcpy((void*)(txBuffer+offset), c, sizeof(TCI));
        offset+=sizeof(TCI);

        // If an outer VLAN ID has been set, but not an inner one
        // (which would be a mistake) set it to 1 so the frame is still valid
        if(vlanID==0) vlanID=1;

    }

    // Check to see if a PCP value or VLAN ID has been supplier
    // for the main etherate frame header
    if(PCP!=PCPDef || vlanID!=vlanIDDef)
    {

        TPI = htons(0x8100);
        memcpy((void*)(txBuffer+offset), p, sizeof(TPI));
        offset+=sizeof(TPI);

        vlanIDtemp = vlanID;
        TCI = (PCP & 0x07) << 5;
        vlanID = vlanID >> 8;
        TCI = TCI | (vlanID & 0x0f);
        vlanID = vlanIDtemp;
        vlanID = vlanID << 8;
        TCI = TCI | (vlanID & 0xffff);

        memcpy((void*)(txBuffer+offset), c, sizeof(TCI));
        offset+=sizeof(TCI);

    }

      // Push on the ethertype for the Etherate payload
      TPI = htons(ethertype);
      memcpy((void*)(txBuffer+offset), p, sizeof(TPI));
      offset+=sizeof(TPI);

      headersLength = offset;

}
