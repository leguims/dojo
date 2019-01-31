# README


## Prerequesites

Raspberry Pi (raspbian):
```
sudo apt install build-essential cmake
sudo apt install libncurses5-dev libzmq3-dev libczmq-dev python3-zmq
```

[Python 3.6+](https://www.python.org/downloads/)


## GitHub

[thales6](https://github.com/thales6)

## ncurses

* [Homepage](https://invisible-island.net/ncurses/)
* [HOWTO](http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/)
* [HOWTO Python3](https://docs.python.org/3/howto/curses.html)

## ZeroMQ

* [Web site](http://zeromq.org/)
* [API](http://api.zeromq.org)
* [zguide](http://zguide.zeromq.org/) [explanations](http://zguide.zeromq.org/page:all#Sending-and-Receiving-Messages)
* [examples](https://github.com/booksbyus/zguide)
* [pyzmq](https://learning-0mq-with-pyzmq.readthedocs.io/)


## ZMQ Patterns

````
zeromq = sockets simplifiées
        ~ message-oriented middleware
        != API de haut niveau
        != broker
        != corba, ice, amqp, mqtt, rpc, ...
````

* développé en mode collectif sur github
* léger
* rapide
* disponible partout et dans tous les langages
* adaptable à tous les besoins (ou presque!)


## Dojo

### Item log : [PUSH/PULL](http://api.zeromq.org/4-2:zmq-socket#toc15)

![pattern](patterns/zmq_pushpull.png)

* producteurs de traces: PUSH
* consommateur de traces: PULL
* transport: ipc
* avantages / inconvénients ?

* Autre possibilité: [PUB/SUB](http://api.zeromq.org/4-2:zmq-socket#toc10)
* Autre possibilité (DRAFT API): [RADIO/DISH](http://api.zeromq.org/4-2:zmq-socket#toc6)

### Serveur -> Joueurs : PUB/SUB

### Joueur -> Serveur : PUSH/PULL
