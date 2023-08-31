### Introduction

This Visual Studio solution contains two projects. *libcapnotrainergo* is basically a wrapper of ASIO-serial port with async read-write functionality wrapped around the data parsing based on CapnoTrainer Universal Dongle. 


The libcapnotrainergo is statically compiled. It offers a class that needs to passed two serial port comports path (COMx, COMy) and a user-defined call back function. Everytime a new data point is received, this callback function gets called. 


The *examples* project is there with a simple C++ file to show how to use the libcapnotrainergo. 


P.S: This is a development repository. The build/release repository will be public.

### Requirements: 

The repository is self contained and only tested for Windows system.

A CMkae project will be developed on top of it for cross compilation as ASIO is cross-compile.

The only dependeency (Asio) is header-only and is placed in the repo. 