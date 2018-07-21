#!/bin/bash

#/tmp/copilotd/copilotd --createServe $(cat /etc/hostname):4444
/tmp/copilotd/copilotd --configpath /tmp/copilotd --nodename testclient --createConnection 127.0.0.1:4444
/tmp/copilotd/copilotd --configpath /tmp/copilotd --nodename testclient 

