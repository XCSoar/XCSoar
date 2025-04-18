===============
Debugging Tipps
===============

Replaying NMEA Logs
-------------------

To replay NMEA logs in XCSoar, follow these steps:

1. **Configure the Device Driver in XCSoar**:
   - Open XCSoar in the device configuration add a new device.
   - Set the device to a TCP client with IP `127.0.0.1` and port `4353`.

2. **Replaying NMEA Logs**:
   - Build the FeenNMEA program by running:

.. code-block:: bash

    make ./output/UNIX/bin/FeedNMEA


     This will compile the `FeedNMEA` program that you will use to send NMEA data.

   - Replay the NMEA log using the following command:
.. code-block:: bash

        cat ./test/data/driver/FLARM/pflaf01.nmea | ./output/UNIX/bin/FeedNMEA tcp 4353
