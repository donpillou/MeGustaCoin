MeGustaCoin
===========

MeGustaCoin is an unofficial Bitstamp market trading client. Its aim is to provide a fast and easy way to
place, withdraw and (especially) edit orders on Bitstamp.

![MeGustaCoin Screenshot](/MeGustaCoin.png)

Although the key features (listing, editing, adding and canceling orders) are functional, MeGustaCoin is in an
early stage of development and may require some testing and feedback. Feel free to report issues and to provide
pull requests and ideas. If you are desperate for a feature, request it, but keep in mind that my time to work
on MeGustaCoin is limited.

Please note that this software is release under the GPL version 2 (see LICENSE file). Hence, MeGustaCoin comes
with absolutely no warranty and the author/authors cannot be hold responsible for any correctly or incorrectly
initiated transactions using this software. Be careful if you are planning on using MeGustaCoin (or similar
software), since it may easily contain malicious code that snatches some money from your account. If you are
not exactly sure what you are doing, you should stop here.


Getting it to work
==================

MeGustaCoin is designed to be easily compiled from source. That way, everyone can look into the source code
(to ensure its clean) and you do not have to reply on precompiled binaries that may contain malicious code.
You should not try to download a precompiled binaries from third-party sources. Really, I strongly recommend
compiling MeGustaCoin from source!

Windows
-------

Compiling MeGustaCoin requires the following steps:
 1. Get and install Git and Visual Studio 2013 (its free, the Express version is sufficient)
 2. Using Git, clone https://github.com/donpillou/MeGustaCoin onto you hard drive and create a working copy
 3. Initialize the Git submodules of your working copy (recursively!!)
 4. In the root directory of your wokring copy, call the generate.bat script to generate project files
 5. Open MeGustaCoin.sln in Visual Studio and compile the MeGustaCoin project

Linux
-----

I am working on that.

Mac
---

There is no Mac version and I will not look into creating one unless someone gifts me an Apple device that I
could use for development/testing.


FAQ
===

Q: Where do I get the login key and secret?<br>
A: In order to use MeGustaCoin, you have to create an API key/secret pair for your Bitstamp account on
Bitstamp's website (Account -> Security -> API Access). https://www.bitstamp.net/account/security/api/
