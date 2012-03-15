# For XCSoar-testing on Android: make a copy R.java for package
# org.xcsoar (from packafge org.xcsoar.testing)

s:</project>:<target name="-pre-compile"><copy file="${gen.absolute.dir}/org/xcsoar/testing/R.java" toFile="${gen.absolute.dir}/org/xcsoar/R.java"><filterset begintoken="package " endtoken=";"><filter token="org.xcsoar.testing" value="package org.xcsoar;"/></filterset></copy></target></project>:g
