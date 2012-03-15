# For XCSoar-nohorizon on Android: make a copy R.java for package
# org.xcsoar (from package org.xcsoar.nohorizon)

s:</project>:<target name="-pre-compile"><copy file="${gen.absolute.dir}/org/xcsoar/nohorizon/R.java" toFile="${gen.absolute.dir}/org/xcsoar/R.java"><filterset begintoken="package " endtoken=";"><filter token="org.xcsoar.nohorizon" value="package org.xcsoar;"/></filterset></copy></target></project>:g
