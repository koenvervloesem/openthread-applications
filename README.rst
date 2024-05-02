###############################################################################################################
Building Wireless Sensor Networks with OpenThread: Developing CoAP Applications for Thread Networks with Zephyr
###############################################################################################################

.. image:: https://img.shields.io/github/license/koenvervloesem/openthread-applications.svg
   :target: https://github.com/koenvervloesem/openthread-applications/blob/main/LICENSE
   :alt: License

This repository contains the code discussed in the book `Building Wireless Sensor Networks with OpenThread: Developing CoAP Applications for Thread Networks with Zephyr <https://koen.vervloesem.eu/books/building-wireless-sensor-networks-with-openthread/>`_, published by `Elektor International Media <https://www.elektor.com>`_, as well as `a list of errors and their corrections <ERRATA.rst>`_ and some `additional tips and references <ADDITIONS.rst>`_.

**************
About the book
**************

This book will guide you through the operation of Thread, the setup of a Thread network, and the creation of your own Zephyr-based OpenThread applications to use it. You’ll acquire knowledge on:

* The capture of network packets on Thread networks using Wireshark and the nRF Sniffer for 802.15.4.
* Network simulation with the OpenThread Network Simulator.
* Connecting a Thread network to a non-Thread network using a Thread Border Router.
* The basics of Thread networking, including device roles and types, as well as the diverse types of unicast and multicast IPv6 addresses used in a Thread network.
* The mechanisms behind network discovery, DNS queries, NAT64, and multicast addresses.
* The process of joining a Thread network using network commissioning.
* CoAP servers and clients and their OpenThread API.
* Service registration and discovery.
* Securing CoAP messages with DTLS, using a pre-shared key or X.509 certificates.
* Investigating and optimizing a Thread device’s power consumption.

Once you‘ve set up a Thread network with some devices and tried connecting and disconnecting them, you’ll have gained a good insight into the functionality of a Thread network, including its self-healing capabilities. After you’ve experimented with all code examples in this book, you’ll also have gained useful programming experience using the OpenThread API and CoAP.

+----------------------+-------------------------------------+
| **Title**            | Building Wireless Sensor Networks with OpenThread: Developing CoAP Applications for Thread Networks with Zephyr         |
+----------------------+-------------------------------------+
| **Author**           | Koen Vervloesem                     |
+----------------------+-------------------------------------+
| **Publication date** | 2024-05-02                          |
+----------------------+-------------------------------------+
| **Number of pages**  | 256                                 |
+----------------------+-------------------------------------+
| **ISBN-13**          | 978-3-89576-618-3                   |
+----------------------+-------------------------------------+
| **Publisher**        | Elektor International Media (EIM)   |
+----------------------+-------------------------------------+

*************
Included code
*************

All example code from this book is included in this repository, stored in a directory for each chapter. For each chapter directory, Zephyr applications are stored in their own subdirectory, while Python code comes in a single Python file.

*****************
Download the code
*****************

You can download the code all at once with `Git <https://git-scm.com/>`_:

.. code-block:: shell

  git clone https://github.com/koenvervloesem/openthread-applications.git

The code is then downloaded into the directory ``openthread-applications``.

You can also download a ZIP file of all code by clicking on the green button **Code** at the top right of this page and then on **Download ZIP**.

Just selecting and copying code from the GitHub web page of a specific file you're interested in and pasting it in an editor may work, but isn't recommended. Especially with Python code the whitespace can become mixed up, which results in invalid code.

******
Errata
******

See the `ERRATA <ERRATA.rst>`_ document for the list of errors in the book and their corrections.

*********
Additions
*********

See the `ADDITIONS <ADDITIONS.rst>`_ document for some additional tips and references to interesting projects not mentioned in the book.

*******
License
*******

All code is provided by `Koen Vervloesem <http://koen.vervloesem.eu>`_ as open source software with the MIT license, unless noted otherwise in the license header of a file. See the `LICENSE <LICENSE>`_ for more information.
