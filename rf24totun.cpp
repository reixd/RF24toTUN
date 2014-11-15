/*
 * The MIT License (MIT)
 * Copyright (c) 2014 Rei <devel@reixd.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "rf24totun.h"

/**
* Configuration and setup of the NRF24 radio
*
* @return True if the radio was configured successfully
*/
bool configureAndSetUpRadio() {

    radio.begin();
    delay(5);
    const uint16_t this_node = thisNodeAddr;
    network.begin(/*channel*/ channel, /*node address*/ this_node);

    if (PRINT_DEBUG >= 1) {
        radio.printDetails();
    }

    return true;
}

/**
* Configure, set up and allocate the TUN/TAP device.
*
* If the TUN/TAP device was allocated successfully the file descriptor is returned.
* Otherwise the application closes with an error.
* @note Currently only a TUN device with multi queue is used.
*
* @return The TUN/TAP device file descriptor
*/
int configureAndSetUpTunDevice() {

    std::string tunTapDevice = "tun_nrf24";
    strcpy(tunName, tunTapDevice.c_str());

    int flags = IFF_TUN | IFF_NO_PI | IFF_MULTI_QUEUE;
    tunFd = allocateTunDevice(tunName, flags);
    if (tunFd >= 0) {
        std::cout << "Successfully attached to tun/tap device " << tunTapDevice << std::endl;
    } else {
        std::cerr << "Error allocating tun/tap interface: " << tunFd << std::endl;
        exit(1);
    }

    return tunFd;
}

/**
* Allocate the TUN/TAP device (if needed)
*
* The TUN/TAP interface code is based code from
* http://backreference.org/2010/03/26/tuntap-interface-tutorial/
*
* @param *dev the name of an interface (or '\0'). MUST have enough space to hold the interface name if '\0' is passed
* @param flags interface flags (eg, IFF_TUN etc.)
* @return The file descriptor for the allocated interface or -1 is an error ocurred
*/
int allocateTunDevice(char *dev, int flags) {
    struct ifreq ifr;
    int fd;

    /* open the clone device */
    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
       return fd;
    }

    // preparation of the struct ifr, of type "struct ifreq"
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;   // IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI

    if (*dev) {
        // if a device name was specified, put it in the structure; otherwise,
        // the kernel will try to allocate the "next" device of the
        // specified type
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // try to create the device
    if(ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        //close(fd);
        std::cerr << "Error: enabling TUNSETIFF" << std::endl;
        return .1;
    }

    //Make interface persistent
    if(ioctl(fd, TUNSETPERSIST, 1) < 0){
        std::cerr << "Error: enabling TUNSETPERSIST" << std::endl;
        return -1;
    }

    // if the operation was successful, write back the name of the
    // interface to the variable "dev", so the caller can know
    // it. Note that the caller MUST reserve space in *dev (see calling
    // code below)
    strcpy(dev, ifr.ifr_name);

    // this is the special file descriptor that the caller will use to talk
    // with the virtual interface
    return fd;
}

/**
* The thread function in charge receiving and transmitting messages with the radio.
* The received messages from RF24Network and NRF24L01 device and enqueued in the rxQueue and forwaded to the TUN/TAP device.
* The messages from the TUN/TAP device (in the txQueue) are sent to the RF24Network lib and transmited over the air.
*
* @note Optimization: Use two thread for rx and tx with the radio, but thread synchronisation and semaphores are needed.
*       It may increase the throughput.
*/
void radioRxTxThreadFunction() {

    while(1) {
    try {

        network.update();

         //RX section
         
        while ( network.available() ) { // Is there anything ready for us?

            RF24NetworkHeader header;        // If so, grab it and print it out
            Message msg;
            char buffer[MAX_PAYLOAD_SIZE];

            unsigned int bytesRead = network.read(header,buffer,MAX_PAYLOAD_SIZE);
            if (bytesRead > 0) {
                msg.setPayload(buffer,bytesRead);
                if (PRINT_DEBUG >= 1) {
                    std::cout << "Radio: Received "<< bytesRead << " bytes ... " << std::endl;
                }
                if (PRINT_DEBUG >= 3) {
                    printPayload(msg.getPayloadStr(),"radio RX");
                }
                radioRxQueue.push(msg);
            } else {
                std::cerr << "Radio: Error reading data from radio. Read '" << bytesRead << "' Bytes." << std::endl;
            }
        } //End RX

        network.update();


         // TX section
        
        if(!radioTxQueue.empty() && !radio.available() ) {
            Message msg = radioTxQueue.pop();

            if (PRINT_DEBUG >= 1) {
                std::cout << "Radio: Sending "<< msg.getLength() << " bytes ... ";
            }
            if (PRINT_DEBUG >= 3) {
                std::cout << std::endl; //PrintDebug == 1 does not have an endline.
                printPayload(msg.getPayloadStr(),"radio TX");
            }
            const uint16_t other_node = otherNodeAddr;
            RF24NetworkHeader header(/*to node*/ other_node);
            bool ok = network.write(header,msg.getPayload(),msg.getLength());

            if (ok) {
                //std::cout << "ok." << std::endl;
            } else {
                std::cerr << "failed." << std::endl;
            }
        } //End Tx

		delay(1);

    } catch(boost::thread_interrupted&) {
        std::cerr << "radioRxThreadFunction is stopped" << std::endl;
        return;
    }
    }
}

/**
* Thread function in charge of reading, framing and enqueuing the packets from the TUN/TAP interface in the RxQueue.
*
* This thread uses "select()" with timeout to avoid a busy waiting
*/
void tunRxThreadFunction() {

    fd_set socketSet;
    struct timeval selectTimeout;
    char buffer[MAX_TUN_BUF_SIZE];
    int nread;

    while(1) {
    try {

        // reset socket set and add tap descriptor
        FD_ZERO(&socketSet);
        FD_SET(tunFd, &socketSet);

        // initialize timeout
        selectTimeout.tv_sec = 1;
        selectTimeout.tv_usec = 0;

        // suspend thread until we receive a packet or timeout
        if (select(tunFd + 1, &socketSet, NULL, NULL, &selectTimeout) != 0) {
            if (FD_ISSET(tunFd, &socketSet)) {
                if ((nread = read(tunFd, buffer, MAX_TUN_BUF_SIZE)) >= 0) {

                    if (PRINT_DEBUG >= 1) {
                        std::cout << "Tun: Successfully read " << nread  << " bytes from tun device" << std::endl;
                    }
                    if (PRINT_DEBUG >= 3) {
                        printPayload(std::string(buffer, nread),"Tun read");
                    }

                    // copy received data into new Message
                    Message msg;
                    msg.setPayload(buffer,nread);

                    // send downwards
                    radioTxQueue.push(msg);

                } else
                    std::cerr << "Tun: Error while reading from tun/tap interface." << std::endl;
            }
        }

    } catch(boost::thread_interrupted&) {
        std::cerr << "tunRxThreadFunction is stopped" << std::endl;
        return;
    }
    }
}

/**
* This thread function waits for incoming messages from the radio and forwards them to the TUN/TAP interface.
*
* This threads blocks until a message is received avoiding busy waiting.
*/
void tunTxThreadFunction() {
    while(1) {
    try {
        //Wait for Message from radio
        Message msg = radioRxQueue.pop();

        assert(msg.getLength() <= MAX_TUN_BUF_SIZE);

        if (msg.getLength() > 0) {

            size_t writtenBytes = write(tunFd, msg.getPayload(), msg.getLength());
			if(!writtenBytes){  writtenBytes = write(tunFd, msg.getPayload(), msg.getLength()); }
            if (writtenBytes != msg.getLength()) {
                std::cerr << "Tun: Less bytes written to tun/tap device then requested." << std::endl;
            } else {
                if (PRINT_DEBUG >= 1) {
                    std::cout << "Tun: Successfully wrote " << writtenBytes  << " bytes to tun device" << std::endl;
                }
            }

            if (PRINT_DEBUG >= 3) {
                printPayload(msg.getPayloadStr(),"tun write");
            }

        }
    } catch(boost::thread_interrupted&) {
        std::cerr << "tunTxThreadFunction is stopped" << std::endl;
        return;
    }
    }
}

/**
* Debug output of the given message buffer preceeded by the debugMsg.
*
* @param buffer The std::string with the message to debug.
* @param debugMsg The info message preceeding the buffer.
*/
void printPayload(std::string buffer, std::string debugMsg = "") {

    std::cout << "********************************************************************************" << std::endl
    << debugMsg << std::endl
    << "Buffer size: " << buffer.size() << " bytes" << std::endl
    << std::hex << buffer << std::endl
    << "********************************************************************************" << std::endl;
}

/**
* Debug output of the given message buffer preceeded by the debugMsg.
*
* @param buffer The char array with the message to debug.
* @param nread The length of the message buffer.
* @param debugMsg The info message preceeding the buffer.
*/
void printPayload(char *buffer, int nread, std::string debugMsg = "") {

    std::cout << "********************************************************************************" << std::endl
    << debugMsg << std::endl
    << "Buffer size: " << nread << " bytes" << std::endl
    << std::hex << std::string(buffer,nread) << std::endl
    << std::cout << "********************************************************************************" << std::endl;
}

/**
* This procedure is called before terminating the programm to properly close and terminate all the threads and file handlers.
*
*/
void on_exit() {
    std::cout << "Cleaning up and exiting" << std::endl;

    if (tunRxThread) {
        tunRxThread->interrupt();
        tunRxThread->join();
    }

    if (tunTxThread) {
        tunTxThread->interrupt();
        tunTxThread->join();
    }

    if (radioRxTxThread) {
        radioRxTxThread->interrupt();
        radioRxTxThread->join();
    }

    if (tunFd >= 0)
        close(tunFd);
}

/**
* Projecure to wait and join all the threads.
*
* @note If an interrupt was not before called, this procedure may block for ever.
*/
void joinThreads() {
    if (tunRxThread) {
        tunRxThread->join();
    }

    if (tunTxThread) {
        tunTxThread->join();
    }

    if (radioRxTxThread) {
        radioRxTxThread->join();
    }
}

/**
* Main
*
* @TODO better address management
*
* @param argc
* @param **argv
* @return Exit code
*/
int main(int argc, char **argv) {

    std::atexit(on_exit);

    std::cout << "\n ************ Address Setup ***********\n";
    std::string input = "";
    char myChar = {0};
    std::cout << "Choose an address: Enter 0 for master, 1 for child, 2 for Master with 1 Arduino routing node (02), 3 for Child with Arduino routing node  (CTRL+C to exit) \n>";
    std::getline(std::cin,input);

    if(input.length() > 0) {
        myChar = input[0];
        if(myChar == '0'){
            thisNodeAddr = 00;
            otherNodeAddr = 1;
        }else if(myChar == '1') {
            thisNodeAddr = 1;
            otherNodeAddr = 00;
		}else if(myChar == '2') {
            thisNodeAddr = 00;
            otherNodeAddr = 012;
		}else if(myChar == '3') {
            thisNodeAddr = 012;
            otherNodeAddr = 00;
        } else {
            std::cout << "Wrong address! Choose 0 or 1." << std::endl;
            exit(1);
        }

        std::cout << "ThisNodeAddress: " << thisNodeAddr << std::endl;
        std::cout << "OtherNodeAddress: " << otherNodeAddr << std::endl;
        std::cout << "\n **********************************\n";
    } else {
            std::cout << "Wrong address! Choose 0 or 1." << std::endl;
            exit(1);
    }


    configureAndSetUpTunDevice();
    configureAndSetUpRadio();

    //start threads
    tunRxThread.reset(new boost::thread(tunRxThreadFunction));
    tunTxThread.reset(new boost::thread(tunTxThreadFunction));
    radioRxTxThread.reset(new boost::thread(radioRxTxThreadFunction));

    joinThreads();

    return 0;
}
