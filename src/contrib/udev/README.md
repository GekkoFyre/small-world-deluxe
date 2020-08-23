# Library of udev related C++ classes

## Getting Started

### Prerequisites

* libudev.so (provided by [systemd](https://www.freedesktop.org/wiki/Software/systemd/))
* [Library of POSIX related C++ classes](https://github.com/dimitry-ishenko/posix)

### Installation

Add as a submodule into an existing project:
```
git submodule add https://github.com/dimitry-ishenko/udev.git
```

### Usage

Example 1:

```cpp
udev::enumerate enu;
enu.match_subsystem("block");

auto devices = enu.get();
for(auto const& dev : devices)
{
    using namespace std;

    if(dev.type() == "partition" && dev.property("ID_BUS") == "ata")
    {
        cout << "Found hard disk" << endl
             << "Path: " << dev.path() << endl
             << "Node: " << dev.node() << endl
             << "  FS: " << dev.property("ID_FS_TYPE") << endl
             << "Name: " << dev.name() << endl
             << "  No: " << dev.num () << endl;
        cout << endl;
    }
}
```

Example 2:

```cpp
udev::monitor mon;
mon.match_device("block");

for(;;)
{
    using namespace std::chrono_literals;
    using namespace std;

    if(auto dev = mon.try_get_for(30s))
    {
        switch(dev.action())
        {
        case udev::added:
            if(dev.type() == "partition" && dev.property("ID_BUS") == "usb")
            {
                cout << "USB drive plugged in" << endl
                     << "Path: " << dev.path() << endl
                     << "Node: " << dev.node() << endl
                     << "  FS: " << dev.property("ID_FS_TYPE") << endl
                     << "Name: " << dev.name() << endl
                     << "  No: " << dev.num () << endl;
                cout << endl;
            }
            break;

        case udev::removed:
            if(dev.type() == "partition" && dev.property("ID_BUS") == "usb")
            {
                cout << "USB drive " << dev.node() << " unplugged" << endl;
                cout << endl;
            }
            break;
        }
    }
    else
    {
        cout << "Nothing seems to be happening" << endl;
        break;
    }
}
```

See also: [libudev and Sysfs Tutorial](http://www.signal11.us/oss/udev/)

## Authors

* **Dimitry Ishenko** - dimitry (dot) ishenko (at) (gee) mail (dot) com

## License

This project is distributed under the GNU GPL license. See the
[LICENSE.md](LICENSE.md) file for details.

## Acknowledgments
