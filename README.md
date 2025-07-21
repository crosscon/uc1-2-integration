# crosscon-uc1-2

This is a repository for client/server mTLS applications for the CROSSCON
project.

Contents:
1. [References](#references)
1. [Structure](#structure)
1. [Building for buildroot and hypervisor](#building-for-buildroot-and-hypervisor)
1. [Building legacy version for NXP demo (PC) or under yocto](#building-legacy-version-for-nxp-demo-pc-or-yocto)

## References

Reference list:
* [SSL/TLS Overview](https://www.wolfssl.com/documentation/manuals/wolfssl/appendix04.html)
* [WolfSSL SSL tutorial](https://www.wolfssl.com/documentation/manuals/wolfssl/chapter11.html)
* [WolfSSL TLS Examples](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/README.md)
* [Up to speed on mTLS architecture](https://www.securew2.com/blog/mutual-tls-mtls-authentication)

## Structure

Below is the simplified repository structure with key components described.

```text
.
├── artifacts <-- A dir that's not tracked by git for build artifacts, etc.
├── buildroot <-- Crosscon buildroot specific files.
|   ├── config <-- Various configuration files for buildroot or it's components.
│   ├── patches <-- Patches for various buildroot components.
|   |               The issue is that in CROSSCON this is not a repository, thus
|   |               can't create patches with git or push changes.
│   └── rootfs_overlay <-- Rootfs overlay, for various configurations, etc.
├── certs   <-- Pregenerated certificates
├── scripts <-- Stores scripts for various automated tasks.
└── <sources and conf. files> <-- Source files are in the repo. root for best
                                  compat. between various build systems.
```

## Building for buildroot and hypervisor

The Crosscon demos build a custom OS that runs under hypervisor and uses
buildroot in the process. This section describes how to build the application
so it is embedded into built OS image.

### Prerequisites

To build the app embedded into system you must first
[build and run Crosscon demos for RPI](https://github.com/3mdeb/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/blob/master/env/README.md).

### Building the image

Here's how to build image.

#### Sync buildroot

Update `CROSSCON_REPO_PATH` in `scripts/settings.sh` with the path to the local
`CROSSCON-Hypervisor-and-TEE-Isolation-Demos` repository.

Next, execute `buildroot: sync` vs-code task, or
`scripts/buildroot_sync_src.sh`. The script does the following:
* Syncs configuration files: buildroot config, wolfsll config, opensc config.
* Updates `packages/Config.in` so the app is being built by default.
* Copies this repository contents to `buildroot/package/mtls`.

Remember to run this task every time you update the sources, or make it run
automatically when saving.

#### Building

To build the "OS" with the applications embedded in the system,
**the steps 8 to 10 from `build_rpi4.sh`** need to be rerun, and the
files must be then copied to SD card.

```bash
# Run the following inside the container
env/build_rpi4.sh --steps=8-10
```

To build hypervisor image run the following command.

```bash
# sudo is necessary here
sudo env/create_hyp_img.sh
```

To flash the image you can use `dd` command.

```bash
sudo dd if=./crosscon-demo-img.img of=<drive> bs=4M conv=fsync
```

#### Testing

To test if the apps work, you must now boot up 2 RPis with built image.

##### Booting up

Use UART to USB adapter to connect RPI to your machine and start up minicom.

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

Supply power to RPI and hit any key when asked to stop u-boot from attempting
auto-boot.

```text
[...]
scanning bus xhci_pci for devices... 2 USB Device(s) found
       scanning usb for storage devices... 0 Storage Device(s) found
Hit any key to stop autoboot:  0
U-Boot>
```

_Note: If you missed the timeframe, you can spam CTRL+C many times to achieve
same result._

Boot the image by manually loading it into the memory and "jumping" to it.

```text
fatload mmc 0 0x200000 crossconhyp.bin; go 0x200000
```

Perform the same steps for the second RPi.

##### Generating key pairs

This part describes how to generate key pair using PKCS#11 TA as secure storage.
This works only if app was built on buildroot (not for Zarhus).

**Note: The keys and certificates will be stored in initramfs and will disappear
after shutdown. The following steps need to be performed after each boot.**

##### Generate key pair

First step is to request IP via DHCP on both targets.
```bash
udhcpc -i eth0
```

Second step is to update `PI_SERVER_HOST` and `PI_CLIENT_HOST` in
`scripts/settings.sh`. Choose one of the platforms to be the server and the
other one to be the client.

```bash
[...]
export PI_SERVER_HOST=192.168.10.29
export PI_CLIENT_HOST=192.168.10.30
[...]
```

Replace the values with the output of `udhcpc -i eth0` for each platform.

**Note**: For development purposes, you can also deploy both server and client
certificates on a single target (`PI_SERVER_HOST`). In that case, you don't need
to specify `PI_CLIENT_HOST` address. To use single target set `SINGLE_TARGET`
variable to `true` in `scripts/settings.sh`

```bash
export SINGLE_TARGET=true
```

Run `buildroot: generate keys` vs-code task, or simply execute
`scripts/buildroot_ta_key_gen.sh` script. The public key will be fetched to
`artifacts/certs/` directory.

##### Generate certificates

Then, you should generate certificates for the server and client. Certificate
Signing Requests (CSRs) are generated using OpenSSL with PKCS#11. In this setup,
the host machine serves as the CA for targets (RPis). The CSRs are transferred
to the host and used for generating certificates using the CA certificate
located in `certs/ca-cert.pem`.

To generate them, either run `buildroot: generate certs` vs-code task or use
`scripts/buildroot_ta_cert_gen.sh`. The script will also set the date on target
devices, because it defaults to 1970 and messes with certificate validity.

##### Test if apps are working

Run the server binary on one of the platforms.

```bash
server-tls
```

Run the client binary on another platform.

```bash
client-tls <SERVER_IP>
```

Replace `<SERVER_IP>` with the IP address of your server RPi.

Expected result:

Server:

```text
# server-tls &
# Waiting for a connection...
Server accept status: 1
Client connected successfully
SSL cipher suite is ECDHE-ECDSA-AES256-GCM-SHA384
Client: hello

Shutdown complete
Waiting for a connection...
```

Client:

```text
# client-tls <SERVER_IP>
SSL cipher suite is ECDHE-ECDSA-AES256-GCM-SHA384
Message for server: hello
Server: I hear ya fa shizzle!

Shutdown not complete
Shutdown complete
```

Do not worry if the client prints `Segmentation fault` at the end. This is a
known issue, which does not affect the process.

## Running NXP demo
It is possible to deploy the server on RPI and use it with the client running on
`LPC55S69`. All that should be required is to build and deploy the solution as a
**single target** ([described here](#generating-key-pairs)) and run
`server-tls`.

**FIXME: Remove this or update commit when this get's merged**  
Make sure to build the NXP solution from
[this](https://github.com/crosscon/uc1-integration/pull/4) branch.

The certificates should be already valid, but in case they expire, they can be
easily regenerated via `scripts/gen_and_convert_certs.sh` script. As of now the
generated c header files contents must be manually transferred (added to NXP
solution source code).
