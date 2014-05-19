/*
 * Etherate Functions,
 *
 * 
 * File Contents:
 *
 * void signal_handler(int signal)
 *
 * void PrintUsage ()
 *
 * void StringExplode(string str, string separator, vector<string>* results)
 *
 * int GetSockInterface(int &sockFD)
 *
 * void ListInterfaces()
 *
 * void BuildHeaders(char* &txBuffer, unsigned char (&destMAC)[6], 
     unsigned char (&sourceMAC)[6], short &PCP, short &vlanID,
     short &qinqID, short &qinqPCP, int &headersLength)
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
    cout << "Error getting socket flags, entering promiscuous mode failed." << endl;
    perror("ioctl() ");
}

ethreq.ifr_flags &= ~IFF_PROMISC;

if (ioctl(sockFD,SIOCSIFFLAGS,&ethreq)==-1) {
    cout << "Error setting socket flags, promiscuous mode failed." << endl;
    perror("ioctl() ");
}

close(sockFD);

exit(0);

}


void PrintUsage () {

cout << "Usage info; [Mode] [Destination] [Source] [Options] [Other]" << endl
     << "[Mode] By default run in transmit mode, not receive" << endl
     << "\t-r\tChange to receive (listening) mode." << endl
     << "[Destination] Destination MAC address" << endl
     << "\t-d\tWithout this we default to 00:00:5E:00:00:02" << endl
     << "\t\tas the TX host and :01 as the RX host." << endl
     << "\t\tSpecify a custom desctination MAC address, -d 11:22:33:44:55:66" << endl
     << "[Source] Source interface and MAC address" << endl
     << "\t-s\tWithout this we default to 00:00:5E:00:00:01" << endl
     << "\t\tas the TX host and :02 as the RX host." << endl
     << "\t\tSpecify a custom source MAC address, -s 11:22:33:44:55:66" << endl
     << "\t-i\tInterface index for outgoing interface, \"2\" is eth0 in" << endl
     << "\t\tmost cases. If no -i option is specified we will try and" << endl
     << "\t\tbest guess an interface." << endl
     << "\t-l\tList interface indexes (then quit) for use with -i option." << endl
     << "[Options] These are all optional settings for the test parameters" << endl
     << "\t-f\tFrame payload size in bytes, default is " << fSizeDef << endl
     << "\t\t(" << (fSizeDef+headersLength) << " bytes is the expected size on the wire with headers)." << endl
     << "\t-t\tTransmition duration, integer in seconds, default is "
     << fDurationDef << "." << endl
     << "\t-c\tNumber of frames to send, default is " << fCountDef << ", default behaviour" << endl
     << "\t\tis to wait for duration." << endl
     << "\t-b\tNumber of bytes to send, default is " << fBytesDef << ", default behaviour" << endl
     << "\t\tis to wait for duration." << endl
     << "\t\tOnly one of -t, -c or -b can be used, both override -t," << endl
     << "\t\t-b overrides -c." << endl
     << "\t-a\tAck mode, have the receiver ack each frame during the test" << endl
     << "\t\t(This will significantly reduce the speed of the test)." << endl
     << "\t-m\tLimit to max bytes per/second, -m 125000 (1Mbps)." << endl
     << "[Other] Misc options and additional test parameters" << endl
     << "\t-v\tAdd an 802.1q VLAN tag. By default none is in the header." << endl
     << "\t\tIf using a PCP value with -p a default VLAN of 0 is added." << endl
     << "\t\t#NOT IMPLEMENTED YET#" << endl
     << "\t-p\tAdd an 802.1p PCP value from 1 to 7 using options -p 1 to" << endl
     << "\t\t-p 7. If more than one value is given, the highest is used." << endl
     << "\t\tDefault is 0 if none specified." << endl
     << "\t\t(If no 802.1q tag is given, the VLAN number infront will be 0)." << endl
     << "\t\t#NOT IMPLEMENTED YET#" << endl
     << "\t-q\tAdd an outter Q-in-Q tag. If used without -v, 1 is used" << endl
     << "\t\tfor the inner VLAN ID." << endl
     << "\t\t#NOT IMPLEMENTED YET#" << endl
     << "\t-o\tAdd an 802.1p PCP value to the outer Q-in-Q VLAN tag." << endl
     << "\t\tIf no PCP value is specified and a Q-in-Q VLAN ID is," << endl
     << "\t\t0 will be use. If no outer Q-in-Q VLAN ID is supplied this" << endl
     << "\t\toption is ignored. Use -o 1 to -o 7 like the -p option above." << endl
     << "\t\t#NOT IMPLEMENTED YET#" << endl
     << "\t-e\tSet a custom ethertype value, the default is 0x0800 (IPv4)." << endl
     << "\t\t#NOT IMPLEMENTED YET#" << endl
     << "\t-x\tDisplay examples." << endl
     << "\t\t#NOT IMPLEMENTED YET#" << endl
     << "\t-V|--version Display version" << endl
     << "\t-h|--help Display this help text" << endl;

}


void StringExplode(string str, string separator, vector<string>* results) {
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


int GetSockInterface(int &sockFD) {
// This function was gleaned from;
// http://cboard.cprogramming.com/linux-programming/43261-ioctl-request-get-hw-address.html
// As far as I can tell its a variation on some open source code originally written for
// Diald, which is here: http://diald.sourceforge.net/FAQ/diald-faq.html
// So thanks go to the Diald authors Eric Schenk and Gordon Soukoreff!

struct ifreq *ifr, *ifend;
struct ifreq ifreq;
struct ifconf ifc;
struct ifreq ifs[MAX_IFS];

ifc.ifc_len = sizeof(ifs);
ifc.ifc_req = ifs;
if (ioctl(sockFD, SIOCGIFCONF, &ifc) < 0)
{
    printf("Error: No compatible interfaces found: ioctl(SIOCGIFCONF): %m\n");
    return 0;
}

ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
{

    // Is this a packet capable device? (Doesn't work with AF_PACKET?)
    if (ifr->ifr_addr.sa_family == AF_INET)
    {

        // Try to get a typical ethernet adapter, not a loopback or virt interface
        if(strncmp(ifr->ifr_name, "eth", 3)==0 ||
          strncmp(ifr->ifr_name, "en", 2)==0 ||
          strncmp(ifr->ifr_name, "em", 2)==0 ||
          strncmp(ifr->ifr_name, "wlan", 4)==0)
        {

            strncpy(ifreq.ifr_name, ifr->ifr_name,sizeof(ifreq.ifr_name));

            // Does this device even have hardware address?
            if (ioctl (sockFD, SIOCGIFHWADDR, &ifreq) != 0)
            {
                printf("Error: Device has no Hardware address: SIOCGIFHWADDR(%s): %m\n",
                       ifreq.ifr_name);
                return 0;
            }

            printf("Using device %s, Hardware Address %02x:%02x:%02x:%02x:%02x:%02x, "
                   "Interface Index ",
                   ifreq.ifr_name,
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[0],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[1],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[2],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[3],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[4],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[5],
                   ifreq.ifr_ifindex);
            ioctl(sockFD, SIOCGIFINDEX, &ifreq);

            // Set the interface name
            strncpy(ifName,ifreq.ifr_name,IFNAMSIZ);
            printf("%u\n", ifreq.ifr_ifindex);
            return ifreq.ifr_ifindex;

        } // If eth* || en* || em* || wlan*

    } // If AF_INET

} // Looping through interface count

return 0;

}


void ListInterfaces() {

struct ifreq *ifr, *ifend;
struct ifreq ifreq;
struct ifconf ifc;
struct ifreq ifs[MAX_IFS];
int SockFD;
SockFD = socket(AF_INET, SOCK_DGRAM, 0);

ifc.ifc_len = sizeof(ifs);
ifc.ifc_req = ifs;

if (ioctl(SockFD, SIOCGIFCONF, &ifc) < 0)
{
    printf("Error: No AF_INET (IPv4) compatible interfaces found: ioctl(SIOCGIFCONF): "
           "%m\n");
    return;
}

ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
{

    if (ifr->ifr_addr.sa_family == AF_INET)
    {
        strncpy(ifreq.ifr_name, ifr->ifr_name,sizeof(ifreq.ifr_name));
        // Does this device hav a hardware address?

        if (ioctl (SockFD, SIOCGIFHWADDR, &ifreq) == 0)
        {
            // Print the current interface address
            printf("Device %s, Hardware Address %02x:%02x:%02x:%02x:%02x:%02x, "
                   "Interface Index ",
                   ifreq.ifr_name,
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[0],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[1],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[2],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[3],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[4],
                   (int) ((unsigned char *) &ifreq.ifr_hwaddr.sa_data)[5]);
            // And the interface index
            ioctl(SockFD, SIOCGIFINDEX, &ifreq);
            printf("%u\n", ifreq.ifr_ifindex);
        }else{ // Interface has no hardware address
            return; 
        }

    }

}

}


void BuildHeaders(char* &txBuffer, unsigned char (&destMAC)[6], 
     unsigned char (&sourceMAC)[6], short &PCP, short &vlanID,
     short &qinqID, short &qinqPCP, int &headersLength) {

int offset = 0;

short TPI = 0;
short TCI = 0;
short *p = &TPI;
short *c = &TCI;

short vlanIDtemp;

// Copy the destination and source MAC addresses
memcpy((void*)txBuffer, (void*)destMAC, ETH_ALEN);
memcpy((void*)(txBuffer+ETH_ALEN), (void*)sourceMAC, ETH_ALEN);
offset = (ETH_ALEN*2);

//Check to see if QinQ VLAN ID has been supplied
if(qinqID!=qinqIDDef)
{

    // Add on the QinQ Tag Protocol Identifier
    TPI = htons(0x88a8); //0x88a8 == IEEE802.1ad, 0x9100 == older IEEE802.1QinQ
    memcpy((void*)(txBuffer+offset), p, 2);
    offset+=2;

    // Now build the QinQ Tag Control Identifier:
    vlanIDtemp = qinqID;
    TCI = (qinqPCP & 0x07) << 5;
    qinqID = qinqID >> 8;
    TCI = TCI | (qinqID & 0x0f);
    qinqID = vlanIDtemp;
    qinqID = qinqID << 8;
    TCI = TCI | (qinqID & 0xffff);

    memcpy((void*)(txBuffer+offset), c, 2);
    offset+=2;

    qinqID = vlanIDtemp;    ///////////// Do we need this?

    // If an outer VLAN ID has been set, but not an inner one (which would be a mistake)
    // set it to 1 so the frame is still valid
    if(vlanID==0) vlanID=1;

}

// Check to see if a PCP value or VLAN ID has been supplier
// for the main etherate frame header
if(PCP!=PCPDef || vlanID!=vlanIDDef)
{

    TPI = htons(0x8100);
    memcpy((void*)(txBuffer+offset), p, 2);
    offset+=2;

    vlanIDtemp = vlanID;
    TCI = (PCP & 0x07) << 5;
    vlanID = vlanID >> 8;
    TCI = TCI | (vlanID & 0x0f);
    vlanID = vlanIDtemp;
    vlanID = vlanID << 8;
    TCI = TCI | (vlanID & 0xffff);

    memcpy((void*)(txBuffer+offset), c, 2);
    offset+=2;

    vlanID = vlanIDtemp;    ///////////// Do we need this?

}

  // Push on the Ethertype (IPv4) for the Etherate payload
  TPI = htons(0x0800);
  memcpy((void*)(txBuffer+offset), p, 2);
  offset+=2;

  headersLength = offset;

  cout << "headersLength: " << headersLength << endl;

}