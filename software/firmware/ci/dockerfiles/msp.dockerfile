# Create docker image for building MSP software

# Versions here: https://hub.docker.com/_/debian
FROM i386/debian:8.11-slim

# Install all necessary packages
RUN apt-get update && apt-get install --no-install-recommends -y \
    git-core \
    wget \
    unzip \
    ca-certificates \
    make \
    python3-dev \
    python3-pip \
    python3-setuptools

# Install python libraries
RUN pip3 install wheel
RUN pip3 install crcmod

WORKDIR /

# Download MSP430 compiler
# See more version here: http://www.ti.com/tool/MSP-CGT
# Latest version: http://software-dl.ti.com/codegen/esd/cgt_public_sw/MSP430/19.6.0.STS/ti_cgt_msp430_19.6.0.STS_linux_installer_x86.bin
RUN wget http://software-dl.ti.com/codegen/esd/cgt_public_sw/MSP430/18.12.3.LTS/ti_cgt_msp430_18.12.3.LTS_linux_installer_x86.bin
RUN chmod a+x ti_cgt_msp430_18.12.3.LTS_linux_installer_x86.bin
RUN printf '\ny\n' | ./ti_cgt_msp430_18.12.3.LTS_linux_installer_x86.bin
RUN rm ti_cgt_msp430_18.12.3.LTS_linux_installer_x86.bin

# Copy in library files and unzip
WORKDIR /

COPY ci/helpers/msp430.zip /msp430.zip
RUN unzip -q msp430.zip
RUN rm msp430.zip

WORKDIR /











