# AddaJobmonitor
A Qt client to monitor and manage ADDA runs locally and remotely.

## What is it?
A Qt GUI based runtime control and supervision of Adda[https://github.com/adda-team/adda] computational
jobs running remotely (PBS or Linux) and locally (Windows and Linux).

## Supported systems
Monitoring works for local or remote Linux machines and clusters that run a Torque resouce manager and
understand commands such as `qstat`, `qhold` etc.  Remote machines (only non Windows) are accessed via
an external ssh so a working version needs to be on the local system (Putty on Windows, OpenSSH on Linux).


## Installation
No binary releases are provided for now, you need to compile from source, using the system supplied Qt or
an installation from her [https://www.qt.io/download-eval-for-applications-step-2/]

### All platforms
A file `addamachines.txt\ needs to be placed in the directory of the binary. It contains one account per line, as
well as a `localhost` entry, should you want to monitor runs on the local machine.

Example:
```
localhost
s
user22@mylinuxbox.uni.com

### Linux
Provided qt development packages are installed, which on Ubuntu are
`sudo apt-get install qt5-default qttools5-dev-tools`

```
qmake
make
```
in the `src/`.

The command 'ssh' (usually openssh) is expected, as well as correctly set up ssh-key based login
into the target, `ssh user0@my-cluster.world.com` is supposed to complete without password, enquiry step.
You need to set up ssh-key management for this. Check you distribution's documentation.

### Windows
Using a Qt SDK for Windows, MinGW works well

The Windows version finds currently open log files via file handles of the adda jobs running. Because there is
no easy way to do this, an external tool is invoked: you need a copy of handle.exe and place it into the system
wide search path (C:\WINDOWS is good for example).

Get handle.exe or handle64.exe from here: https://technet.microsoft.com/en-us/sysinternals/handle.aspx[https://technet.microsoft.com/en-us/sysinternals/handle.aspx]
Windows XP: The handle.exe published on the above link no longer works on win XP. You need an older version of the handle.exe binary (->Google).


On Windows this is assumes installation of Putty [http://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html] into the standard, suggested
  locations and correctly setup ssh key management (keybased login into the target machine) via pageant.exe.


### Development platforms
Or known to work:

- QtCreator 2.7.2, based on Qt 5.1.0 (32bit, MinGW 4.8), Win 7 (32bit)
- QtCreator 3.0.1, based on Qt 5.2.1 (gcc 4.8.2, 64bit), Ubuntu 14.01

# License
GPLv3
Uses: QCustomplot for plotting:  http://www.qcustomplot.com/,  Version: 2.0.0-beta, Autor: Emanuel Eichhammer, Version 2.0.0-beta, GPLv3



