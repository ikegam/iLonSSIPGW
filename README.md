# i.Lon® Smart Server SOAP to IEEE1888 Gateway

## Overview

This gateway provides a bridge between i.Lon Smart Servers and IEEE1888 protocol, enabling communication between LonWorks networks and IEEE1888-based systems.

## Installation for OpenBlocks

### Prerequisites
```bash
# aptitude install libcunit1-dev
# aptitude install build-essential git
# git clone https://github.com/ikegam/liblight1888.git
# cd liblight1888; make install
```

### Clone and Setup
```bash
# cd /root
# git clone https://github.com/ikegam/iLonSSIPGW
# cd iLonSSIPGW
```

### Build and Installation
```bash
# mkdir /var/log/ieee1888_ilonss_gw
# mkdir /var/cache/ieee1888_ilonss_gw
# make
# cp ieee1888_ilonss_gw.conf /etc/
# cp init-script /etc/init.d/ieee1888_ilonss_gw
```

### Launch
```bash
# /etc/init.d/ieee1888_ilonss_gw start
```

## Optional Configuration

### 1. Health Check (Process Monitoring)
To ensure the service keeps running, you can set up automatic monitoring:

1. Edit crontab:
   ```bash
   crontab -e
   ```

2. Add the following line to check every minute:
   ```
   * * * * * pgrep "ieee1888_ilonss" || (/etc/init.d/ieee1888_ilonss_gw stop; /etc/init.d/ieee1888_ilonss_gw start)
   ```

### 2. Auto-start on Boot
The health check above will handle automatic startup. However, for explicit boot-time startup:

1. Edit startup script:
   ```bash
   sudo vi /etc/rc.local
   ```

2. Add the following line:
   ```bash
   (/etc/init.d/ieee1888_ilonss_gw stop; /etc/init.d/ieee1888_ilonss_gw start)
   ```
