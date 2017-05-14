# copilot - A linux server monitoring and manageing tool

Hello out there, this project aims an easy-to-use web-based monitoring and administration tool for linux systems.

### Just, Why ?
Yes, there are many tools out there to manager your linux-systems, but for me it was difficult to setup, all uses
some kind of web-server and magic behind the scenes. So i started this project mainly to manage my home servers and
my servers on the internet. And hey, maybe somebody can use it also :)

## Architecture
Copilot is devided into different systems:
- Client - An linux machine ( like a raspberry pi, an NAS, a web-server or any other unobserved linux machine )
- Server - Clients connect to these ( over mqtt )
- Engineering - The linux-machine from where you monitor and administrate the clients

There is not realy a difference between this systems, there are just a preset of services. For example the client
can only contain the mqtt-client and the nft-service. So this client will only be capable of using nft


### Systems

#### Client
The "client" contains following default components:
- mqtt-client
- nft-service

#### Server
The server is (yet) only an mqtt-server. Copilot is tested with mosquitto

#### Engineering
The "engineering-station" contains following default components:
- mqtt-client
- websocket server



## Services

### Core Dependencies:
Every component need this packages:
- libjansson


### mqtt-client
This service sends all requests/answers to an mqtt-server
Of yourse every system should have that to communicate over the network

#### Additional Dependencies
- mosquitto ( for mqtt-support )


### nft
The nft-service provide setting up the firewall with nftables

#### Additional Dependencies
- sudo ( to gain system call access )
- nftable support by the kernel
- nftable package installed



### websocket-server
The websocket-server is mainly for the engineering station to communicate with the
copilot-web-interface

#### Additional Dependencies
- libsodium ( for encryption and authentication )
- libwebsockets




