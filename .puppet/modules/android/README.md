Puppet Module for Android SDK
=============================

This Puppet module is used to install the Android SDK, 
along with platforms and other add-ons.

Supported platforms:

* Linux (RedHat, Debian families)
* Mac OS X

Examples
--------

To install the Android SDK in the default location (/usr/local/android on Linux
and /Developer/android on Mac OS X) you simply include the android class like so.

```
  include android
```

You can also change the default parameters like so:

```
  class { 'android':
    user       => 'someuser',
    group      => 'somegroup',
    installdir => '/path/to/your/dir',
  }
```

To install an android platform, do it like so:

```
  android::platform { 'android-16': }
```

You can also install add-ons:

```
  android::addon { 'some-add-on': }
```

License
-------
```
  Copyright 2012 MaestroDev

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
```
