FROM mcr.microsoft.com/azurespheresdk:latest

# Install GNU Arm Embedded Toolchain
RUN curl -L -O https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2 && \
    tar -xf gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2 && \
    rm gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2;
ENV PATH "$PATH:/:/gcc-arm-none-eabi-9-2019-q4-major/bin"
