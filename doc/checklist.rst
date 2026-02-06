Checklists
==========

XCSoar supports custom checklists with Markdown formatting, clickable
links, and interactive checkboxes.

File Location
-------------

XCSoar looks for checklist files in the following locations:

1. The file specified in **Site Files > Checklist** setting
2. ``xcsoar-checklist.txt`` in the XCSoar data directory
3. Legacy: Files with ``.xcc`` extension in legacy search paths

The XCSoar data directory is typically:

- **Android**: ``/sdcard/XCSoarData/``
- **Linux**: ``~/.xcsoar/``
- **Windows**: ``My Documents\XCSoarData\``

File Format
-----------

Checklists use a simple text format with Markdown enhancements.

Pages
~~~~~

Each checklist can have multiple pages. Pages are defined by section
headers in square brackets::

    [Pre-Flight]
    Content for pre-flight page...

    [Takeoff]
    Content for takeoff page...

    [Landing]
    Content for landing page...

Markdown Formatting
-------------------

Headings
~~~~~~~~

Use ``#`` for headings::

    # Main Heading
    ## Sub Heading
    ### Smaller Heading

Bold Text
~~~~~~~~~

Use double asterisks or double underscores for bold::

    **Important text**
    __Also bold__

List Items
~~~~~~~~~~

Use ``-`` for bullet points::

    - First item
    - Second item
    - Third item

Checkboxes
~~~~~~~~~~

Interactive checkboxes that can be toggled by clicking or pressing Enter::

    - [ ] Unchecked item
    - [x] Checked item

Links
-----

XCSoar supports various link types that can be clicked to perform actions.

Web Links
~~~~~~~~~

Open URLs in the device browser::

    [XCSoar Website](https://xcsoar.org)
    https://xcsoar.org

Internal Links (xcsoar://)
~~~~~~~~~~~~~~~~~~~~~~~~~~

Open XCSoar dialogs and settings directly from your checklist.

**Configuration dialogs**::

    [Devices](xcsoar://config/devices)
    [Planes](xcsoar://config/planes)
    [Site Files](xcsoar://config/site-files)
    [Logger](xcsoar://config/logger)
    [Time](xcsoar://config/time)
    [InfoBox Sets](xcsoar://config/infoboxes)
    [Pages](xcsoar://config/pages)
    [WeGlide](xcsoar://config/weglide)

**Action dialogs**::

    [Flight Setup](xcsoar://dialog/flight)
    [Wind Settings](xcsoar://dialog/wind)
    [Task Manager](xcsoar://dialog/task)
    [Analysis](xcsoar://dialog/analysis)
    [Status](xcsoar://dialog/status)
    [Checklist](xcsoar://dialog/checklist)

Communication Links
~~~~~~~~~~~~~~~~~~~

Phone and messaging::

    [Call](tel:+1234567890)
    [Email](mailto:pilot@example.com?subject=Flight%20Report)
    [SMS](sms:+1234567890?body=Status%20update)

Messaging Apps
~~~~~~~~~~~~~~

Deep links to messaging applications::

    [WhatsApp](whatsapp://send?phone=1234567890&text=Hello)
    [Signal](https://signal.me/#p/+1234567890)
    [Telegram](tg://msg?to=+1234567890)
    [Skype](skype:username?call)
    [FaceTime](facetime:+1234567890)

Geographic Links
~~~~~~~~~~~~~~~~

Open locations in map applications. The ``geo:`` URI uses the format
``geo:latitude,longitude`` with optional zoom level ``?z=N``.

**Basic coordinates**::

    geo:47.4647,8.5492

**With zoom level** (higher = more zoomed in)::

    [Home Airfield](geo:47.4647,8.5492?z=15)

**Practical aviation examples**::

    # Airfields
    [LSZH Zurich](geo:47.4647,8.5492?z=14)
    [EDDF Frankfurt](geo:50.0379,8.5622?z=14)

    # Turnpoints
    [TP1 - Hilltop](geo:47.1234,8.5678?z=16)
    [TP2 - Lake](geo:47.2345,8.6789?z=16)

    # Emergency fields
    [Alternate 1 - Farm field](geo:47.3456,8.4567?z=17)
    [Alternate 2 - Grass strip](geo:47.4567,8.3456?z=17)

    # Meeting points
    [Retrieve meeting point](geo:47.5,8.6?z=18)
    [Club hangar](geo:47.4640,8.5485?z=19)

**With location name query** (opens search)::

    [Search Zurich Airport](geo:0,0?q=Zurich+Airport)

File Links
~~~~~~~~~~

Open local documents (desktop platforms only)::

    [Aircraft POH](file:///path/to/document.pdf)

Example Checklist
-----------------

Here is a complete example checklist file for glider operations::

    [Pre-boarding - ABCD]
    # Airframe Check

    - [ ] **A** - Airframe
    - [ ] **B** - Ballast
    - [ ] **C** - Controls
    - [ ] **D** - Dollies removed

    [Post-boarding - CHAOTIC]
    # Cockpit Check

    - [ ] **C** - Control access
    - [ ] **H** - Harness secure
    - [ ] **A** - Airbrakes and flaps
    - [ ] **O** - Outside clear, Options set
    - [ ] **T** - Trim
    - [ ] **I** - Instruments
    - [ ] **C** - Canopy locked

    **XCSoar Setup**
    - [ ] [Flight Setup](xcsoar://dialog/flight) - Weight & ballast
    - [ ] [Wind](xcsoar://dialog/wind) - Set initial wind

    [Outlanding - SSSSSW]
    # Field Selection

    - [ ] **S** - Size and shape adequate
    - [ ] **S** - Slope acceptable
    - [ ] **S** - Surface suitable
    - [ ] **S** - Surroundings clear
    - [ ] **S** - Stock/animals
    - [ ] **W** - Wind direction

    [Pre-landing - FUST]
    # Approach Check

    - [ ] **F** - Flaps set
    - [ ] **U** - Undercarriage down
    - [ ] **S** - Speed appropriate
    - [ ] **T** - Trim set

    [Emergency]
    # Emergency Contacts

    - [Call Club](tel:+1234567890)
    - [Send Position](sms:+1234567890?body=Need%20assistance)
    - [Location](geo:47.5,8.5)

    [References]
    - [XCSoar Manual](https://xcsoar.org/discover/manual.html)
    - [Weather](https://www.windy.com)

Supported URI Schemes
---------------------

+-------------------+----------------------------------+------------------------+
| Scheme            | Purpose                          | Platform Support       |
+===================+==================================+========================+
| ``http://``       | Web pages                        | All                    |
+-------------------+----------------------------------+------------------------+
| ``https://``      | Secure web pages                 | All                    |
+-------------------+----------------------------------+------------------------+
| ``xcsoar://``     | Internal dialogs                 | All                    |
+-------------------+----------------------------------+------------------------+
| ``tel:``          | Phone calls                      | Mobile, Desktop*       |
+-------------------+----------------------------------+------------------------+
| ``mailto:``       | Email                            | All                    |
+-------------------+----------------------------------+------------------------+
| ``sms:``          | Text messages                    | Mobile, Desktop*       |
+-------------------+----------------------------------+------------------------+
| ``geo:``          | Map coordinates                  | All                    |
+-------------------+----------------------------------+------------------------+
| ``whatsapp://``   | WhatsApp                         | Where app installed    |
+-------------------+----------------------------------+------------------------+
| ``sgnl://``       | Signal (unofficial)              | Where app installed    |
+-------------------+----------------------------------+------------------------+
| ``tg://``         | Telegram                         | Where app installed    |
+-------------------+----------------------------------+------------------------+
| ``skype:``        | Skype                            | Where app installed    |
+-------------------+----------------------------------+------------------------+
| ``facetime:``     | FaceTime                         | iOS/macOS only         |
+-------------------+----------------------------------+------------------------+
| ``file://``       | Local files                      | Desktop only           |
+-------------------+----------------------------------+------------------------+

\* Requires appropriate handler application installed

Keyboard Navigation
-------------------

When viewing a checklist:

- **Up/Down arrows**: Navigate between checkboxes and links
- **Enter**: Toggle checkbox or activate link
- **Left/Right arrows**: Switch between checklist pages

Tips
----

1. **URL Encoding**: Spaces in URLs must be encoded as ``%20``
2. **Phone Numbers**: Use international format with ``+`` prefix
3. **File Paths**: Use forward slashes, even on Windows
4. **Testing**: Test links on your target device before flight
