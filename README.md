Improve readability of InfoBoxes and still give maximum screen space to the Map

This feature makes the constant CONTROLHEIGHTRATIO in InfoBoxes/InfoBoxLayout.cpp user-accessible.
Previously it was set to 7.4 which doesn't always yield the optimal results in terms of trade-off
between screen space for InfoBoxes vs. screen space for the Map.
