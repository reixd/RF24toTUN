RF24toTUN
=========

Application to send IP packets with a TUN/TAP interface in a RF24Network infrastructure.

The rf24totun_configAndPing.sh script shows hot to configure and set up the TUN/TAP interface to be able to send and receive IP traffic.

## Main features

  * Create a persistent TUN/TAP device
  * Send and receive IP packets accross a RF24Network infrastructure
  * Node address configuration at startup
  
## Dependencies

 * RF24 library  
   * https://github.com/tmrh20/RF24.git
 * RF24Network   
   * If you want to be able to send large payloads (size > 24 Bytes) use the fragmentation_v2 branch in https://github.com/reixd/RF24Network
   * https://github.com/tmrh20/RF24Network.git
     * https://tmrh20.github.io/RF24Network
 * C++ Boost libs
 
# Install

    make
    sudo make install

# Running 

    sudo rf24totun
    
OR
    
    sudo ./rf24totun_configAndPing.sh 1 2   #On node1
    sudo ./rf24totun_configAndPing.sh 2 1   #On node2


# Licence

The MIT License (MIT)

Copyright (c) 2014 Rei <devel@reixd.net>

See the appended file for more information.
