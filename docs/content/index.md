---
title: XCSoar Docs
description: Documentation for XCSoar, the open-source glide computer.
navigation: false
---

::u-page-hero
---
orientation: horizontal
---
#headline
Documentation

#title
XCSoar 7

#description
The open-source glide computer for cross-country, competition, and training flights.

#links
  :::u-button
  ---
  size: xl
  to: /manual/installation/getting-started
  trailing-icon: i-lucide-arrow-right
  ---
  Get started
  :::

  :::u-button
  ---
  color: neutral
  size: xl
  to: /quick-guide/xcsoar-in-a-flash
  variant: outline
  ---
  Quick guide
  :::

#default
![XCSoar main screen](/img/figures/plain.png)
::

::u-page-section
#headline
Find your way around

#title
From the first start to the last InfoBox

#description
The manual is the complete reference. The quick guide brings you into the air with the fewest steps.

#default
  :::u-page-grid{class="lg:grid-cols-2"}
    ::::u-page-card
    ---
    icon: i-lucide-book-open
    title: User Manual
    description: Installation, user interface, navigation, cross country tasks, glide computer, airspace, configuration and data files, chapter by chapter.
    to: /manual/preface
    variant: subtle
    ---
    ::::

    ::::u-page-card
    ---
    icon: i-lucide-zap
    title: Quick Guide
    description: XCSoar in a flash. A checklist from installation to the after flight check for your first flights with XCSoar.
    to: /quick-guide/xcsoar-in-a-flash
    variant: subtle
    ---
    ::::
  :::

  :::u-page-grid
    ::::u-page-card
    ---
    icon: i-lucide-layout-grid
    title: InfoBox Reference
    description: Every InfoBox with its category, caption and meaning.
    to: /infobox
    variant: subtle
    ---
    ::::

    ::::u-page-card
    ---
    icon: i-lucide-tablet
    title: Hardware
    description: Devices XCSoar runs on and the instruments it connects to.
    to: /hardware
    variant: subtle
    ---
    ::::

    ::::u-page-card
    ---
    icon: i-lucide-terminal
    title: Developers
    description: Building XCSoar, the architecture and how to contribute.
    to: /dev
    variant: subtle
    ---
    ::::
  :::
::

::u-page-section
#headline
Popular topics

#title
Straight to the chapter you need

#features
  :::u-page-feature
  ---
  icon: i-lucide-download
  title: Installation
  description: What you need to run XCSoar, the software installation and the data files.
  to: /manual/installation/getting-started
  ---
  :::

  :::u-page-feature
  ---
  icon: i-lucide-settings
  title: Configuration
  description: The system setup dialogues, page by page, with every setting explained.
  to: /manual/configuration/configure-system
  ---
  :::

  :::u-page-feature
  ---
  icon: i-lucide-route
  title: Cross Country Tasks
  description: Ordered, goto and abort tasks, the task manager and flight analysis.
  to: /manual/cross-country-tasks/overview
  ---
  :::

  :::u-page-feature
  ---
  icon: i-lucide-calculator
  title: Glide Computer
  description: Flight modes, MacCready, final glide, safety heights and wind estimation.
  to: /manual/glide-computer/flight-modes
  ---
  :::

  :::u-page-feature
  ---
  icon: i-lucide-shield-alert
  title: Airspace and Traffic
  description: Airspace display and warnings, FLARM traffic and team flying.
  to: /manual/airspace/airspace-display
  ---
  :::

  :::u-page-feature
  ---
  icon: i-lucide-folder
  title: Data Files
  description: Maps, waypoints, airspace, polars, profiles and where XCSoar stores them.
  to: /manual/data-files/file-management
  ---
  :::
::

::u-page-c-t-a
---
title: Get XCSoar
description: XCSoar is free software. Downloads, hardware advice and news are on xcsoar.org.
variant: subtle
links:
  - label: xcsoar.org
    to: https://xcsoar.org
    target: _blank
    icon: i-lucide-globe
  - label: Source on GitHub
    to: https://github.com/XCSoar/XCSoar
    target: _blank
    color: neutral
    variant: outline
    icon: i-simple-icons-github
---
::
