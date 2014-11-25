#!/bin/bash

#
# The MIT License (MIT)
#
# Copyright (c) 2014 Rei <devel@reixd.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

USAGE="Usage:
Set the IP for this node and ping the other node.
  $0 \$nodeID \$nodeID

This is node1 and it will ping node2 three times
  $0 1 2
"

if [[ -z "${1##*[!0-9]*}" ]] || [[ -z "${2##*[!0-9]*}" ]]; then
        echo -e "$USAGE"
fi

INTERFACE="tun_nrf24"
#MYIP="10.10.2.${1}/16"
#PINGIP="10.10.2.${2}"
MYIP="192.168.1.${1}/24"
PINGIP="192.168.1.${2}"
MTU=1500

function setIP() {
	sleep 4s && \
	if [ -d /proc/sys/net/ipv4/conf/${INTERFACE} ]; then
		ip link set ${INTERFACE} up  > /dev/null 2>&1
		ip addr add ${MYIP} dev ${INTERFACE}  > /dev/null 2>&1
		ip link set dev ${INTERFACE} mtu ${MTU} > /dev/null 2>&1
		ip addr show dev ${INTERFACE} > /dev/null 2>&1
		return 0
	else
		return 1
	fi
}

function pingOther() {
	sleep 6s && \
	ping -c3 -I ${INTERFACE} ${PINGIP}
}

setIP && pingOther &
/usr/local/bin/rf24totun
