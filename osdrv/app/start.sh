#!/bin/sh
echo "start.sh"
ulimit -s 10240
/app/wifi/wifi_loadko.sh
telnetd&
/app/cdr_rtc_app &
/app/cdr_daemon &
sleep 10
/app/wifi/wifi_cfg.sh ap
sleep 2
cp /app/goahead/upload.html /mnt/mmc/tmp/
/app/goahead/goahead -v --home /app/goahead/ /mnt/mmc &
echo "start.sh finish"
