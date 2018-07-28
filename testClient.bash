#!/bin/bash

# {"id":"0c946862-1ae6-4eaf-bd56-12e179f24dd8","s":"hacktop7","t":"","g":"bus","c":"subscribe","v":"{ \"t\": \"hacktop7\" }"}
# {"id":"xxx1","s":"testnode","t":"","g":"bus","c":"subscribe","v":"{ \"t\": \"all\" }"}
# { "id":"xxx2", "s":"testnode", "t":"all", "g":"co", "c":"ping", "v": "" }

certpath=/etc/copilot/ssl_private
nodename=$(hostname)
hostname=localhost
port=4444



gnutls-cli --no-ca-verification \
--x509keyfile=${certpath}/${nodename}-key.pem \
--x509certfile=${certpath}/${nodename}-cert.pem  \
${hostname}:${port}

