# Third-Party Notices

This project is licensed under the MIT License (see [LICENSE](LICENSE)) and
uses the following third-party components.

## Qt6 (Widgets)

This application uses the Qt framework, dynamically linked (Qt's `.dll`
files are distributed alongside this application's executable, not
compiled into it, and are not modified in any way).

- License: GNU Lesser General Public License v3 (LGPLv3)
- Source and license text: https://www.qt.io/download-open-source
- Full LGPLv3 text: https://www.gnu.org/licenses/lgpl-3.0.html

Because Qt is linked dynamically, this application's own source code is
**not** required to be released under LGPL, and is not - it remains
MIT-licensed. Only Qt's own components remain under their original LGPLv3
terms. You may replace the bundled Qt `.dll` files with a compatible
build of your own, per LGPLv3.

## GoogleTest

Used as a development-time dependency for the automated test suite
(`tests/`). Not linked into, or distributed with, the playable
application (`gui.exe`).

- License: BSD 3-Clause License
- Source: https://github.com/google/googletest
