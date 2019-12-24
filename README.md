Improve readability of Title and Comment in InfoBoxes

It is useful in partucular for high and narrow screen orientation like OpenVario in portrait orientation.

To achieve this, I introduce a set of 10 InfoBoxes. This fits in between the existing layouts for 8 and 12 InfoBoxes.
This gives a better balance/tradeoff between readability and number of InfoBoxes.
This patch introduces 3 additional screen layouts: TOP_LEFT_10, BOTTOM_RIGHT_10, SPLIT_10.

An additional patch will follow to make the constant CONTROLHEIGHTRATIO in InfoBoxes/InfoBoxLayout.cpp user-accessible.
