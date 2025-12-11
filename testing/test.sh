#!/bin/bash
if [ -f /etc/os-release ]; then
  grep PRETTY_NAME /etc/os-release
fi
hydrasdr_info
timeout 15 SoapySDRUtil --args=driver=hydrasdr --rate=10e6 --direction=RX
exit 0
