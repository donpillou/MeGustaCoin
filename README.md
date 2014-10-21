MegucoClient
===========

MegucoClient is a client for the Meguco Trade Framework. The aim of the Meguco Trade Framework project is to
provide a framework to develop, optimize and control bots for Bitcoin exchanges like Bitstamp.

Please note that this software is release under the GPL version 2 (see LICENSE file). Hence, The Meguco Trade
Framework comes with absolutely no warranty and the author/authors cannot be hold responsible for any
correctly or incorrectly initiated transactions from this software. Be careful if you are planning on using
MegucoClient (or similar software), since it may easily contain malicious code that snatches some money from
your account. If you are not exactly sure what you are doing, you should stop here.


Getting started
===============

The Meguco Trade Framework consists of a client (MegucoClient), an exchange data collector daemon (MegucoData)
and a bot manager daemon (MegucoBot).

All components of the Meguco Trade Framework are designed to be easily compilable from source. That way,
everyone can look into the source code (to ensure its clean) and you do not have to rely on precompiled
binaries that may contain malicious code. You should not try to download a precompiled binaries from
third-party sources.

Windows
-------

Compiling the Meguco Trade Framework requires the following steps:
 1. Get and install Git and Visual Studio 2013 (its free, the Express version is sufficient)
 2. Using Git, clone
    https://github.com/donpillou/MegucoClient.git
    https://github.com/donpillou/MegucoData.git
    https://github.com/donpillou/MegucoBot.git
    onto you hard drive and create your working copies
 3. Initialize the Git submodules of your working copies (recursively!!)
 4. In the root directory of your working copies, call the generate.bat script to generate project files
 5. Open the Visual Studio Solution files (.sln) in Visual Studio and compile the MegucoClient, MegucoData
    and MegucoBot project

Linux
-----

```
sudo apt-get install git g++ libqt4-dev libcurl4-openssl-dev
cd ~/somedir
git clone https://github.com/donpillou/MegucoClient.git
cd MegucoClient
git submodule update --init --recursive
./generate
cd ~/somedir
git clone https://github.com/donpillou/MegucoData.git
cd MegucoData
git submodule update --init
./generate
cd ~/somedir
git clone https://github.com/donpillou/MegucoBot.git
cd MegucoBot
git submodule update --init
./generate
```
