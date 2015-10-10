Etherate
========

[![Build Status](https://travis-ci.org/jwbensley/Etherate.svg?branch=master)](https://travis-ci.org/jwbensley/Etherate)

What is it:

  The original idea was to provide a simple speed testing program
  that with any modern day CPU could saturate a gigabit Ethernet link,
  with any typical off the shelf/on-board GigE NIC. Programs such as
  iPerf/jPerf (there are many others!) exist and are mostly excellent 
  at this. They can saturate a link or connection to measure throughput
  or simulate congestion, but they operate at layer 4 of the OSI model
  using either TCP or UDP for data transport. This is fine for WAN
  testing or testing over a layer 3 boundary, across the Internet
  (home broadband or tail circuit diagnostics etc).

  Etherate is a Linux CLI program for testing layer 2 Ethernet circuits.
  The original design scope has been widened and improved upon to
  create a CLI testing program for layer 2 connectivity that performs
  a variety of tests and generates many more stats and metrics than speed.

  
Why was it Made:  

  Hardware testers can be used to test a layer 2 link end to end but
  these are typically very expensive and most techies are already
  carrying a laptop around. After the initial beta release the goal for
  Etherate is to start to bring in RFC2544 and ITU-T Y.1564 measurement
  compliance testing, OAM testing, MPLS and VPLS testing and stress testing.
  
  Free Ethernet testing for all!
  

Current Features:

  Now in the beta stages and out of alpha its time to start improving on the
  original code and add more functions. This are the current working features:
  
  - Any EtherType
  - Any Source MAC and/or Destination MAC
  - Any VLAN ID
  - Any priority (PCP) value
  - Supports double tagging (inner and outer VLAN ID and PCP)
  - Toggle DFI bit
  - Toggle frame acknowledgement
  - Optional speed limit in Mbps or Frames/p/s
  - Optional frame payload size
  - Optional transfer limit in MBs/GBs
  - Optional test duration
  - Perform MTU sweeping test

  
Development Plan

  These are features that will be in development after the beta 
  is released;
  
  - Add L2 storm testing (broadcast and multicast without no RX host)
  - Add feature to load frame payload from a text file
  - Add BPDU & keepalive generation shortcuts
  - Report throughput if additional headers (IPv4/6/TCP/UDP) were present
  - Simulate large MPLS label stacks (4/5/6 labels etc)
  
  
Technical details:

  In the alpha stage Etherate is single threaded and only runs a single
  test between a transmitting and receiving host. Later development
  will see it turn to bidirectional more complex testing.


Usage Examples:

  "./etherate -x" will display some example of the CLI syntex.

  The traffic is easily identified in Wireshark as Etherate uses MAC
  addresses from the IANA unassigned space. The TX host is 01 and
  the RX host is 02:

  eth.addr == 00:00:5e:00:00:01 || eth.addr == 00:00:5e:00:00:02

  or

  eth.addr contains 00:00:5e:00:00


More info:

  http://null.53bits.co.uk/index.php?page=etherate
  
 
Who made it:

  James Bensley <jwbensley at gmail dot com>
