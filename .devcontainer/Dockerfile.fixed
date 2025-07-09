FROM mcr.microsoft.com/azurespheresdk:latest

# Install USB utilities for device detection
RUN apt-get update && apt-get install -y usbutils

# Note: Removed the old ARM GCC installation as it conflicts with SDK 25.04
# The Azure Sphere SDK 25.04 includes the proper ARM toolchain

# Add user to dialout group for serial port access
RUN usermod -a -G dialout root

# Set up proper permissions for USB devices
RUN echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="045e", ATTRS{idProduct}=="*", MODE="0666", GROUP="dialout"' > /etc/udev/rules.d/99-azure-sphere.rules
