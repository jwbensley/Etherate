Etherate
========

[![Build Status](https://travis-ci.org/jwbensley/Etherate.svg?branch=master)](https://travis-ci.org/jwbensley/Etherate)
[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/jwbensley/etherate/trend.png)](https://bitdeli.com/free "Bitdeli Badge")
[![PayPal Donate](https://img.shields.io/badge/paypal-donate-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=james%40bensley%2eme&lc=GB&item_name=Etherate&currency_code=GBP)


What is it:

  Etherate is a Linux CLI program for testing layer 2 Ethernet connections.

  Programs such as iPerf/jPerf/Ostinato/PathEth/Scapy (to name just a few) 
  are mostly excellent. They can saturate a link to measure throughput or 
  simulate congestion, but they usually operate at layer 3 or 4 of the OSI
  model using either TCP or UDP for data transport, using sockets defined
  by the OS.

  This is fine for testing over a layer 3 boundary such as across the
  Internet, home broadband or tail circuit diagnostics etc. Etherate uses
  raw sockets operating directly over layer 2 Ethernet to provide low level
  network analysis for Ethernet and MPLS connections.


Why was it Made:  

  The main goal is a free Ethernet and MPLS testing program that allows for
  advance network analysis and with any modern day CPU and off the self NIC
  can saturate a gigabit Ethernet of 10GigE link.

  Physical hardware Ethernet testers can be used to test a layer 2 link end
  to end but these are typically very expensive and most techies are already
  carrying a laptop around. In the case of network service providers, many
  have servers dotted around with 10GigE NICs that engineers and customers
  can test back to.


Current Features:

  Having moved from alpha to beta the current focus is to increase the
  feature set of Etherate and improve its efficiency.

  Currently working features which all operate directly over layer 2:
  
  - Any EtherType
  - Any Source MAC and/or Destination MAC
  - Any VLAN ID
  - Any priority (PCP) value
  - Supports double tagging (any inner and outer VLAN ID, any PCP)
  - Toggle DFI bit
  - Toggle frame acknowledgement
  - Optional speed limit in Mbps, MBps or Frames/p/s
  - Optional frame payload size
  - Optional transfer limit in MBs/GBs
  - Optional test duration
  - L2 storm (BUM) testing
  - One way delay measurement
  - Perform MTU sweeping test
  - Perform link quality tests (RTT & jitter)

  
Development Plan

  These are features currently being worked upon:
  
  - Add feature to load frame payload from a text file
  - Add BPDU & keepalive generation shortcuts
  - Report throughput if additional headers (IPv4/6/TCP/UDP) were present
  - Simulate MPLS label stacks


Technical details:

  Etherate is a single threaded application, despite this 10G throughput
  can be achieved one a 2.4Ghz Intel CPU with 10G Intel NIC. Etherate is
  not currently using NetMap or the similar framework released by Google
  as it is still in beta performance tuning is still ongoing. This may
  become a future development (if the desired performance levels can't
  be reached) however this would introduce an additional dependency.


More info, usage examples, FAQs etc:

  http://null.53bits.co.uk/index.php?page=etherate


Who made it (where to send your hate mail):

  James Bensley <jwbensley at gmail dot com>
