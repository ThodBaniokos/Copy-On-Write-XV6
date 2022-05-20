# Copy-On-Write-XV6
Implementation of the copy on write mechanism as part of a project for K22 Operating Systems Course.

# Basic Idea
The idea behind Copy On Write (COW) is that when a new process is created as a child of another the two processes will point at the same pages in physical memory and the pages of the parent process will not be coppied until one of the processes needs to write something.

# Visual Representation
<img src="https://github.com/ThodBaniokos/Copy-On-Write-XV6/blob/main/img/Copy%20On%20Write%20Example%201.png" alt="Copy On Write Example"/>

# Run program
To run this program the xv6 operating sytem and qemu emulator must be installed on the system, more information here <a href ="https://gcallah.github.io/OperatingSystems/xv6Install.html">xv6</a> and here <a href ="https://www.qemu.org/download/">qemu</a>. Then all you have to do is run this command: make && make qemu.
