#!/bin/bash

nodename=$(hostname)

echo "{ \"id\":\"\", \"s\":\"testnode\", \"t\":\"${nodename}\", \"g\":\"${1}\", \"c\":\"${2}\", \"v\": \"${3}\" }"
gnutls-cli --no-ca-verification \
--x509keyfile=/etc/copilot/services/ssl_my_keys/${nodename}-key.pem \
--x509certfile=/etc/copilot/services/ssl_my_keys/${nodename}-cert.pem  \
localhost:4567

